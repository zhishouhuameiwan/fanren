// 伙伴改版·底座（docs/interfaces-companions.md 第 6 节「第一步」）：数据这一半。
//   1. 存档 v10：新字段往返、旧档全给空、写坏了判失败、全部交接档读进来再写出逐字段不变；
//   2. 伙伴数据：加载器的形状、两个查询函数、默契的档；
//   3. 人物表按表的新字段（scale / walk_cycle / battle_file / battle_scale）与 hd/index.json 覆盖。
// 走 Application 真入口的那一半在 CompanionsBaseWiringTests.cpp。
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../vendor/json.hpp"

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/model/Visual.h"
#include "core/rules/Companions.h"
#include "game/SpriteAtlas.h"
#include "io/CompanionLoader.h"
#include "io/SaveFile.h"
#include "io/VisualLoader.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::BondRecord;
using fanren::core::Companion;
using fanren::core::GameState;
using nlohmann::json;

fs::path repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "tests" / "fixtures" / "ch13-end-first.sav")) return candidate;
    }
    return ".";
}

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

void writeFile(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

// v10 新加的五个键。判据照抄契约 2.3，不从 SaveFile.cpp 里读。
const std::set<std::string> kNewKeys = {"bonds", "openedChests", "gatheredDay", "clearedFoes", "openedGates"};

// ---------------------------------------------------------------------------
// 1. 存档 v10
// ---------------------------------------------------------------------------

TEST(CompanionsSave, TheCurrentVersionIsTen) {
    // 契约 2.3：「存档 v10」。写死 10，不写 kSaveVersion（那是恒真）。
    EXPECT_EQ(fanren::io::kSaveVersion, 10);
}

TEST(CompanionsSave, BondsAndWorldObjectsRoundTripThroughTheSaveFile) {
    fanren::test::TempDir dir("fanren_companions_save");
    GameState state;
    state.mapId = "ch07_xuese";
    state.bonds["nangong_wan"] = BondRecord{47, 12, 4, 11};
    state.bonds["chen_shimei"] = BondRecord{5, 0, 0, 9};
    state.worldObjects.openedChests = {"ch07_xuese.chest_a", "ch07_xuese.chest_b"};
    state.worldObjects.gatheredDay = {{"ch07_xuese.herb_a", 10}};
    state.worldObjects.clearedFoes = {{"ch07_xuese.foe_wolf", 12}};
    state.worldObjects.openedGates = {"ch07_xuese.gate_seal"};
    const fs::path path = dir.path() / "v10.sav";
    ASSERT_TRUE(fanren::io::saveGame(state, path.string()).ok);

    const json root = json::parse(readFile(path));
    EXPECT_EQ(root.at("save_version"), 10);
    const json& p = root.at("payload");
    EXPECT_EQ(p.at("bonds").at("nangong_wan").at("value"), 47);
    EXPECT_EQ(p.at("openedChests"), json({"ch07_xuese.chest_a", "ch07_xuese.chest_b"}));

    const auto back = fanren::io::loadGame(path.string());
    ASSERT_TRUE(back.ok) << back.error;
    const BondRecord& nw = back.value.bonds.at("nangong_wan");
    EXPECT_EQ(nw.value, 47);
    EXPECT_EQ(nw.battleDay, 12);
    EXPECT_EQ(nw.battleGainedThatDay, 4);
    EXPECT_EQ(nw.giftDay, 11);
    EXPECT_EQ(back.value.bonds.at("chen_shimei").giftDay, 9);
    EXPECT_EQ(back.value.bondOf("chen_shimei"), 5);
    EXPECT_EQ(back.value.bondOf("nobody"), 0);
    EXPECT_EQ(back.value.worldObjects.openedChests, state.worldObjects.openedChests);
    EXPECT_EQ(back.value.worldObjects.gatheredDay, state.worldObjects.gatheredDay);
    EXPECT_EQ(back.value.worldObjects.clearedFoes, state.worldObjects.clearedFoes);
    EXPECT_EQ(back.value.worldObjects.openedGates, state.worldObjects.openedGates);
}

// [配对的负向] 两份只差新字段的档，读回来必须不同——少了这一条，上面那串相等可能只是两边都是缺省。
TEST(CompanionsSave, TwoSavesThatDifferOnlyInTheNewFieldsReadBackDifferently) {
    fanren::test::TempDir dir("fanren_companions_save_neg");
    GameState a;
    a.mapId = "m";
    a.bonds["x"].value = 3;
    GameState b = a;
    b.bonds["x"].value = 4;
    b.worldObjects.openedChests = {"m.chest"};
    ASSERT_TRUE(fanren::io::saveGame(a, (dir.path() / "a.sav").string()).ok);
    ASSERT_TRUE(fanren::io::saveGame(b, (dir.path() / "b.sav").string()).ok);
    const auto backA = fanren::io::loadGame((dir.path() / "a.sav").string());
    const auto backB = fanren::io::loadGame((dir.path() / "b.sav").string());
    ASSERT_TRUE(backA.ok && backB.ok);
    EXPECT_EQ(backA.value.bondOf("x"), 3);
    EXPECT_EQ(backB.value.bondOf("x"), 4);
    EXPECT_TRUE(backA.value.worldObjects.openedChests.empty());
    EXPECT_EQ(backB.value.worldObjects.openedChests.count("m.chest"), 1u);
}

// 带正确校验和地写一份 v10 存档，payload 由调用方给（写坏新字段用）。
fs::path writeV10(const fanren::test::TempDir& dir, const json& payload) {
    const auto path = dir.path() / "hand.sav";
    // 照 SaveFile.cpp 的公式复算校验和：16 位十六进制的 fnv1a64("version|payload.dump()")。
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char c : "10|" + payload.dump()) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 0x100000001b3ULL;
    }
    std::ostringstream sum;
    sum << std::hex << std::setw(16) << std::setfill('0') << hash;
    writeFile(path, json{{"save_version", 10}, {"checksum", sum.str()}, {"payload", payload}}.dump());
    return path;
}

