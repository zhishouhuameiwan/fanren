// io/VisualLoader：美术产物描述文件（sprites/index.json、maps/<id>/meta.json）的读取器。
//
// 字段的规则分在两处量：人物表、物件、变体的缺省与坏文件在 SpriteAtlasTests，meta 的缺省与坏文件在
// MapVisualTests（那两处量的是「读 + 画面层换算」合起来的结果）。这里量两样：
//   * 两份文件共用的语法那一层——原先由两个手写读取器（界面层 ui::Json、世界画面 game::VisualJson）
//     各自守着：坏一个字就报错且报得出行号、拒 BOM、限嵌套、UTF-8 与转义原样还原、整数不收小数。
//     统一到 io 层的 nlohmann 之后，这些口径一条也不能少；
//   * 战斗画面要的那几样：人物表的战斗帧与朝向、非人形敌人、按战斗覆写的外观、meta 里的战斗背景。
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/model/Visual.h"
#include "io/VisualLoader.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::BattleFacing;
using fanren::core::TimeOfDay;
using fanren::io::parseMapMeta;
using fanren::io::parseSpriteIndex;

// 仓库根：测试可能从 build-xxx/ 或工程根启动。判据用一定存在的那张图。
fs::path repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_hanjiacun.tmj")) return candidate;
    }
    return ".";
}

// 一张四向齐全的人物表，拼进各条用例的 JSON 里。
constexpr const char* kWalk = R"("walk": {"down": [0,1,2], "left": [3,4,5], "right": [6,7,8], "up": [9,10,11]})";

std::string indexWithSheet(const std::string& sheetFields) {
    return std::string(R"({"sheets": {"a": {"file": "a.png", )") + sheetFields + ", " + kWalk + "}}}";
}

// ---------------------------------------------------------------------------
// 语法
// ---------------------------------------------------------------------------

TEST(VisualLoader, RejectsWhatTheGeneratorWouldNeverWrite) {
    // 两个手写读取器各自的拒收清单合在一起。每一条都得报错，且报错里带着位置——那是人去找坏处的唯一线索。
    const char* broken[] = {
        "",                                  // 空文件
        "{\"a\": 1,}",                       // 尾逗号
        "[1, 2,]",                           // 数组尾逗号
        "[1 2]",                             // 缺逗号
        "{'a': 1}",                          // 单引号
        "{a: 1}",                            // 键不带引号
        "{\"a\" 1}",                         // 缺冒号
        "{\"a\": \"unterminated}",           // 字符串没收尾
        "{\"a\": \"tab\there\"}",            // 字符串里未转义的控制字符
        "01",                                // 前导零
        "[01]",
        "+1",                                // 正号
        "[1.]",                              // 小数点后没数字
        "[.5]",                              // 小数点前没数字
        "[1e]",                              // 指数没数字
        "[tru]",                             // 字面量写错
        "[NaN]",                             // 不是 JSON 的数
        "{\"a\": 1} {\"b\": 2}",             // 值之后还有东西
        "{} x",
        "{\"a\": 1 // 注释\n}",              // 注释
        "[\"\\x41\"]",                       // 认不出的转义
        "[\"\\udc00\"]",                     // 孤立的低代理
        "[\"\\ud83d\"]",                     // 高代理后面缺低代理
        "\xEF\xBB\xBF{}",                    // BOM：nlohmann 缺省会悄悄跳过，这里必须拦
    };
    for (const char* text : broken) {
        const auto meta = parseMapMeta(text);
        EXPECT_FALSE(meta.ok) << "竟然读成功了：" << text;
        EXPECT_NE(meta.error.find("第 "), std::string::npos) << "报错里没有位置：" << meta.error;
        EXPECT_NE(meta.error.find("meta.json"), std::string::npos) << meta.error;
        const auto index = parseSpriteIndex(text);
        EXPECT_FALSE(index.ok) << "竟然读成功了：" << text;
        EXPECT_NE(index.error.find("第 "), std::string::npos) << "报错里没有位置：" << index.error;
        EXPECT_NE(index.error.find("index.json"), std::string::npos) << index.error;
    }
}

TEST(VisualLoader, ReportsTheLineOfTheFirstError) {
    for (const char* text : {"{\n  \"time\": \"day\",\n  \"water\": [1, 2,]\n}", "{\n  \"a\": 1,\n  \"b\": ?\n}",
                             "{\n \"time\": \"night\",\n}"}) {
        const auto parsed = parseMapMeta(text);
        ASSERT_FALSE(parsed.ok) << text;
        EXPECT_NE(parsed.error.find("第 3 行"), std::string::npos) << parsed.error;
    }
}

