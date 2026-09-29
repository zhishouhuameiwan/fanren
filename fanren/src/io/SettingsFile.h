#pragma once
// 系统设置文件（施工契约 docs/settings.md 第 1、2 节）：<资产根>/saves/settings.json。
//
// 设置属于这台机器，不属于某一局，所以与存档分开放、分开读写；存档格式（v8）一个字节不动。
//
// **读盘宽松、写盘完整**：
//   · 文件不存在 → 全默认，不算错（第一次启动）；
//   · 读不了 / JSON 坏了 / 顶层不是对象 → 全默认，error 写原因（调用方打一行警告）。
//     这时**不覆盖那个坏文件**——那是 Application 的事：它只在玩家真的改了什么时才写；
//   · 缺字段 → 那一项默认；数值越界 → 夹紧；类型不对、枚举认不出 → 那一项默认；
//     多出来的字段 → 忽略。每一处「没照文件里写的办」都留一条 warnings；
//   · keys 形状不对 → **整张回空表**（= 全部动作用默认键位）+ 一条警告。只回滚一个动作会造出
//     别的冲突，整张回滚的结果才可预期。一期只查形状：动作 id 认得、每个动作恰好两个非负整数、
//     扫描码小于 kScancodeLimit。固定键 / 保留键 / 重复 / 某动作一个键都不剩这几条要引擎的键位表，
//     二期由引擎校验（Engine::setCustomKeys）。
//   · 写盘永远写全部字段，次序照契约第 2 节那份样例（version 在最前）。
//
// io 不依赖 engine：音量存 0–10 的档位、键位存「动作 id → 两个 SDL 扫描码数值」。换成引擎的增益
// （volumeGain）与 Engine::Key 是 Application 的事。
#include <array>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "core/Result.h"

namespace fanren::io {

inline constexpr int kSettingsVersion = 1;

// 音量档位 0–10（10 = 改造前的响度）。
inline constexpr int kVolumeLevels = 10;

// 键位的扫描码须小于它（SDL_SCANCODE_COUNT）。0 表示那一格空着。
inline constexpr int kScancodeLimit = 512;

enum class DisplayScale { Integer, Fit };      // 整数倍 / 铺满
enum class EffectsLevel { Full, Lite };        // 画面特效：完整 / 精简（关辉光、关景深）
enum class TextSpeed { Slow, Normal, Fast, Instant };

// 键位设置里的动作 id（契约第 6 节那张表，次序即表的次序）。二期引擎的 Engine::keyId
// 必须用同一组词——到时候加一条测试把两边钉在一起（共享词表只许有一种写法）。
inline constexpr std::array<std::string_view, 10> kKeyActionIds{
    "up", "down", "left", "right", "confirm", "cancel", "menu", "skip", "save", "action"};

// 一个动作的两格自定义键位（SDL 扫描码数值，0 = 空格位）。
using KeySlots = std::array<int, 2>;

// 缺省值一律等于改造前的行为（契约第 1 节）：不开设置面板的玩家，画面、声音、手感与改造前相同。
// 唯一的例外是手柄震动（docs/gamepad.md 第 8 节）：改造前根本不认手柄，没有「以前」可对，缺省开；键盘玩家永远感觉不到它。
struct Settings {
    int bgmVolume = kVolumeLevels;
    int sfxVolume = kVolumeLevels;
    bool fullscreen = false;
    DisplayScale scale = DisplayScale::Integer;
    bool vsync = true;
    EffectsLevel effects = EffectsLevel::Full;
    bool screenShake = true;
    TextSpeed textSpeed = TextSpeed::Normal;
    bool padRumble = true;   // 文件里的键 pad_rumble
    // 动作 id → 两格键位。没列出的动作用默认键位；空表 = 全部默认。一期只存、只往返，不推给引擎。
    std::map<std::string, KeySlots> keys;

    friend bool operator==(const Settings&, const Settings&) = default;
};

// 音量档位 → 线性增益：(level / 10)²。越界先夹到 0–10。0 → 0（静音），10 → 1（改造前的响度）。
// 平方曲线是听感上「每档差不多一样大」的近似（契约第 1 节）。
[[nodiscard]] float volumeGain(int level) noexcept;

// 文件里的词。写盘、读盘共用这几张表，面板不用它们（面板的字在 data/text/ui.json）。
[[nodiscard]] const char* displayScaleId(DisplayScale scale) noexcept;
[[nodiscard]] const char* effectsLevelId(EffectsLevel level) noexcept;
[[nodiscard]] const char* textSpeedId(TextSpeed speed) noexcept;

struct SettingsRead {
    Settings settings;                   // 永远可以直接用：读不进来时就是全默认
    bool found = false;                  // 文件在不在
    std::string error;                   // 非空 = 整份没读进来（已按全默认）
    std::vector<std::string> warnings;   // 读进来了，但有几项没照文件里写的办
};

// 解析一段设置文本（loadSettings 的后半截，公开出来给测试直接喂）。
[[nodiscard]] SettingsRead parseSettings(std::string_view text);

// 读盘。规则见本文件开头。不抛异常。
[[nodiscard]] SettingsRead loadSettings(const std::string& path);

// 写盘：全部字段，UTF-8、两格缩进、末尾换行。目录不在就建。失败如实返回。
[[nodiscard]] core::Result<bool> saveSettings(const Settings& settings, const std::string& path);

}  // namespace fanren::io