json validPayload() {
    fanren::test::TempDir dir("fanren_companions_payload");
    GameState state;
    state.mapId = "m";
    const auto path = dir.path() / "s.sav";
    EXPECT_TRUE(fanren::io::saveGame(state, path.string()).ok);
    return json::parse(readFile(path)).at("payload");
}

TEST(CompanionsSave, AHandWrittenValidPayloadIsAccepted) {
    // 下面那几条判失败的前提：同一套写法、新字段写对了就读得进。
    fanren::test::TempDir dir("fanren_companions_ok");
    json p = validPayload();
    p["bonds"] = {{"x", {{"value", 100}, {"battleDay", 1}, {"battleGained", 6}, {"giftDay", 0}}}};
    const auto back = fanren::io::loadGame(writeV10(dir, p).string());
    ASSERT_TRUE(back.ok) << back.error;
    EXPECT_EQ(back.value.bondOf("x"), 100);
}

TEST(CompanionsSave, MalformedNewFieldsAreRejectedNotDropped) {
    const std::vector<std::pair<const char*, json>> broken = {
        {"bonds", json::array()},
        {"bonds", {{"x", {{"value", 101}, {"battleDay", 0}, {"battleGained", 0}, {"giftDay", 0}}}}},
        {"bonds", {{"x", {{"value", -1}, {"battleDay", 0}, {"battleGained", 0}, {"giftDay", 0}}}}},
        {"bonds", {{"x", {{"value", "5"}, {"battleDay", 0}, {"battleGained", 0}, {"giftDay", 0}}}}},
        {"bonds", {{"x", {{"value", 5}}}}},
        {"openedChests", "a"},
        {"openedChests", {1, 2}},
        {"gatheredDay", {{"a", "3"}}},
        {"clearedFoes", json::array()},
        {"openedGates", {{"a", 1}}},
    };
    for (const auto& [key, value] : broken) {
        fanren::test::TempDir dir("fanren_companions_bad");
        json p = validPayload();
        p[key] = value;
        const auto back = fanren::io::loadGame(writeV10(dir, p).string());
        EXPECT_FALSE(back.ok) << key << " = " << value.dump();
    }
}

