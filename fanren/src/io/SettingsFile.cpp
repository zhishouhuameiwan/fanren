#include "io/SettingsFile.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

#include "io/StrictJson.h"

namespace fanren::io {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

// 一张「枚举 ↔ 文件里的词」表的一行。
template <typename E>
struct Word {
    E value;
    const char* id;
};

constexpr std::array<Word<DisplayScale>, 2> kScaleWords{{
    {DisplayScale::Integer, "integer"},
    {DisplayScale::Fit, "fit"},
}};
constexpr std::array<Word<EffectsLevel>, 2> kEffectsWords{{
    {EffectsLevel::Full, "full"},
    {EffectsLevel::Lite, "lite"},
}};
constexpr std::array<Word<TextSpeed>, 4> kTextSpeedWords{{
    {TextSpeed::Slow, "slow"},
    {TextSpeed::Normal, "normal"},
    {TextSpeed::Fast, "fast"},
    {TextSpeed::Instant, "instant"},
}};

template <typename E, std::size_t N>
const char* idOf(const std::array<Word<E>, N>& words, E value) noexcept {
    for (const Word<E>& word : words) {
        if (word.value == value) return word.id;
    }
    return words.front().id;
}

// 音量档位：缺 → 不动；不是整数 → 不动 + 警告；越界 → 夹紧 + 警告。
// 不走 detail::toInt：1e10 也是「越界」，该夹到 10，而不是当成读不懂的东西扔掉。
void readLevel(const json& root, const char* key, int& out, std::vector<std::string>& warnings) {
    const auto it = root.find(key);
    if (it == root.end()) return;
    if (!it->is_number()) {
        warnings.push_back(std::string(key) + " 不是数，按默认 " + std::to_string(out));
        return;
    }
    const double value = it->get<double>();
    if (!std::isfinite(value) || value != std::floor(value)) {
        warnings.push_back(std::string(key) + " 不是整数，按默认 " + std::to_string(out));
        return;
    }
    const double clamped = std::clamp(value, 0.0, static_cast<double>(kVolumeLevels));
    out = static_cast<int>(clamped);
    if (clamped != value) {
        warnings.push_back(std::string(key) + " 超出 0–" + std::to_string(kVolumeLevels) + "，夹到 " +
                           std::to_string(out));
    }
}

void readBool(const json& root, const char* key, bool& out, std::vector<std::string>& warnings) {
    const auto it = root.find(key);
    if (it == root.end()) return;
    if (!it->is_boolean()) {
        warnings.push_back(std::string(key) + " 不是 true / false，按默认 " + (out ? "true" : "false"));
        return;
    }
    out = it->get<bool>();
}

template <typename E, std::size_t N>
void readWord(const json& root, const char* key, const std::array<Word<E>, N>& words, E& out,
              std::vector<std::string>& warnings) {
    const auto it = root.find(key);
    if (it == root.end()) return;
    if (it->is_string()) {
        const std::string text = it->get<std::string>();
        for (const Word<E>& word : words) {
            if (text == word.id) {
                out = word.value;
                return;
            }
        }
        warnings.push_back(std::string(key) + " 认不出「" + text + "」，按默认 " + idOf(words, out));
        return;
    }
    warnings.push_back(std::string(key) + " 不是字符串，按默认 " + idOf(words, out));
}

bool knownAction(const std::string& id) {
    return std::find(kKeyActionIds.begin(), kKeyActionIds.end(), id) != kKeyActionIds.end();
}

// keys 的形状检查（一期）。返回空串 = 合格，parsed 里是读出来的表；否则是第一处毛病。
std::string parseKeys(const json& keys, std::map<std::string, KeySlots>& parsed) {
    if (!keys.is_object()) return "keys 不是对象";
    for (const auto& item : keys.items()) {
        const std::string& id = item.key();
        const json& slots = item.value();
        if (!knownAction(id)) return "认不出的动作「" + id + "」";
        if (!slots.is_array() || slots.size() != 2) return "「" + id + "」不是恰好两格";
        KeySlots pair{};
        for (std::size_t i = 0; i < 2; ++i) {
            int code = 0;
            if (!detail::toInt(slots[i], code) || code < 0 || code >= kScancodeLimit) {
                return "「" + id + "」的第 " + std::to_string(i + 1) + " 格不是 0–" +
                       std::to_string(kScancodeLimit - 1) + " 的整数";
            }
            pair[i] = code;
        }
        parsed[id] = pair;
    }
    return {};
}

void readKeys(const json& root, std::map<std::string, KeySlots>& out, std::vector<std::string>& warnings) {
    const auto it = root.find("keys");
    if (it == root.end()) return;
    std::map<std::string, KeySlots> parsed;
    const std::string problem = parseKeys(*it, parsed);
    if (!problem.empty()) {
        out.clear();
        warnings.push_back("键位表整张回默认：" + problem);
        return;
    }
    out = std::move(parsed);
}

SettingsRead parseSettingsImpl(std::string_view text) {
    SettingsRead read;
    read.found = true;
    // 读盘宽松：这份文件是给人手改的，记事本存出来带 BOM 也认（生成器写的数据文件才拒 BOM）。
    if (text.substr(0, 3) == "\xEF\xBB\xBF") text.remove_prefix(3);
    auto parsed = detail::parseStrictJson(text);
    if (!parsed) {
        read.error = "设置文件不是合法的 JSON，全部按默认：" + parsed.error;
        return read;
    }
    const json& root = parsed.value;
    if (!root.is_object()) {
        read.error = "设置文件的顶层不是对象，全部按默认";
        return read;
    }

    std::vector<std::string>& warnings = read.warnings;
    if (const auto version = root.find("version"); version != root.end()) {
        int number = 0;
        if (!detail::toInt(*version, number)) {
            warnings.push_back("version 不是整数，认得的字段照读");
        } else if (number != kSettingsVersion) {
            warnings.push_back("version 是 " + std::to_string(number) + "，本程序认的是 " +
                               std::to_string(kSettingsVersion) + "，认得的字段照读");
        }
    }

    Settings& s = read.settings;
    readLevel(root, "bgm_volume", s.bgmVolume, warnings);
    readLevel(root, "sfx_volume", s.sfxVolume, warnings);
    readBool(root, "fullscreen", s.fullscreen, warnings);
    readWord(root, "scale", kScaleWords, s.scale, warnings);
    readBool(root, "vsync", s.vsync, warnings);
    readWord(root, "effects", kEffectsWords, s.effects, warnings);
    readBool(root, "screen_shake", s.screenShake, warnings);
    readWord(root, "text_speed", kTextSpeedWords, s.textSpeed, warnings);
    readBool(root, "pad_rumble", s.padRumble, warnings);
    readKeys(root, s.keys, warnings);
    return read;
}

// 写盘用保序的对象：人打开文件看到的次序就是契约第 2 节那份样例的次序。
std::string settingsText(const Settings& s) {
    nlohmann::ordered_json root;
    root["version"] = kSettingsVersion;
    root["bgm_volume"] = std::clamp(s.bgmVolume, 0, kVolumeLevels);
    root["sfx_volume"] = std::clamp(s.sfxVolume, 0, kVolumeLevels);
    root["fullscreen"] = s.fullscreen;
    root["scale"] = displayScaleId(s.scale);
    root["vsync"] = s.vsync;
    root["effects"] = effectsLevelId(s.effects);
    root["screen_shake"] = s.screenShake;
    root["text_speed"] = textSpeedId(s.textSpeed);
    root["pad_rumble"] = s.padRumble;   // docs/gamepad.md 第 8 节：样例里排在 text_speed 后面
    // 键位按第 6 节那张表的次序写；表外的 id（只可能是代码里直接塞进来的）排在后面照写，不悄悄丢。
    nlohmann::ordered_json keys = nlohmann::ordered_json::object();
    for (const std::string_view id : kKeyActionIds) {
        if (const auto it = s.keys.find(std::string(id)); it != s.keys.end()) {
            keys[it->first] = nlohmann::ordered_json::array({it->second[0], it->second[1]});
        }
    }
    for (const auto& [id, slots] : s.keys) {
        if (!knownAction(id)) keys[id] = nlohmann::ordered_json::array({slots[0], slots[1]});
    }
    root["keys"] = std::move(keys);
    return root.dump(2) + "\n";
}

core::Result<bool> saveSettingsImpl(const Settings& settings, const std::string& pathStr) {
    const fs::path path(pathStr);
    if (path.has_parent_path()) {
        // 建不出目录不在这里报：下面打开文件失败时统一报，那句话更贴近玩家能做的事（同 saveGame）。
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
    }
    const std::string text = settingsText(settings);
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        return core::Result<bool>::failure("写不了设置文件：" + path.string());
    }
    stream << text;
    stream.flush();
    if (!stream.good()) {
        return core::Result<bool>::failure("写设置文件失败：" + path.string());
    }
    return core::Result<bool>::success(true);
}

}  // namespace