TEST(VisualLoader, DeepNestingIsAnErrorNotACrash) {
    // 成千上万个 '[' 的坏文件只该换来一句报错，而不是把调用栈吃穿。
    EXPECT_FALSE(parseMapMeta(std::string(5000, '[')).ok);
    // 语法对、却深得离谱（藏在读取器不看的字段里也一样）：生成器写不出来，照样报。
    const std::string deep = "{\"note\": " + std::string(100, '[') + std::string(100, ']') + "}";
    const auto parsed = parseMapMeta(deep);
    EXPECT_FALSE(parsed.ok);
    EXPECT_NE(parsed.error.find("嵌套"), std::string::npos) << parsed.error;
    // 生成器写得出来的深度（四五层）照常能读；字符串里的括号（含转义的引号之后）不算层数。
    EXPECT_TRUE(parseMapMeta(R"({"note": [[[[[[[[1]]]]]]]]})").ok);
    const auto bracketsInText = parseMapMeta("{\"theme\": \"\\\"" + std::string(100, '[') + "\"}");
    EXPECT_TRUE(bracketsInText.ok) << bracketsInText.error;
}

TEST(VisualLoader, KeepsUtf8AndDecodesEveryEscape) {
    // 生成器用 ensure_ascii=False 写，中文是原样的 UTF-8；\u 转义也得认（人手改文件时常见）。
    const auto parsed = parseMapMeta(R"({"theme": "韩立（第 3 章起）", "lights": [
        {"x": 1, "y": 2, "kind": "\"\\\/\b\f\n\r\t"},
        {"x": 1, "y": 2, "kind": "\u4e2d\u00e9"},
        {"x": 1, "y": 2, "kind": "\ud83d\ude00"}]})");
    ASSERT_TRUE(parsed.ok) << parsed.error;
    EXPECT_EQ(parsed.value.theme, "韩立（第 3 章起）");
    ASSERT_EQ(parsed.value.lights.size(), 3u);
    EXPECT_EQ(parsed.value.lights[0].kind, std::string("\"\\/\b\f\n\r\t"));
    EXPECT_EQ(parsed.value.lights[1].kind, "中\xC3\xA9");
    // 代理对拼成一个码点，写成四字节 UTF-8，而不是两个三字节的半截字符。
    EXPECT_EQ(parsed.value.lights[2].kind, "\xF0\x9F\x98\x80");
}

TEST(VisualLoader, IntegersAreNumbersWithoutAFraction) {
    // JSON 的数不分整数与小数：1.6e1 恰好是 16，算整数；16.5、1e20（超出 int）、"16"（字符串）都不算。
    const auto ok = parseSpriteIndex(indexWithSheet(R"("frame_w": 1.6e1)"));
    ASSERT_TRUE(ok.ok) << ok.error;
    EXPECT_EQ(ok.value.sheets.at("a").frameW, 16);
    for (const char* bad : {R"("frame_w": 16.5)", R"("frame_w": 1e20)", R"("frame_w": "16")", R"("frame_w": true)"}) {
        const auto parsed = parseSpriteIndex(indexWithSheet(bad));
        EXPECT_FALSE(parsed.ok) << bad;
        EXPECT_NE(parsed.error.find("sheets.a.frame_w"), std::string::npos) << parsed.error;
    }
    // 小数照 JSON 的文法读：2E-2、1.5e-1、负零。
    const auto meta = parseMapMeta(R"({"dof": 2E-2, "bloom": 1.5e-1, "water": [[-0, 3]]})");
    ASSERT_TRUE(meta.ok) << meta.error;
    EXPECT_FLOAT_EQ(*meta.value.dof, 0.02f);
    EXPECT_FLOAT_EQ(*meta.value.bloom, 0.15f);
    ASSERT_EQ(meta.value.water.size(), 1u);
    EXPECT_EQ(meta.value.water[0].x, 0);
    // 溢出 double 的数：nlohmann 报 out_of_range 而不是语法错，也得是报错。
    EXPECT_FALSE(parseMapMeta(R"({"dof": 1e400})").ok);
}