// 契约 5：「v9 的全部交接档读进 v10、写出、再读回，除新增字段外逐字段相同」。
// 第 3–8 章的档是 v8（formerRealm 是 v9 新加的，那一项与 v10 的五项一样算「新增」）。
TEST(CompanionsSave, EveryHandoverFixtureReadsIntoV10AndWritesBackFieldForField) {
    std::vector<fs::path> fixtures;
    for (const auto& entry : fs::directory_iterator(repoRoot() / "tests" / "fixtures")) {
        const std::string name = entry.path().filename().string();
        if (name.rfind("ch", 0) == 0 && entry.path().extension() == ".sav") fixtures.push_back(entry.path());
    }
    ASSERT_EQ(fixtures.size(), 22u) << "第 3–13 章各两份";
    fanren::test::TempDir dir("fanren_companions_fixtures");
    for (const fs::path& fixture : fixtures) {
        const std::string label = fixture.filename().string();
        const json original = json::parse(readFile(fixture));
        const int version = original.at("save_version").get<int>();
        ASSERT_TRUE(version == 8 || version == 9) << label;
        const json& before = original.at("payload");

        const auto loaded = fanren::io::loadGame(fixture.string());
        ASSERT_TRUE(loaded.ok) << label << ": " << loaded.error;
        EXPECT_TRUE(loaded.value.bonds.empty()) << label;
        EXPECT_TRUE(loaded.value.worldObjects.empty()) << label;

        const fs::path first = dir.path() / (label + ".v10");
        ASSERT_TRUE(fanren::io::saveGame(loaded.value, first.string()).ok) << label;
        const json written = json::parse(readFile(first));
        EXPECT_EQ(written.at("save_version"), 10) << label;
        const json& after = written.at("payload");

        std::set<std::string> added = kNewKeys;
        if (version == 8) added.insert("formerRealm");
        for (const auto& [key, value] : before.items()) {
            ASSERT_TRUE(after.contains(key)) << label << " 写出时丢了 " << key;
            EXPECT_EQ(after.at(key), value) << label << " 的 " << key << " 变了";
        }
        for (const auto& [key, value] : after.items()) {
            if (before.contains(key)) continue;
            EXPECT_TRUE(added.count(key) == 1) << label << " 多出一项不是新增字段的 " << key;
        }
        for (const std::string& key : kNewKeys) {
            ASSERT_TRUE(after.contains(key)) << label << " 没写 " << key;
            EXPECT_TRUE(after.at(key).empty()) << label << " 的 " << key << " 不是空的";
        }

        // 再读回、再写出：第二份与第一份逐字节相同。
        const auto again = fanren::io::loadGame(first.string());
        ASSERT_TRUE(again.ok) << label << ": " << again.error;
        const fs::path second = dir.path() / (label + ".v10b");
        ASSERT_TRUE(fanren::io::saveGame(again.value, second.string()).ok) << label;
        EXPECT_EQ(readFile(second), readFile(first)) << label;
    }
}

// ---------------------------------------------------------------------------
// 2. 伙伴数据
// ---------------------------------------------------------------------------

constexpr const char* kNangong = R"({
  "id": "nangong_wan",
  "roles": [{"role": "baiyi_shaonv", "until_flag": "ch07.nangong"}, {"role": "nangong_wan_ch07"},
            {"role": "nangong_wan"}],
  "met_flag": "ch07.nangong", "field_skill": "break_seal", "chase": true,
  "duo": "duo_nangong_zhuque", "gifts": {"like": ["herb_a"], "dislike": ["herb_b"]}, "note": "ch07"
})";

