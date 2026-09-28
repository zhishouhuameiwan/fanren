// 系统设置文件（docs/settings.md 第 2 节）与两个纯函数（第 1 节：音量曲线、文字速度）。
//
// 判据抄契约原文，不从被测物推：默认值 = 改造前的行为（10 / 10 / 窗口 / 整数倍 / 开 / 完整 / 开 / 标准）；
// 音量曲线 (level/10)²；文字速度 慢 30 / 标准 60 / 快 120 / 瞬显单独判。
// 读盘宽松：缺文件 = 默认且成功；坏 JSON = 默认 + 失败；越界夹紧；认不出的枚举回默认；
// keys 往返；keys 形状不对整张回默认。写盘写全部字段。
// 一律写临时目录（tests/TempDir.h），仓库里的 saves/settings.json 一个字节不碰。
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>

#include "TempDir.h"
#include "game/DialogueScene.h"
#include "io/SettingsFile.h"

namespace {

namespace fs = std::filesystem;

using fanren::io::DisplayScale;
using fanren::io::EffectsLevel;
using fanren::io::Settings;
using fanren::io::SettingsRead;
using fanren::io::TextSpeed;

void writeText(const fs::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

std::string readText(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// 与改造前行为一一对应的那组默认值（契约第 1 节表格的「默认」一列），逐项写死。
void expectFactoryDefaults(const Settings& s) {
    EXPECT_EQ(s.bgmVolume, 10);
    EXPECT_EQ(s.sfxVolume, 10);
    EXPECT_FALSE(s.fullscreen);
    EXPECT_EQ(s.scale, DisplayScale::Integer);
    EXPECT_TRUE(s.vsync);
    EXPECT_EQ(s.effects, EffectsLevel::Full);
    EXPECT_TRUE(s.screenShake);
    EXPECT_EQ(s.textSpeed, TextSpeed::Normal);
    EXPECT_TRUE(s.keys.empty());
}

// 每一项都不是默认值的一份。
Settings everythingChanged() {
    Settings s;
    s.bgmVolume = 3;
    s.sfxVolume = 7;
    s.fullscreen = true;
    s.scale = DisplayScale::Fit;
    s.vsync = false;
    s.effects = EffectsLevel::Lite;
    s.screenShake = false;
    s.textSpeed = TextSpeed::Instant;
    s.keys = {{"up", {26, 0}}, {"confirm", {29, 44}}, {"action", {8, 20}}};
    return s;
}

}  // namespace

// ---------------------------------------------------------------------------
// 默认值与两个纯函数
// ---------------------------------------------------------------------------

TEST(SettingsDefaults, AreTheBehaviourBeforeTheSettingsExisted) {
    expectFactoryDefaults(Settings{});
}

TEST(VolumeCurve, ZeroIsSilenceTenIsTheOldLoudnessAndItOnlyGoesUp) {
    EXPECT_FLOAT_EQ(fanren::io::volumeGain(0), 0.f);
    EXPECT_FLOAT_EQ(fanren::io::volumeGain(10), 1.f) << "10 档 = 改造前的混音响度";
    EXPECT_FLOAT_EQ(fanren::io::volumeGain(5), 0.25f) << "平方曲线：(5/10)²";
    for (int level = 0; level < 10; ++level) {
        EXPECT_LT(fanren::io::volumeGain(level), fanren::io::volumeGain(level + 1)) << "第 " << level << " 档";
    }
    // 越界先夹紧：不会给混音器一个负增益或比原来还响的增益。
    EXPECT_FLOAT_EQ(fanren::io::volumeGain(-3), 0.f);
    EXPECT_FLOAT_EQ(fanren::io::volumeGain(99), 1.f);
}

TEST(TextSpeedRate, SlowNormalFastAreThirtySixtyAHundredTwentyAndInstantHasNoRate) {
    using fanren::game::DialogueScene;
    EXPECT_EQ(DialogueScene::graphemesPerSecond(TextSpeed::Slow), std::optional<double>(30.0));
    EXPECT_EQ(DialogueScene::graphemesPerSecond(TextSpeed::Normal), std::optional<double>(60.0))
        << "标准 = 改造前的 60 字/秒";
    EXPECT_EQ(DialogueScene::graphemesPerSecond(TextSpeed::Fast), std::optional<double>(120.0));
    // 瞬显单独判：它不是一个「很快的速率」，是一上来就全显。
    EXPECT_FALSE(DialogueScene::graphemesPerSecond(TextSpeed::Instant).has_value());
}

// ---------------------------------------------------------------------------
// 读盘宽松
// ---------------------------------------------------------------------------

TEST(SettingsFile, AMissingFileIsTheDefaultsAndNotAnError) {
    const fanren::test::TempDir dir("fanren_settings_missing");
    const SettingsRead read = fanren::io::loadSettings((dir.path() / "settings.json").string());
    EXPECT_FALSE(read.found);
    EXPECT_TRUE(read.error.empty()) << read.error;
    EXPECT_TRUE(read.warnings.empty());
    expectFactoryDefaults(read.settings);
}

TEST(SettingsFile, EveryFieldSurvivesARoundTrip) {
    const fanren::test::TempDir dir("fanren_settings_roundtrip");
    const std::string path = (dir.path() / "nested" / "settings.json").string();
    const Settings original = everythingChanged();
    ASSERT_TRUE(fanren::io::saveSettings(original, path).ok) << "目录不在要自己建";

    const SettingsRead read = fanren::io::loadSettings(path);
    EXPECT_TRUE(read.found);
    EXPECT_TRUE(read.error.empty()) << read.error;
    EXPECT_TRUE(read.warnings.empty()) << read.warnings.front();
    EXPECT_EQ(read.settings, original);
    // 键位逐项核一遍：operator== 相等的前提下，这几个数就是契约第 2 节那份样例里的数。
    EXPECT_EQ(read.settings.keys.at("up"), (fanren::io::KeySlots{26, 0}));
    EXPECT_EQ(read.settings.keys.at("confirm"), (fanren::io::KeySlots{29, 44}));
    EXPECT_EQ(read.settings.keys.at("action"), (fanren::io::KeySlots{8, 20}));

    // 默认值也照样往返（全部字段都写，不省略「和默认一样」的项）。
    const std::string plainPath = (dir.path() / "plain.json").string();
    ASSERT_TRUE(fanren::io::saveSettings(Settings{}, plainPath).ok);
    EXPECT_EQ(fanren::io::loadSettings(plainPath).settings, Settings{});
}

TEST(SettingsFile, TheFileNamesEveryFieldWithTheContractsWords) {
    const fanren::test::TempDir dir("fanren_settings_words");
    const fs::path path = dir.path() / "settings.json";
    ASSERT_TRUE(fanren::io::saveSettings(everythingChanged(), path.string()).ok);
    const std::string text = readText(path);
    for (const char* piece : {"\"version\": 1", "\"bgm_volume\": 3", "\"sfx_volume\": 7", "\"fullscreen\": true",
                              "\"scale\": \"fit\"", "\"vsync\": false", "\"effects\": \"lite\"",
                              "\"screen_shake\": false", "\"text_speed\": \"instant\"", "\"keys\""}) {
        EXPECT_NE(text.find(piece), std::string::npos) << "写出来的文件里没有 " << piece << "\n" << text;
    }
    // version 写在最前：人打开文件第一眼看到的是它。
    EXPECT_LT(text.find("\"version\""), text.find("\"bgm_volume\""));

    // 默认值那一份也写全部字段。
    ASSERT_TRUE(fanren::io::saveSettings(Settings{}, path.string()).ok);
    const std::string plain = readText(path);
    for (const char* piece : {"\"bgm_volume\": 10", "\"fullscreen\": false", "\"scale\": \"integer\"",
                              "\"vsync\": true", "\"effects\": \"full\"", "\"screen_shake\": true",
                              "\"text_speed\": \"normal\"", "\"keys\": {}"}) {
        EXPECT_NE(plain.find(piece), std::string::npos) << piece << "\n" << plain;
    }
}

TEST(SettingsFile, BrokenJsonFallsBackToTheDefaultsAndSaysWhy) {
    const fanren::test::TempDir dir("fanren_settings_broken");
    const fs::path path = dir.path() / "settings.json";
    const std::string broken = "{\n  \"bgm_volume\": 3,\n  \"sfx_volume\": \n}";
    writeText(path, broken);

    const SettingsRead read = fanren::io::loadSettings(path.string());
    EXPECT_TRUE(read.found);
    EXPECT_FALSE(read.error.empty()) << "坏文件要说出来";
    EXPECT_NE(read.error.find("第 4 行"), std::string::npos) << "带行号：" << read.error;
    expectFactoryDefaults(read.settings);   // 连写对了的 bgm_volume 也不取：半截文件不可信
    EXPECT_EQ(readText(path), broken) << "读盘不许动那个坏文件";

    // 顶层不是对象也算坏。
    writeText(path, "[1, 2, 3]");
    const SettingsRead array = fanren::io::loadSettings(path.string());
    EXPECT_FALSE(array.error.empty());
    expectFactoryDefaults(array.settings);
}

TEST(SettingsFile, OutOfRangeNumbersAreClampedAndMissingFieldsTakeTheDefault) {
    const SettingsRead read = fanren::io::parseSettings(R"({"version": 1, "bgm_volume": 15, "sfx_volume": -4})");
    EXPECT_TRUE(read.error.empty()) << read.error;
    EXPECT_EQ(read.settings.bgmVolume, 10);
    EXPECT_EQ(read.settings.sfxVolume, 0);
    EXPECT_EQ(read.warnings.size(), 2u) << "两处夹紧各留一句";
    // 其余字段缺着：各按默认，不算毛病。
    EXPECT_FALSE(read.settings.fullscreen);
    EXPECT_EQ(read.settings.scale, DisplayScale::Integer);
    EXPECT_EQ(read.settings.textSpeed, TextSpeed::Normal);

    // 大得离谱的数也是「越界」，夹到 10，而不是当成读不懂的东西扔掉。
    EXPECT_EQ(fanren::io::parseSettings(R"({"bgm_volume": 1e10})").settings.bgmVolume, 10);
    // 不是整数：那一项默认 + 警告。
    const SettingsRead half = fanren::io::parseSettings(R"({"bgm_volume": 4.5})");
    EXPECT_EQ(half.settings.bgmVolume, 10);
    EXPECT_EQ(half.warnings.size(), 1u);
}

TEST(SettingsFile, UnknownWordsAndWrongTypesFallBackPerFieldWithAWarning) {
    const SettingsRead read = fanren::io::parseSettings(R"({
        "fullscreen": "yes", "scale": "stretch", "vsync": false, "effects": "ultra",
        "screen_shake": 0, "text_speed": "normal", "sfx_volume": 6, "extra_field": [1, 2]
    })");
    EXPECT_TRUE(read.error.empty()) << "一两项认不出不算整份坏：" << read.error;
    EXPECT_FALSE(read.settings.fullscreen) << "\"yes\" 不是布尔：默认";
    EXPECT_EQ(read.settings.scale, DisplayScale::Integer) << "认不出的词：默认";
    EXPECT_EQ(read.settings.effects, EffectsLevel::Full);
    EXPECT_TRUE(read.settings.screenShake);
    // 写对了的照读：一项坏不连累别的项。
    EXPECT_FALSE(read.settings.vsync);
    EXPECT_EQ(read.settings.sfxVolume, 6);
    EXPECT_EQ(read.settings.textSpeed, TextSpeed::Normal);
    EXPECT_EQ(read.warnings.size(), 4u) << "四处各一句；多出来的字段直接忽略、不算警告";
}

TEST(SettingsFile, KeysOfTheWrongShapeResetTheWholeTable) {
    // 先验：形状对的表整张读进来。
    const SettingsRead good = fanren::io::parseSettings(R"({"keys": {"up": [26, 0], "skip": [224, 0]}})");
    ASSERT_EQ(good.settings.keys.size(), 2u);
    EXPECT_TRUE(good.warnings.empty());

    // 每一种形状毛病：整张回空表（= 全默认）+ 一句警告，别的字段不受牵连。
    for (const char* keys : {
             R"({"keys": {"up": [26, 0], "jump": [44, 0]}, "bgm_volume": 4})",   // 认不出的动作
             R"({"keys": {"up": [26], "down": [22, 0]}, "bgm_volume": 4})",       // 只有一格
             R"({"keys": {"up": [26, 0, 4]}, "bgm_volume": 4})",                  // 三格
             R"({"keys": {"up": [600, 0]}, "bgm_volume": 4})",                    // 扫描码越界（≥ 512）
             R"({"keys": {"up": [-1, 0]}, "bgm_volume": 4})",                     // 负数
             R"({"keys": {"up": ["W", 0]}, "bgm_volume": 4})",                    // 不是数
             R"({"keys": {"up": [2.5, 0]}, "bgm_volume": 4})",                    // 不是整数
             R"({"keys": [26, 0], "bgm_volume": 4})",                             // 整个不是对象
         }) {
        const SettingsRead read = fanren::io::parseSettings(keys);
        EXPECT_TRUE(read.error.empty()) << keys;
        EXPECT_TRUE(read.settings.keys.empty()) << "键位表没有整张回默认：" << keys;
        EXPECT_EQ(read.warnings.size(), 1u) << keys;
        EXPECT_EQ(read.settings.bgmVolume, 4) << "别的字段照读：" << keys;
    }
}

// 读盘宽松的两处（实现记录第 3 条）：记事本存出来的 UTF-8 BOM 照认；version 不是 1 只警告、认得的字段照读。
TEST(SettingsFile, AByteOrderMarkIsAcceptedBecausePeopleEditThisFileByHand) {
    const SettingsRead read = fanren::io::parseSettings("\xEF\xBB\xBF{\"version\": 1, \"bgm_volume\": 4}");
    EXPECT_TRUE(read.error.empty()) << read.error;
    EXPECT_TRUE(read.warnings.empty());
    EXPECT_EQ(read.settings.bgmVolume, 4);
}

TEST(SettingsFile, AnotherVersionIsOnlyAWarningAndTheKnownFieldsAreStillRead) {
    const SettingsRead read = fanren::io::parseSettings(R"({"version": 2, "sfx_volume": 3, "text_speed": "fast"})");
    EXPECT_TRUE(read.error.empty()) << read.error;
    ASSERT_EQ(read.warnings.size(), 1u);
    EXPECT_NE(read.warnings.front().find("version"), std::string::npos) << read.warnings.front();
    EXPECT_EQ(read.settings.sfxVolume, 3);
    EXPECT_EQ(read.settings.textSpeed, TextSpeed::Fast);
    // [配对] 写的是 1 就一句警告都没有。
    EXPECT_TRUE(fanren::io::parseSettings(R"({"version": 1, "sfx_volume": 3})").warnings.empty());
}

TEST(SettingsFile, AWriteFailureIsReportedNotSwallowed) {
    const fanren::test::TempDir dir("fanren_settings_unwritable");
    // 目标路径本身是一个目录：打不开来写。
    const fs::path blocker = dir.path() / "settings.json";
    fs::create_directories(blocker);
    const auto wrote = fanren::io::saveSettings(Settings{}, blocker.string());
    EXPECT_FALSE(wrote.ok);
    EXPECT_FALSE(wrote.error.empty());
}