float volumeGain(int level) noexcept {
    const float x = static_cast<float>(std::clamp(level, 0, kVolumeLevels)) / static_cast<float>(kVolumeLevels);
    return x * x;
}

const char* displayScaleId(DisplayScale scale) noexcept {
    return idOf(kScaleWords, scale);
}

const char* effectsLevelId(EffectsLevel level) noexcept {
    return idOf(kEffectsWords, level);
}

const char* textSpeedId(TextSpeed speed) noexcept {
    return idOf(kTextSpeedWords, speed);
}

SettingsRead parseSettings(std::string_view text) {
    try {
        return parseSettingsImpl(text);
    } catch (const std::exception& e) {
        // parseStrictJson 自己接住了语法错；这里兜的是取值时意料之外的异常。坏文件不许挡启动。
        SettingsRead read;
        read.found = true;
        read.error = std::string("读设置文件时出现未预期的异常，全部按默认：") + e.what();
        return read;
    }
}

SettingsRead loadSettings(const std::string& path) {
    std::error_code ec;
    const bool exists = fs::exists(fs::path(path), ec);
    if (!ec && !exists) {
        return SettingsRead{};   // 第一次启动：全默认，不算错
    }
    // 查不清在不在（ec 非空）就照读：读不了会在下面如实报出来，不当成「第一次启动」悄悄过去。
    auto text = detail::readTextFile(path);
    if (!text) {
        SettingsRead read;
        read.found = true;
        read.error = "读不了设置文件，全部按默认：" + text.error;
        return read;
    }
    return parseSettings(text.value);
}

core::Result<bool> saveSettings(const Settings& settings, const std::string& path) {
    try {
        return saveSettingsImpl(settings, path);
    } catch (const std::exception& e) {
        return core::Result<bool>::failure("写设置文件时出现未预期的异常：" + path + "：" + e.what());
    }
}

}  // namespace fanren::io