TEST(CompanionLoader, ReadsEveryFieldOfTheContractExample) {
    fanren::test::TempDir dir("fanren_companions_loader");
    writeFile(dir.path() / "nangong_wan.json", kNangong);
    const auto loaded = fanren::io::loadCompanions(dir.path().string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_EQ(loaded.value.size(), 1u);
    const Companion& c = loaded.value.at("nangong_wan");
    ASSERT_EQ(c.roles.size(), 3u);
    EXPECT_EQ(c.roles[0].roleId, "baiyi_shaonv");
    EXPECT_EQ(c.roles[0].untilFlag, "ch07.nangong");
    EXPECT_TRUE(c.roles[2].untilFlag.empty());
    EXPECT_EQ(c.metFlag, "ch07.nangong");
    EXPECT_EQ(c.fieldSkill, "break_seal");
    EXPECT_TRUE(c.chase);
    EXPECT_EQ(c.duo, "duo_nangong_zhuque");
    EXPECT_EQ(c.giftsLike, std::vector<std::string>{"herb_a"});
    EXPECT_EQ(c.giftsDislike, std::vector<std::string>{"herb_b"});
}

TEST(CompanionLoader, AMissingDirectoryMeansNoCompanions) {
    const auto loaded = fanren::io::loadCompanions((fs::temp_directory_path() / "fanren_no_such_dir_x").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_TRUE(loaded.value.empty());
}

TEST(CompanionLoader, RejectsBadShapes) {
    const char* broken[] = {
        R"({"id": "a", "roles": [{"role": "r"}]})",                                   // 缺 met_flag
        R"({"id": "a", "met_flag": "f"})",                                            // 缺 roles
        R"({"id": "a", "met_flag": "f", "roles": []})",                               // roles 空
        R"({"id": "a", "met_flag": "f", "roles": [{"until_flag": "x"}]})",            // 缺 role
        R"({"id": "a", "met_flag": "f", "roles": [{"role": "r", "until": "x"}]})",    // 拼错 until_flag
        R"({"id": "a", "met_flag": "f", "roles": [{"role": "r"}], "chase": "yes"})",  // chase 不是布尔
        R"({"id": "a", "met_flag": "f", "roles": [{"role": "r"}], "romance": 1})",    // 表外字段
        R"({"id": "a", "met_flag": "f", "roles": [{"role": "r"}], "gifts": {"love": []}})",
    };
    for (const char* text : broken) {
        fanren::test::TempDir dir("fanren_companions_badshape");
        writeFile(dir.path() / "a.json", text);
        EXPECT_FALSE(fanren::io::loadCompanions(dir.path().string()).ok) << text;
    }
}

TEST(CompanionLoader, OneRoleBelongsToAtMostOneCompanion) {
    fanren::test::TempDir dir("fanren_companions_dup");
    writeFile(dir.path() / "a.json", R"({"id": "a", "met_flag": "f", "roles": [{"role": "shared"}]})");
    writeFile(dir.path() / "b.json", R"({"id": "b", "met_flag": "f", "roles": [{"role": "shared"}]})");
    const auto loaded = fanren::io::loadCompanions(dir.path().string());
    ASSERT_FALSE(loaded.ok);
    EXPECT_NE(loaded.error.find("shared"), std::string::npos) << loaded.error;
}

std::map<std::string, Companion> nangongOnly() {
    fanren::test::TempDir dir("fanren_companions_rules");
    writeFile(dir.path() / "n.json", kNangong);
    return fanren::io::loadCompanions(dir.path().string()).value;
}

TEST(CompanionRules, CompanionOfRoleAnswersForEveryRoleSheUsedAndNobodyElse) {
    const auto companions = nangongOnly();
    EXPECT_EQ(fanren::rules::companionOfRole(companions, "baiyi_shaonv"), "nangong_wan");
    EXPECT_EQ(fanren::rules::companionOfRole(companions, "nangong_wan_ch07"), "nangong_wan");
    EXPECT_EQ(fanren::rules::companionOfRole(companions, "nangong_wan"), "nangong_wan");
    EXPECT_EQ(fanren::rules::companionOfRole(companions, "kuilei"), "") << "傀儡不是伙伴";
    EXPECT_EQ(fanren::rules::companionOfRole(companions, ""), "");
}

TEST(CompanionRules, CurrentRoleIsTheFirstWhoseUntilFlagIsNotSet) {
    const auto companions = nangongOnly();
    GameState state;
    EXPECT_EQ(fanren::rules::currentRole(companions, "nangong_wan", state), "baiyi_shaonv");
    state.setFlag("ch07.nangong");
    EXPECT_EQ(fanren::rules::currentRole(companions, "nangong_wan", state), "nangong_wan_ch07");
    EXPECT_EQ(fanren::rules::currentRole(companions, "nobody", state), "");
}

TEST(BondRules, TiersFollowTheContractThresholds) {
    // 契约 2.2：0 / 20 / 45 / 70 / 95。
    const std::vector<std::pair<int, int>> cases = {{0, 1},  {19, 1}, {20, 2}, {44, 2}, {45, 3},
                                                    {69, 3}, {70, 4}, {94, 4}, {95, 5}, {100, 5}};
    for (const auto& [value, tier] : cases) EXPECT_EQ(fanren::rules::bondTier(value), tier) << value;
}

TEST(BondRules, AddingOnlyRaisesAndStopsAtAHundred) {
    BondRecord r;
    EXPECT_TRUE(fanren::rules::addBond(r, 30));
    EXPECT_EQ(r.value, 30);
    EXPECT_FALSE(fanren::rules::addBond(r, -5)) << "只增不减";
    EXPECT_EQ(r.value, 30);
    EXPECT_TRUE(fanren::rules::addBond(r, 1000));
    EXPECT_EQ(r.value, 100);
}

// ---------------------------------------------------------------------------
// 3. 人物表按表的字段与 hd/index.json
// ---------------------------------------------------------------------------

constexpr const char* kWalk3 = R"("walk": {"down": [0,1,2], "left": [3,4,5], "right": [6,7,8], "up": [9,10,11]})";
constexpr const char* kWalk4 =
    R"("walk": {"down": [0,1,2,3], "left": [4,5,6,7], "right": [8,9,10,11], "up": [12,13,14,15]})";

std::string baseIndex() {
    return std::string(R"({"walk_cycle": [0, 1, 0, 2], "walk_fps": 8, "sheets": {"boy": {"file": "boy.png", )") +
           kWalk3 + R"(}}, "roles": {"hanli": "boy", "zhang": "boy"},
           "role_variants": {"hanli": [{"look": "boy", "max_chapter": 2}]}})";
}