TEST(VisualLoader, LaterDuplicateKeysWin) {
    // 与 nlohmann、原先的 game::VisualJson 同一个口径（本仓库的表里没有重复键，tools/validate.py 也拦着）。
    const auto parsed = parseMapMeta(R"({"time": "day", "time": "night"})");
    ASSERT_TRUE(parsed.ok) << parsed.error;
    EXPECT_EQ(parsed.value.time, TimeOfDay::Night);
}

TEST(VisualLoader, FileErrorsNameThePath) {
    const std::string missing = "no/such/dir/meta.json";
    const auto meta = fanren::io::loadMapMeta(missing);
    EXPECT_FALSE(meta.ok);
    EXPECT_NE(meta.error.find(missing), std::string::npos) << meta.error;
    const auto index = fanren::io::loadSpriteIndex("no/such/dir/index.json");
    EXPECT_FALSE(index.ok);
    EXPECT_NE(index.error.find("no/such/dir/index.json"), std::string::npos) << index.error;

    // 语法错的文件：报错里既有路径、也有行号。
    const fanren::test::TempDir dir("fanren_visual_loader");
    const fs::path bad = dir.path() / "index.json";
    {
        std::ofstream out(bad, std::ios::binary);
        out << "{\n \"sheets\": {},\n}";
    }
    const auto broken = fanren::io::loadSpriteIndex(bad.string());
    EXPECT_FALSE(broken.ok);
    EXPECT_NE(broken.error.find(bad.string()), std::string::npos) << broken.error;
    EXPECT_NE(broken.error.find("第 3 行"), std::string::npos) << broken.error;
}

// ---------------------------------------------------------------------------
// 战斗画面要的字段
// ---------------------------------------------------------------------------

constexpr const char* kBattleIndex = R"({
 "frame_w": 16, "frame_h": 24,
 "sheets": {
  "hero": {"file": "chars/hero.png", "walk": {"down": [0,1,2], "left": [3,4,5], "right": [6,7,8], "up": [9,10,11]},
           "battle": {"idle": [12, 13], "attack": [15]}, "battle_facing": "left", "weapon": "sword"},
  "plain": {"file": "chars/plain.png", "walk": {"down": [0,1,2], "left": [3,4,5], "right": [6,7,8], "up": [9,10,11]}}
 },
 "enemies": {
  "wolf": {"file": "enemies/wolf.png", "kind": "beast", "frame_w": 32, "cols": 5,
           "battle": {"idle": [0, 1], "down": [4]}, "battle_facing": "right", "foot_y": 22, "head_y": 3},
  "orb": {"file": "enemies/orb.png", "kind": "orb", "frame_w": 40, "frame_h": 40, "cols": 6,
          "battle": {"idle": [0, 1, 2, 3]}, "battle_facing": "none", "float": true},
  "wolf2": {"file": "enemies/wolf.png", "alias_of": "wolf", "battle": {"idle": [0]}}
 },
 "battle_overrides": {"b03": {"hero": "orb"}}
})";

TEST(VisualLoader, ReadsTheBattleFieldsOfTheSpriteIndex) {
    const auto parsed = parseSpriteIndex(kBattleIndex);
    ASSERT_TRUE(parsed.ok) << parsed.error;
    const auto& index = parsed.value;

    const auto& hero = index.sheets.at("hero");
    EXPECT_EQ(hero.battle.at("idle"), (std::vector<int>{12, 13}));
    EXPECT_EQ(hero.battle.at("attack"), (std::vector<int>{15}));
    EXPECT_EQ(hero.battleFacing, BattleFacing::Left);
    EXPECT_EQ(hero.weapon, "sword");
    const auto& plain = index.sheets.at("plain");
    EXPECT_TRUE(plain.battle.empty()) << "没写战斗帧的人物表照样收：行走画面不需要它";
    EXPECT_EQ(plain.battleFacing, BattleFacing::Left) << "人物表的战斗帧一律面向左（docs/art-sprites.md 第 4 节）";

    ASSERT_EQ(index.enemies.size(), 3u);
    const auto& wolf = index.enemies.at("wolf");
    EXPECT_EQ(wolf.file, "enemies/wolf.png");
    EXPECT_EQ(wolf.kind, "beast");
    EXPECT_EQ(wolf.frameW, 32);
    EXPECT_EQ(wolf.frameH, 24) << "帧高缺省取顶层";
    EXPECT_EQ(wolf.cols, 5);
    EXPECT_EQ(wolf.footY, 22) << "非人形的帧下面留空：脚底按写的来";
    EXPECT_EQ(wolf.headY, 3);
    EXPECT_EQ(wolf.battleFacing, BattleFacing::Right);
    EXPECT_FALSE(wolf.floating);
    const auto& orb = index.enemies.at("orb");
    EXPECT_TRUE(orb.floating);
    EXPECT_EQ(orb.battleFacing, BattleFacing::None);
    EXPECT_EQ(orb.footY, 39) << "foot_y 缺省是最后一行";
    const auto& alias = index.enemies.at("wolf2");
    EXPECT_EQ(alias.aliasOf, "wolf");
    EXPECT_EQ(alias.battleFacing, BattleFacing::Right) << "非人形敌人缺省面向右、不翻转";

    EXPECT_EQ(index.battleOverrides.at("b03").at("hero"), "orb");
}