TEST(SheetFields, OldSheetsKeepTodaysBehaviourByDefault) {
    const auto index = fanren::io::parseSpriteIndex(baseIndex());
    ASSERT_TRUE(index.ok) << index.error;
    const fanren::core::CharacterSheet& boy = index.value.sheets.at("boy");
    EXPECT_EQ(boy.scale, 3);
    EXPECT_EQ(boy.walkCycle, (std::vector<int>{0, 1, 0, 2}));
    EXPECT_TRUE(boy.battleFile.empty());
    EXPECT_EQ(boy.battleScale, 4);
}

TEST(SheetFields, EachSheetMayCarryItsOwnScaleCycleAndBattleImage) {
    const std::string text = std::string(R"({"sheets": {"hd": {"file": "hd.png", "frame_w": 40, "frame_h": 48, "cols": 4,
        "scale": 2, "walk_cycle": [0, 1, 2, 3], "battle_file": "hd_battle.png", "battle_scale": 1, )") +
                             kWalk4 + "}}}";
    const auto index = fanren::io::parseSpriteIndex(text);
    ASSERT_TRUE(index.ok) << index.error;
    const fanren::core::CharacterSheet& hd = index.value.sheets.at("hd");
    EXPECT_EQ(hd.scale, 2);
    EXPECT_EQ(hd.walkCycle, (std::vector<int>{0, 1, 2, 3}));
    EXPECT_EQ(hd.battleFile, "hd_battle.png");
    EXPECT_EQ(hd.battleScale, 1);
    EXPECT_EQ(index.value.walkCycle, (std::vector<int>{0, 1, 0, 2})) << "全局那一份不受影响";
}

TEST(SheetFields, RejectsACycleLongerThanItsOwnSheetAndNonPositiveScales) {
    const std::string bad[] = {
        std::string(R"({"sheets": {"a": {"file": "a.png", "walk_cycle": [0, 1, 2, 3], )") + kWalk3 + "}}}",
        std::string(R"({"sheets": {"a": {"file": "a.png", "walk_cycle": [], )") + kWalk3 + "}}}",
        std::string(R"({"sheets": {"a": {"file": "a.png", "scale": 0, )") + kWalk3 + "}}}",
        std::string(R"({"sheets": {"a": {"file": "a.png", "battle_scale": -1, )") + kWalk3 + "}}}",
        std::string(R"({"sheets": {"a": {"file": "a.png", "battle_file": "", )") + kWalk3 + "}}}",
    };
    for (const std::string& text : bad) EXPECT_FALSE(fanren::io::parseSpriteIndex(text).ok) << text;
}

TEST(SheetFields, WalkFramePlaysEachSheetsOwnCycle) {
    const std::string text = std::string(R"({"walk_fps": 8, "sheets": {"boy": {"file": "b.png", )") + kWalk3 +
                             R"(}, "hd": {"file": "h.png", "walk_cycle": [0, 1, 2, 3], )" + kWalk4 + "}}}";
    const auto index = fanren::io::parseSpriteIndex(text);
    ASSERT_TRUE(index.ok) << index.error;
    const float step = 1.f / 8.f;
    const auto& boy = index.value.sheets.at("boy");
    const auto& hd = index.value.sheets.at("hd");
    // 第 4 拍（下标 3）：旧表播的是「迈乙」（walk[2] 的第 2 帧），高清表播第 3 帧。
    EXPECT_EQ(fanren::game::walkFrame(index.value, boy, 2, true, step * 3.5f), 2);
    EXPECT_EQ(fanren::game::walkFrame(index.value, hd, 2, true, step * 3.5f), 3);
    EXPECT_EQ(fanren::game::walkFrame(index.value, hd, 2, true, step * 2.5f), 2);
    EXPECT_EQ(fanren::game::walkFrame(index.value, boy, 2, true, step * 2.5f), 0);
}