TEST(VisualLoader, BrokenBattleFieldsAreErrorsThatSayWhere) {
    struct Case {
        std::string json;
        const char* mentions;   // 报错里必须点到的字段
    };
    const std::string sheets = R"("sheets": {})";
    const Case cases[] = {
        {indexWithSheet(R"("battle": [12])"), "sheets.a.battle"},
        {indexWithSheet(R"("battle": {"idle": []})"), "sheets.a.battle.idle"},
        {indexWithSheet(R"("battle": {"idle": [-1]})"), "sheets.a.battle.idle"},
        {indexWithSheet(R"("battle": {"idle": [1.5]})"), "sheets.a.battle.idle"},
        {indexWithSheet(R"("battle_facing": "west")"), "sheets.a.battle_facing"},
        {indexWithSheet(R"("weapon": 3)"), "sheets.a.weapon"},
        {"{" + sheets + R"(, "enemies": []})", "enemies"},
        {"{" + sheets + R"(, "enemies": {"x": {"battle": {"idle": [0]}}}})", "enemies.x.file"},
        {"{" + sheets + R"(, "enemies": {"x": {"file": "x.png"}}})", "enemies.x.battle"},
        {"{" + sheets + R"(, "enemies": {"x": {"file": "x.png", "battle": {"idle": [0]}, "float": "yes"}}})",
         "enemies.x.float"},
        {"{" + sheets + R"(, "enemies": {"x": {"file": "x.png", "battle": {"idle": [0]}, "frame_w": 0}}})",
         "enemies.x"},
        {"{" + sheets + R"(, "battle_overrides": {"b03": ["hanli"]}})", "battle_overrides.b03"},
        {"{" + sheets + R"(, "battle_overrides": {"b03": {"hanli": 3}}})", "battle_overrides.b03.hanli"},
    };
    for (const Case& c : cases) {
        const auto parsed = parseSpriteIndex(c.json);
        EXPECT_FALSE(parsed.ok) << "竟然读成功了：" << c.json;
        EXPECT_NE(parsed.error.find(c.mentions), std::string::npos)
            << c.json << " 的报错没点到 " << c.mentions << "：" << parsed.error;
    }
    const auto backdrop = parseMapMeta(R"({"backdrop": 3})");
    EXPECT_FALSE(backdrop.ok);
    EXPECT_NE(backdrop.error.find("backdrop"), std::string::npos) << backdrop.error;
}