// hd/index.json 与 index.json 放在一起的那种目录；返回 index.json 的路径。
fs::path spritesDir(const fanren::test::TempDir& dir, const std::string& hd) {
    const fs::path sprites = dir.path() / "art" / "sprites";
    writeFile(sprites / "index.json", baseIndex());
    if (!hd.empty()) writeFile(sprites / "hd" / "index.json", hd);
    return sprites / "index.json";
}

const std::string kHd = std::string(R"({"sheets": {"hanli_hd": {"file": "hanli.png", "frame_w": 40, "frame_h": 48,
    "cols": 4, "scale": 2, "walk_cycle": [0, 1, 2, 3], "battle_file": "hanli_battle.png", )") + kWalk4 + R"(}},
    "roles": {"hanli": "hanli_hd"}, "files": {"hanli.png": "00"}})";

TEST(HdIndex, WithoutTheFileNothingChanges) {
    fanren::test::TempDir dir("fanren_hd_none");
    const auto loaded = fanren::io::loadSpriteIndex(spritesDir(dir, "").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const auto parsed = fanren::io::parseSpriteIndex(baseIndex());
    EXPECT_EQ(loaded.value.roles, parsed.value.roles);
    EXPECT_EQ(loaded.value.sheets.size(), parsed.value.sheets.size());
    EXPECT_EQ(loaded.value.variants.count("hanli"), 1u);
}

TEST(HdIndex, OverridesTheRolesItNamesAndOnlyThose) {
    fanren::test::TempDir dir("fanren_hd_override");
    const auto loaded = fanren::io::loadSpriteIndex(spritesDir(dir, kHd).string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const auto never = [](int) { return false; };
    const fanren::core::CharacterSheet* hanli = fanren::game::sheetForRole(loaded.value, "hanli", never);
    ASSERT_NE(hanli, nullptr);
    EXPECT_EQ(hanli->file, "hd/hanli.png") << "文件名照旧相对 art/sprites/";
    EXPECT_EQ(hanli->scale, 2);
    EXPECT_EQ(hanli->battleFile, "hd/hanli_battle.png") << "战斗图同样补 hd/ 前缀";
    EXPECT_EQ(loaded.value.variants.count("hanli"), 0u) << "同名角色整个由手绘表接管，生成的按章变体不再作数";
    const fanren::core::CharacterSheet* zhang = fanren::game::sheetForRole(loaded.value, "zhang", never);
    ASSERT_NE(zhang, nullptr);
    EXPECT_EQ(zhang->file, "boy.png");
    // 没点名的角色一个不动：除了 hanli，roles 与不带 hd 时逐项相同。
    auto expected = fanren::io::parseSpriteIndex(baseIndex()).value.roles;
    expected["hanli"] = "hanli_hd";
    EXPECT_EQ(loaded.value.roles, expected);
}

TEST(HdIndex, ABrokenHdIndexFailsTheWholeLoad) {
    const std::string bad[] = {
        "{",                                                                               // 语法错
        R"({"roles": {"hanli": "nope"}, "sheets": {}})",                                    // 角色指向没有的表
        std::string(R"({"sheets": {"boy": {"file": "x.png", )") + kWalk3 + R"(}}, "roles": {}})",  // 与生成的表同名
    };
    for (const std::string& hd : bad) {
        fanren::test::TempDir dir("fanren_hd_bad");
        const auto loaded = fanren::io::loadSpriteIndex(spritesDir(dir, hd).string());
        EXPECT_FALSE(loaded.ok) << hd;
        if (!loaded.ok) EXPECT_NE(loaded.error.find("hd"), std::string::npos) << loaded.error;
    }
}

TEST(HdIndex, TheRealIndexStillLoadsWithEveryOldSheetOnTheOldDefaults) {
    const auto loaded = fanren::io::loadSpriteIndex((repoRoot() / "assets/art/sprites/index.json").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_FALSE(loaded.value.sheets.empty());
    for (const auto& [id, sheet] : loaded.value.sheets) {
        EXPECT_EQ(sheet.scale, 3) << id;
        EXPECT_EQ(sheet.walkCycle, loaded.value.walkCycle) << id;
        EXPECT_TRUE(sheet.battleFile.empty()) << id;
        EXPECT_EQ(sheet.battleScale, 4) << id;
    }
}

}  // namespace