TEST(VisualLoader, TheRealIndexCarriesWhatTheBattleScreenNeeds) {
    // 判据抄 docs/art-sprites.md 第 4–5 节：人物表的战斗帧一律面向左、每张都有待机帧；
    // 识海之战（b03_shihai_duoshe）里韩立换成光球 hanli_yuanshen，光球悬空、不分朝向。
    const fs::path path = repoRoot() / "assets" / "art" / "sprites" / "index.json";
    if (!fs::exists(path)) GTEST_SKIP() << "仓库里还没有 " << path.string();
    const auto loaded = fanren::io::loadSpriteIndex(path.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const auto& index = loaded.value;
    for (const auto& [look, sheet] : index.sheets) {
        EXPECT_EQ(sheet.battleFacing, BattleFacing::Left) << look;
        EXPECT_EQ(sheet.battle.count("idle"), 1u) << look;
    }
    ASSERT_EQ(index.battleOverrides.count("b03_shihai_duoshe"), 1u);
    const std::string& soul = index.battleOverrides.at("b03_shihai_duoshe").at("hanli");
    EXPECT_EQ(soul, "hanli_yuanshen");
    ASSERT_EQ(index.enemies.count(soul), 1u) << "覆写成的外观得真有图";
    EXPECT_TRUE(index.enemies.at(soul).floating);
    EXPECT_EQ(index.enemies.at(soul).battleFacing, BattleFacing::None);
    EXPECT_EQ(index.enemies.at("wild_wolf").battleFacing, BattleFacing::Right);
}

TEST(VisualLoader, EveryBakedMetaNamesABattleBackdropThatExists) {
    // A1 与战斗画面的契约：每张烘焙过的地图都说得出在它上面开打用哪一套战斗背景，
    // 而那一套的四层图（docs/art-maps.md：sky / far / mid / ground）真在。
    const fs::path maps = repoRoot() / "assets" / "art" / "maps";
    int seen = 0;
    if (fs::exists(maps)) {
        for (const fs::directory_entry& entry : fs::directory_iterator(maps)) {
            const fs::path meta = entry.path() / "meta.json";
            if (!fs::exists(meta)) continue;
            ++seen;
            const auto loaded = fanren::io::loadMapMeta(meta.string());
            ASSERT_TRUE(loaded.ok) << loaded.error;
            const std::string& backdrop = loaded.value.backdrop;
            EXPECT_FALSE(backdrop.empty()) << meta.string();
            const fs::path dir = repoRoot() / "assets" / "art" / "battle" / backdrop;
            for (const char* layer : {"sky", "far", "mid", "ground"}) {
                const fs::path png = dir / (std::string(layer) + ".png");
                EXPECT_TRUE(fs::exists(png)) << meta.string() << " → " << png.string();
            }
        }
    }
    if (seen == 0) GTEST_SKIP() << "仓库里还没有烘焙好的 meta.json（" << maps.string() << "）";
}

// ---- data/visual/battles.json：战斗背景的人工指派 ----
//
// 这张表合入战斗画面时一度没人读，运行时按所在地图的 meta 取背景，19 场里 11 场取错：
// 攻防战在夜里的演武场却是白天，墨府尸傀在地窖却是墨府夜景。

TEST(BattleBackdrops, ReadsOnlyTheBackdropOfEachBattle) {
    const auto table = fanren::io::parseBattleBackdrops(
        R"({"_doc": "x", "battles": {"b1": {"backdrop": "cave_tunnel", "map": "m", "why": "w"},
                                     "b2": {"backdrop": "soulsea"}}})");
    ASSERT_TRUE(table) << table.error;
    ASSERT_EQ(table.value.size(), 2u);
    EXPECT_EQ(table.value.at("b1"), "cave_tunnel");
    EXPECT_EQ(table.value.at("b2"), "soulsea");
}

TEST(BattleBackdrops, ABrokenTableIsAnErrorNotAnEmptyOne) {
    for (const char* bad : {R"([])", R"({"_doc": "x"})", R"({"battles": []})",
                            R"({"battles": {"b1": {"map": "m"}}})", R"({"battles": {"b1": {"backdrop": ""}}})",
                            R"({"battles": {"b1": {"backdrop": 3}}})", R"({"battles": {"b1": "cave"}})"}) {
        const auto table = fanren::io::parseBattleBackdrops(bad);
        EXPECT_FALSE(table) << bad;
        if (!table) EXPECT_NE(table.error.find("battles.json"), std::string::npos) << table.error;
    }
}

// 判据写死成 docs/art-maps.md 第 8 节那张表的原文，不从被测文件推：地窖里的尸傀用密室、
// 夜里的攻防用夜版校场、外刃堂的切磋用白天的沙地校场、庄外林子的遭遇用庄外夜景。
TEST(BattleBackdrops, TheRealTableAssignsTheDocumentedBackdrops) {
    const fs::path path = repoRoot() / "data" / "visual" / "battles.json";
    const auto table = fanren::io::loadBattleBackdrops(path.string());
    ASSERT_TRUE(table) << table.error;
    EXPECT_EQ(table.value.at("b05_mofu_shigui"), "secret_room");
    EXPECT_EQ(table.value.at("b04_gongfang_weijian"), "drill_ground_night");
    EXPECT_EQ(table.value.at("b04_qiecuo_maliu"), "drill_ground");
    EXPECT_EQ(table.value.at("be05_linzi_yezhu"), "wild_manor");
    // 每一个指派都真有那套四层图。
    for (const auto& [battle, backdrop] : table.value) {
        EXPECT_TRUE(fs::exists(repoRoot() / "assets" / "art" / "battle" / backdrop / "ground.png"))
            << battle << " → " << backdrop;
    }
}

}  // namespace
