// 第 7 章五张必打编成、三张遭遇编成与每个人的数——直接读 data，判据抄自施工图 8.2 原文（docs/ch07-design.md 验收 4），
// 外加「那只手在意图玩家的局面下打不打得赢」（验收 4 末句「③ 在背包没有天雷子时那只手也赢得了（平衡路实测）」）。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件（第 4 章独立校对 MEDIUM-1）
// ---------------------------------------------------------------------------
// 那些数被**用**着（通关测试要靠它们才走得完），却从来没有被**断言**过，改了也没人知道。本文件与
// tests/Ch07AcceptanceTests.cpp 的通关测试刻意不同形状：通关测试问「这一章走不走得完」，这里问「这几样是不是
// 施工图写的那几样」。逐张按 id 钉（8.2 那张表点名写死的），不写「所有的 b07_* 都……」。
// 8.2 只写意图与破绽，**血攻防速、蓄势倍率之外的数值归平衡路**（8.2 表下第二条）——那几样这里一个也不判；
// 必须守住的只有 8.2 写死的：陆师兄 maxHp 180（锚点）、⑤ 的 killable_by、各场 defeat_is_fatal。
//
// 「那只手打不打得赢」那一组是**切片**：不走通关，按施工图 8.1 的意图玩家在每一场开打时身上该有的东西摆局面
// （第 6 章交接存档第二侧——省的那一侧——加上本章到那一场为止给的，写在 intentAt 里），满血满法力，
// 用 tests/BattleHand.h 的意图玩家那只手打。它量的是「这一场在意图玩家手里赢不赢得了」，不是通关测试那一趟的血线。
//
// 平衡路（2026-09-29）在这一组里补的：每一场量回合、血、法力、吃了几瓶、一回合挨得最重的一轮（Result）；
// 协调者裁决（施工图 16.5 末行）的两条——② ③ 要吃药、① ④ ⑤ 不许一回合打完；7.3 斩杀线口径用到五场；
// ③ 天雷子是省力（掷了回合更少、药不多吃）、② ③ 用不用符都赢；环形山三张遭遇一瓶不吃。
// 数与量法记在 docs/ch07-balance.md。set FANREN_CH07_BATTLE_LOG=1 再跑，打印每一场的出手记录。
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "BattleHand.h"
#include "ChapterFixture.h"
#include "core/battle/Battle.h"
#include "core/model/AttackCategory.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "io/SaveFile.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::BattleSetup;
using fanren::core::BattleUnitSpec;
using fanren::core::GameState;
using fanren::core::RoleTemplate;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::Unit;
using fanren::game::Application;
using fanren::game::BattleScene;
using fanren::rules::Realm;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "battles" / "b07_yixiantian.json")) return candidate;
    }
    return ".";
}

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

int cats(std::initializer_list<const char*> names) {
    int mask = 0;
    for (const char* name : names) mask |= fanren::core::categoryFromName(name);
    return mask;
}

// ---------------------------------------------------------------------------
// 判据一：施工图 8.2 那张表（一行一张编成）＋ 8.2 表下「战斗背景」
// ---------------------------------------------------------------------------
struct CompositionSpec {
    const char* mark;
    const char* id;
    std::multiset<std::string> allies;
    std::multiset<std::string> enemies;
    bool canEscape;
    bool defeatIsFatal;
    const char* backdrop;
};

const std::vector<CompositionSpec>& designSaysSo() {
    static const std::vector<CompositionSpec> kSpecs = {
        {"①", "b07_lu_shixiong", {}, {"lu_shixiong"}, false, true, "mountain_forest_dusk"},
        {"②", "b07_yixiantian", {}, {"luosai_huzi", "yan_xiongdi", "zichi_bishe", "chuanshan_shou"}, false, true,
         "cliff_top"},
        {"③", "b07_fengyue", {}, {"feng_yue"}, false, true, "mountain_forest"},
        {"④", "b07_zhongxinqu_duoyao", {}, {"yan_wuchi"}, false, true, "mountain_forest"},
        {"⑤", "b07_zhaoze_shouyao", {"baiyi_shaonv"}, {"mo_jiao"}, false, true, "cave_tunnel"},
        // 可选：环形山遭遇（可逃、不致命），be07_tiebi_yuan 兼作 Z2 东坡那一场。
        {"可选·推山兽", "be07_tuishan_shou", {}, {"tuishan_shou"}, true, false, "mountain_forest"},
        {"可选·铁臂猿", "be07_tiebi_yuan", {}, {"tiebi_yuan", "tiebi_yuan"}, true, false, "mountain_forest"},
        {"可选·火焰鼠", "be07_huoyan_shu", {}, {"huoyan_shu", "huoyan_shu", "huoyan_shu"}, true, false, "mountain_forest"},
    };
    return kSpecs;
}

// ---------------------------------------------------------------------------
// 判据二：8.2 那张表里每个人的境界、五行、架势、破绽、行动、蓄势（-1 / 0 = 那一格没写）
// ---------------------------------------------------------------------------
struct RoleSpec {
    const char* roleId;
    Realm realm;
    int element;
    int toughness;
    int weaknesses;
    int actions;         // 0 = 没写
    int chargeEvery;     // 0 = 没写蓄势
    double chargeMult;
    bool chargeAll;
    const char* why;
};

const std::vector<RoleSpec>& rolesSaySo() {
    using fanren::core::kElementEarth;
    using fanren::core::kElementMetal;
    using fanren::core::kElementWater;
    using fanren::core::kElementWood;
    static const std::vector<RoleSpec> kRoles = {
        {"lu_shixiong", Realm::QiRefining12, kElementWood, 5, cats({"金", "火"}), 0, 3, 2.0, false,
         "① 陆师兄（十二层，木＝风灵根；架势 5；破绽金、火；蓄势「化蛟」每 3 回合 ×2.0 单体）"},
        {"luosai_huzi", Realm::QiRefining13, kElementWood, 3, cats({"土", "金"}), 1, 0, 0, false,
         "② 络腮胡子（十三层，木；架势 3；破绽土、金；行动 1）"},
        {"yan_xiongdi", Realm::QiRefining12, kElementEarth, 2, cats({"暗器", "火"}), 0, 0, 0, false,
         "② 严姓（十二层顶峰，土；架势 2；破绽暗器、火）"},
        {"zichi_bishe", Realm::Mortal, kElementWood, 2, cats({"火"}), 0, 0, 0, false, "② 紫翅碧蛇（木；架势 2；破绽火）"},
        {"chuanshan_shou", Realm::Mortal, kElementEarth, 2, cats({"木", "水"}), 0, 0, 0, false,
         "② 穿山甲兽（土；架势 2；破绽木、水）"},
        {"feng_yue", Realm::QiRefining13, kElementEarth, 6, cats({"火", "木"}), 2, 2, 1.8, false,
         "③ 封岳（十三层顶峰，土；架势 6；破绽火、木；行动 2；蓄势「小刀符宝」每 2 回合 ×1.8 单体）"},
        {"yan_wuchi", Realm::QiRefining13, kElementMetal, 5, cats({"暗器", "火"}), 0, 3, 2.0, false,
         "④ 赤脚大汉（十三层，金；架势 5；破绽暗器、火；蓄势「银盘」每 3 回合 ×2.0 单体）"},
        {"mo_jiao", Realm::QiRefining13, kElementWater, 6, cats({"土", "火"}), 2, 3, 1.8, true,
         "⑤ 墨蛟（数据写炼气十三层，水；架势 6；破绽土、火；行动 2；蓄势「紫液」每 3 回合 ×1.8 全体）"},
        {"tuishan_shou", Realm::Mortal, kElementEarth, -1, cats({"木", "暗器"}), 0, 0, 0, false, "可选 推山兽（土；破绽木、暗器）"},
        {"tiebi_yuan", Realm::Mortal, -1, -1, cats({"火", "剑"}), 0, 0, 0, false, "可选 铁臂猿（破绽火、剑）"},
        {"huoyan_shu", Realm::Mortal, -1, -1, cats({"水"}), 0, 0, 0, false, "可选 火焰鼠（破绽水）"},
    };
    return kRoles;
}

class Ch07BattleData : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    Application app_;
};

// 验收 4：五张必打 ＋ 三张遭遇与 8.2 一致——谁在场、敌友、逃不逃得、输了死不死、灵石 0、drops 空、背景。
TEST_F(Ch07BattleData, TheCompositionsAreTheOnesTableEightTwoDraws) {
    ASSERT_EQ(designSaysSo().size(), 8u) << "施工图 8.2：五张必打 ＋ 三张遭遇";
    for (const CompositionSpec& spec : designSaysSo()) {
        const BattleSetup* setup = app_.battleSetup(spec.id);
        ASSERT_NE(setup, nullptr) << spec.mark << " " << spec.id << " 不在 data/battles";
        EXPECT_EQ(setup->chapter, 7) << spec.id << "：规则 25 按编成的 chapter 认章，遭遇编成也写 7（8.2 表下）";
        std::multiset<std::string> allies;
        std::multiset<std::string> enemies;
        for (const BattleUnitSpec& unit : setup->units) {
            (unit.ally ? allies : enemies).insert(unit.roleId);
            EXPECT_EQ(unit.wave, 0) << spec.id << "：8.2 没有波次（② 的两只灵兽前移到开场，8.3）";
        }
        EXPECT_EQ(allies, spec.allies) << spec.mark << " " << spec.id << " 的友军与 8.2 不符";
        EXPECT_EQ(enemies, spec.enemies) << spec.mark << " " << spec.id << " 的敌方与 8.2 不符";
        EXPECT_EQ(setup->canEscape, spec.canEscape) << spec.mark << " 「逃」一列";
        EXPECT_EQ(setup->defeatIsFatal, spec.defeatIsFatal) << spec.mark << " 「败」一列";
        EXPECT_FALSE(setup->heroAbsent) << spec.mark << "：韩立在场";
        EXPECT_EQ(setup->reward.spiritStones, 0) << spec.mark << "：8.2 表下「结算卡」/ 第 9 节第 3 条，编成不掉灵石";
        EXPECT_TRUE(setup->reward.drops.empty()) << spec.mark << "：drops 空（东西全由脚本搜身给）";
        EXPECT_EQ(app_.battleBackdrop(spec.id), spec.backdrop) << spec.mark << "：8.2 表下的战斗背景（走 Application 的真实入口）";
    }
    // 验收 4 原文单点：⑤ killable_by == ["土"]、友军恰一人；五张 can_escape false、defeat_is_fatal true。
    const RoleTemplate* mojiao = app_.data().findRole("mo_jiao");
    ASSERT_NE(mojiao, nullptr);
    EXPECT_EQ(mojiao->killableBy, cats({"土"})) << "验收 4：⑤ killable_by == [\"土\"]";
    const BattleSetup* zhaoze = app_.battleSetup("b07_zhaoze_shouyao");
    ASSERT_NE(zhaoze, nullptr);
    const auto allies = std::count_if(zhaoze->units.begin(), zhaoze->units.end(), [](const BattleUnitSpec& u) { return u.ally; });
    EXPECT_EQ(allies, 1) << "验收 4：⑤ 友军恰一人（白衣少女，编成只有一张）";
    // 另外四个 ⑤ 以外的必打敌人都不设 killable_by（8.2 ③：「不用 killable_by」）。
    for (const char* id : {"lu_shixiong", "luosai_huzi", "yan_xiongdi", "zichi_bishe", "chuanshan_shou", "feng_yue", "yan_wuchi"}) {
        const RoleTemplate* role = app_.data().findRole(id);
        ASSERT_NE(role, nullptr) << id;
        EXPECT_EQ(role->killableBy, 0) << id << "：8.2 只有 ⑤ 写了 killable_by";
    }
}

// 8.2 每个人的境界、五行、架势、破绽、行动、蓄势。
TEST_F(Ch07BattleData, EveryoneIsTheRealmElementStanceWeaknessAndMovesOfTableEightTwo) {
    for (const RoleSpec& spec : rolesSaySo()) {
        const RoleTemplate* role = app_.data().findRole(spec.roleId);
        ASSERT_NE(role, nullptr) << spec.roleId;
        if (spec.realm != Realm::Mortal) EXPECT_EQ(role->realm, spec.realm) << spec.why;
        if (spec.element >= 0) EXPECT_EQ(role->element, spec.element) << spec.why;
        if (spec.toughness >= 0) EXPECT_EQ(role->toughness, spec.toughness) << spec.why;
        EXPECT_EQ(role->weaknesses, spec.weaknesses) << spec.why;
        if (spec.actions > 0) EXPECT_EQ(role->actions, spec.actions) << spec.why;
        if (spec.chargeEvery > 0) {
            EXPECT_EQ(role->charge.every, spec.chargeEvery) << spec.why;
            EXPECT_DOUBLE_EQ(role->charge.mult, spec.chargeMult) << spec.why;
            EXPECT_EQ(role->charge.all, spec.chargeAll) << spec.why;
            EXPECT_NE(app_.data().lookupText(role->charge.textKey), role->charge.textKey) << spec.roleId << " 的蓄势预告查不到";
        }
    }
}

// 8.2 ① 与 10.1 E7：陆师兄 maxHp 钉 180（RealmTests 的曲线锚点）；法术青弧斩（木），法术表里留一门伤人法术且法力够放
//（Ch03BattleKitTests 拿它验「敌人真的施法」）。⑤ 友军白衣少女：朱雀环＝火攻（8.2 ⑤ 友军一列）。
TEST_F(Ch07BattleData, TheAnchorsAndTheNamedSpellsOfTableEightTwo) {
    const RoleTemplate* lu = app_.data().findRole("lu_shixiong");
    ASSERT_NE(lu, nullptr);
    EXPECT_EQ(lu->maxHp, 180) << "8.2 ① / E7：maxHp 钉 180（十二层 168 的 ±15% 内）";
    EXPECT_EQ(lu->realm, Realm::QiRefining12);
    const int curve = fanren::rules::realmMaxHp(Realm::QiRefining12);
    EXPECT_LE(std::abs(lu->maxHp - curve) * 100, curve * 15) << "E7：锚点在十二层曲线的一成半以内";
    bool castable = false;
    for (const std::string& id : lu->magics) {
        const fanren::core::Magic* m = app_.data().findMagic(id);
        ASSERT_NE(m, nullptr) << id;
        castable = castable || (m->power > 0 && m->needMp <= lu->maxMp);
    }
    EXPECT_TRUE(castable) << "E7：陆师兄的法术表里必须留一门伤人法术且法力够放";
    const fanren::core::Magic* qinghu = app_.data().findMagic("magic_qinghu_zhan");
    ASSERT_NE(qinghu, nullptr) << "8.2 ①：青弧斩";
    EXPECT_EQ(qinghu->element, fanren::core::kElementWood) << "8.2 ①：青弧斩（木）";
    EXPECT_NE(std::find(lu->magics.begin(), lu->magics.end(), "magic_qinghu_zhan"), lu->magics.end());
    const RoleTemplate* girl = app_.data().findRole("baiyi_shaonv");
    ASSERT_NE(girl, nullptr);
    const fanren::core::Magic* ring = app_.data().findMagic("magic_zhuque_huan");
    ASSERT_NE(ring, nullptr) << "8.2 ⑤：朱雀环";
    EXPECT_EQ(ring->element, fanren::core::kElementFire) << "8.2 ⑤：朱雀环＝火攻";
    EXPECT_GT(ring->power, 0);
    EXPECT_NE(std::find(girl->magics.begin(), girl->magics.end(), "magic_zhuque_huan"), girl->magics.end());
    // 陆师兄在第 6 章起就在的锚点表里（RealmTests）：那一行已经改成十二层 180（E7，引擎路）——这里读字核一遍。
    const std::string realmTests = readFile(fs::path(assetRoot()) / "tests" / "RealmTests.cpp");
    EXPECT_NE(realmTests.find("{Realm::QiRefining12, 180, \"lu_shixiong\"}"), std::string::npos)
        << "E7：RealmTests 的锚点该是 {QiRefining12, 180, lu_shixiong}";
}

// 规则 25 的意思落到这一章（8.2 表下第一条）：每一个敌人至少一样破绽、⑤ 要命的那一类，都是**那一刻**我方必有的手段
// 打得到的。手段照施工图写死：
//   ①（节点 13，11b 万宝楼之后、陆师兄死之前）：拳、剑（软剑、烈阳剑）、刀（冷月刀）、火（火弹术）、土（流沙术、金光砖）、
//     水（冰冻术）、金（剑符、金蚨子母刃）；
//   ②–⑤ 与环形山遭遇（节点 21b 之后）：①的全部 ＋ 木（青蛟旗，节点 13 战后）＋ 暗器（丝线，节点 21b）。
TEST_F(Ch07BattleData, EveryFoeHasAWeaknessTheHandAtThatNodeCanReach) {
    const int atLu = cats({"拳", "剑", "刀", "火", "土", "水", "金"});
    const int afterSixian = atLu | cats({"木", "暗器"});
    const std::vector<std::pair<const char*, int>> kMeans = {
        {"b07_lu_shixiong", atLu},         {"b07_yixiantian", afterSixian},    {"b07_fengyue", afterSixian},
        {"b07_zhongxinqu_duoyao", afterSixian}, {"b07_zhaoze_shouyao", afterSixian}, {"be07_tuishan_shou", afterSixian},
        {"be07_tiebi_yuan", afterSixian},  {"be07_huoyan_shu", afterSixian}};
    for (const auto& [id, means] : kMeans) {
        const BattleSetup* setup = app_.battleSetup(id);
        ASSERT_NE(setup, nullptr) << id;
        for (const BattleUnitSpec& unit : setup->units) {
            if (unit.ally) continue;
            const RoleTemplate* role = app_.data().findRole(unit.roleId);
            ASSERT_NE(role, nullptr) << unit.roleId;
            EXPECT_NE(role->weaknesses & means, 0) << id << " / " << unit.roleId << " 的破绽「"
                                                   << fanren::core::categoryNames(role->weaknesses)
                                                   << "」没有一样是那一刻我方必有的手段打得到的（规则 25）";
            if (role->killableBy != 0) {
                EXPECT_NE(role->killableBy & means, 0) << id << " / " << unit.roleId << " 要命的那一类，那一刻手里没有";
            }
        }
    }
    // 手段的来路本身（施工图 E5 / 第 7 节「法器即法术」）：祭金蚨子母刃金、连发；祭金光砖土、一击；祭青蛟旗木；
    // 祭青凝镜削架势 2；丝线是暗器兵刃。
    struct SpellSpec {
        const char* id;
        int element;
        fanren::core::MagicEffect effect;
    };
    for (const SpellSpec& spec : std::vector<SpellSpec>{
             {"magic_ji_jinfu", fanren::core::kElementMetal, fanren::core::MagicEffect::None},
             {"magic_ji_jinguangzhuan", fanren::core::kElementEarth, fanren::core::MagicEffect::None},
             {"magic_ji_qingjiao", fanren::core::kElementWood, fanren::core::MagicEffect::None},
             {"magic_ji_qingning", fanren::core::kElementNone, fanren::core::MagicEffect::Stagger}}) {
        const fanren::core::Magic* m = app_.data().findMagic(spec.id);
        ASSERT_NE(m, nullptr) << spec.id;
        EXPECT_EQ(m->element, spec.element) << spec.id << "：E5";
        EXPECT_EQ(m->effect, spec.effect) << spec.id << "：E5";
    }
    const fanren::core::Magic* jinfu = app_.data().findMagic("magic_ji_jinfu");
    ASSERT_NE(jinfu, nullptr);
    EXPECT_EQ(jinfu->boost, fanren::core::MagicBoost::Hits) << "E5：金蚨子母刃连发";
    const fanren::core::Magic* brick = app_.data().findMagic("magic_ji_jinguangzhuan");
    ASSERT_NE(brick, nullptr);
    EXPECT_NE(brick->boost, fanren::core::MagicBoost::Hits) << "E5：金光砖一击";
    const fanren::core::Magic* mirror = app_.data().findMagic("magic_ji_qingning");
    ASSERT_NE(mirror, nullptr);
    EXPECT_EQ(mirror->stagger, 2) << "E5：祭青凝镜 effect stagger 2";
    const fanren::core::Item* thread = app_.data().findItem("weapon_wuming_sixian");
    ASSERT_NE(thread, nullptr) << "3.2 节点 21b：透明丝线";
    EXPECT_EQ(thread->weapon, cats({"暗器"})) << "第 7 节：透明丝线做成暗器兵刃";
}

// 施工图 E6 / 8.2 表下第一条：规则 25 的表登了本章。直接读 tools/validate.py 的字（协调者那一段，本条只看它登了什么）。
TEST_F(Ch07BattleData, RuleTwentyFiveKnowsTheMeansOfChapterSeven) {
    const std::string validate = readFile(fs::path(assetRoot()) / "tools" / "validate.py");
    ASSERT_GT(validate.size(), 10000u) << "先验：读得到 tools/validate.py";
    EXPECT_TRUE(validate.find("\nMEANS_LAST_CHAPTER = 7\n") != std::string::npos ||
                validate.find("\nMEANS_LAST_CHAPTER = 8\n") != std::string::npos ||
                validate.find("\nMEANS_LAST_CHAPTER = 9\n") != std::string::npos)
        << "E6: Chapter 7 remains covered after the Chapter 9 table extension";
    const std::size_t chapterMeans = validate.find("\nCHAPTER_MEANS = {");
    ASSERT_NE(chapterMeans, std::string::npos);
    const std::size_t seven = validate.find("\n    7: [", chapterMeans);
    ASSERT_NE(seven, std::string::npos) << "E6：CHAPTER_MEANS 有第 7 章";
    const std::string row = validate.substr(seven, validate.find(']', seven) - seven);
    for (const char* means : {"(\"拳\", \"hand\", \"\", \"\")", "(\"火\", \"learn\", \"magic_huodan_shu\", \"ch04\")",
                              "(\"土\", \"learn\", \"magic_liusha_shu\", \"ch06\")",
                              "(\"水\", \"learn\", \"magic_bingdong_shu\", \"ch06\")",
                              "(\"金\", \"learn\", \"magic_ji_jinfu\", \"ch07\")"}) {
        EXPECT_NE(row.find(means), std::string::npos) << "8.2 表下：CHAPTER_MEANS[7] 该有 " << means;
    }
    EXPECT_EQ(row.find("金光砖"), std::string::npos) << "8.2 表下：金光砖（土）不登";
    EXPECT_EQ(row.find("magic_ji_jinguangzhuan"), std::string::npos) << "8.2 表下：金光砖（土）不登";
    const std::size_t extra = validate.find("\nBATTLE_EXTRA_MEANS = {");
    ASSERT_NE(extra, std::string::npos);
    const std::string table = validate.substr(extra, validate.find("\n}", extra) - extra);
    for (const char* means : {"(\"木\", \"learn\", \"magic_ji_qingjiao\", \"ch07\")",
                              "(\"暗器\", \"give\", \"weapon_wuming_sixian\", \"ch07\")"}) {
        EXPECT_NE(table.find(means), std::string::npos) << "8.2 表下：BATTLE_EXTRA_MEANS 该有 " << means;
    }
    for (const char* id : {"b07_yixiantian", "b07_fengyue", "b07_zhongxinqu_duoyao", "b07_zhaoze_shouyao", "be07_tuishan_shou"}) {
        EXPECT_NE(table.find(std::string("\"") + id + "\""), std::string::npos) << "8.2 表下：②③④⑤ 与 be07_tuishan_shou 各加木、暗器";
    }
    EXPECT_EQ(table.find("\"b07_lu_shixiong\""), std::string::npos) << "8.2 表下：① 不加（那时两样都还没有）";
}

// 8.4：环形山遭遇表（dailyCap 2、三张编成），读 Application 真读进来的那一份。
TEST_F(Ch07BattleData, TheRingMountainEncounterTableIsSectionEightFour) {
    const auto& tables = app_.encounterTables();
    const auto it = tables.find("encounter_ch07_huanxingshan");
    ASSERT_NE(it, tables.end()) << "8.4：data/encounters/ch07_huanxingshan.json";
    EXPECT_EQ(it->second.dailyCap, 2) << "8.4：dailyCap 2";
    std::set<std::string> ids;
    for (const auto& entry : it->second.entries) ids.insert(entry.battleId);
    EXPECT_EQ(ids, (std::set<std::string>{"be07_tuishan_shou", "be07_tiebi_yuan", "be07_huoyan_shu"})) << "8.4 的三条";
    for (const auto& entry : it->second.entries) {
        EXPECT_LE(fanren::rules::toValue(entry.minRealm), fanren::rules::toValue(Realm::QiRefining11))
            << entry.battleId << "：十一层的意图玩家遇得上";
        EXPECT_GE(fanren::rules::toValue(entry.maxRealm), fanren::rules::toValue(Realm::QiRefining11)) << entry.battleId;
    }
}

// 扫描器自检：本章各张编成的敌方两两不同——Ch07AcceptanceTests 的 identifyBattle 按落地数据里的敌方（role 与波次）
// 认是哪一场，所以读的是 data/battles，不是上面的判据表（判据表两两不同，数据照样可能撞在一起）。
TEST_F(Ch07BattleData, TheCompositionsCanBeToldApartByTheirFoes) {
    std::map<std::multiset<std::string>, std::string> seen;
    for (const CompositionSpec& spec : designSaysSo()) {
        const BattleSetup* setup = app_.battleSetup(spec.id);
        ASSERT_NE(setup, nullptr) << spec.id;
        std::multiset<std::string> foes;
        for (const BattleUnitSpec& unit : setup->units) {
            if (!unit.ally) foes.insert(unit.roleId + "@" + std::to_string(unit.wave));
        }
        const auto [it, fresh] = seen.emplace(foes, spec.id);
        EXPECT_TRUE(fresh) << spec.id << " 的敌方与 " << it->second << " 相同，通关测试认不出是哪一场";
    }
}

// ---------------------------------------------------------------------------
// 那只手在意图玩家的局面下：五场必打各打一遍（施工图 8.1 / 8.2 的「意图」一列、验收 4 末句）
// ---------------------------------------------------------------------------
class Ch07IntentHand : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    // 第 fight 场（0 = ①…4 = ⑤）开打时，意图玩家身上该有的东西（施工图 8.1 那一段逐项、3.2 各节点的 give / take）：
    //   起点：第 6 章交接存档第二侧（省的那一侧：没做 Z1、身上没有定神符）；
    //   节点 3：十一层（1.3，满血满法力——禁地里每一场之前他都在打坐点坐满了，见 Ch07AcceptanceTests 文件头）；
    //   节点 1：中阶灵石 2、土牢符 2；节点 11：天雷子、金光砖符宝，学祭金蚨子母刃、祭金光砖；
    //   ① 之后：剑符与祭剑符收回、学祭青蛟旗、火球符 4、护身符 2；节点 17：养精丹 3、金疮药 3（没做 Z1）；
    //   节点 18：中阶灵石 1；节点 21b：丝线；节点 23（③ 之后）：学祭青凝镜。
    GameState intentAt(int fight) {
        auto loaded = fanren::io::loadGame(
            fanren::test::chapterFixturePath(assetRoot(), fanren::test::kChapterSixEndingSecond).string());
        EXPECT_TRUE(loaded.ok) << loaded.error;
        GameState s = loaded.value;
        s.realm = s.realmCap = Realm::QiRefining11;
        s.hp = s.maxHp = fanren::rules::realmMaxHp(Realm::QiRefining11);
        s.mp = s.maxMp = fanren::rules::realmMaxMp(Realm::QiRefining11);
        s.addItem("material_lingshi_zhong", 2, 0);
        s.addItem("talisman_tulao_fu", 2, 0);
        s.addItem("talisman_tianleizi", 1, 0);
        s.addItem("talisman_jinguangzhuan", 1, 0);
        s.learnMagic("magic_ji_jinfu");
        s.learnMagic("magic_ji_jinguangzhuan");
        if (fight >= 1) {
            while (s.removeItem("talisman_jianfu", 1)) {
            }
            s.forgetMagic("magic_ji_jianfu");
            s.learnMagic("magic_ji_qingjiao");
            s.addItem("talisman_huoqiu_fu", 4, 0);
            s.addItem("talisman_hushen_fu", 2, 0);
            s.addItem("pill_yangjing_dan", 3, 0);
            s.addItem("pill_jinchuang_yao", 3, 0);
            s.addItem("material_lingshi_zhong", 1, 0);
            s.addItem("weapon_wuming_sixian", 1, 0);
        }
        if (fight >= 3) s.learnMagic("magic_ji_qingning");
        return s;
    }

    static fanren::test::HandPolicy intentPolicy() {
        fanren::test::HandPolicy p;
        p.healAtPercent = 50;
        p.healWhenDoomed = false;
        p.fleeWhenSpent = true;
        p.pills = {"pill_yangjing_dan", "pill_jinchuang_yao"};
        return p;
    }

    // 一场打下来量到的东西（平衡路 2026-09-29 补齐：从前只记胜负、回合、剩血，docs/ch07-balance.md 要的
    // 法力、吃了几瓶、一回合挨得最重的一轮都量不出来）。
    struct Result {
        BattlePhase phase = BattlePhase::Ongoing;
        int rounds = 0;
        int hpLeft = 0;
        int mpLeft = 0;
        int pills = 0;         // 这一场吃掉的养精丹
        int salves = 0;        // 这一场吃掉的金疮药
        int allyHpLeft = -1;   // 友军（⑤ 白衣少女）收场时的气血；没有友军 -1
        int worstRound = 0;    // 韩立一回合里挨得最重的一轮（打在他身上的每一击 ＋ 毒发）：7.3 斩杀线口径的实测
        int breaks = 0;        // 敌方被打到破势几次
        int charges = 0;       // 敌方宣告蓄势几次
        int interrupts = 0;    // 蓄势被破势打断几次
        [[nodiscard]] int eaten() const { return pills + salves; }
    };

    // 开场亲手出的那一招（与 Ch07AcceptanceTests 的 openWith 同一个做法：轮到韩立的头一手，走真菜单把那件东西
    // 用在那个人身上）。那只手不会用带 castMagic 的符（BattleHand 只撒 policy.poisons 里的毒）。
    struct Opener {
        const char* itemId = nullptr;
        const char* roleId = nullptr;
    };

    Result fight(const char* battleId, const GameState& start, Opener opener = {}) {
        app_.state() = start;
        BattleScene scene(battleId);
        scene.onEnter(app_);
        fanren::test::BattleHand hand(app_, intentPolicy());
        if (opener.itemId != nullptr) {
            const int actor = scene.runToAllyTurn();
            int target = -1;
            for (std::size_t i = 0; i < scene.battle().units().size(); ++i) {
                const Unit& u = scene.battle().units()[i];
                if (!u.ally && u.alive() && u.id == opener.roleId) target = static_cast<int>(i);
            }
            fanren::core::battle::Action a;
            a.kind = fanren::core::battle::ActionKind::Item;
            a.actorIndex = actor;
            a.targetIndex = target;
            a.magicId = opener.itemId;
            EXPECT_TRUE(actor >= 0 && target >= 0 && hand.issue(scene, a))
                << battleId << "：开场那一招（" << opener.itemId << " → " << opener.roleId << "）没用出去";
        }
        Result r;
        r.phase = hand.play(scene);
        r.rounds = scene.battle().round();
        int hero = -1;
        for (std::size_t i = 0; i < scene.battle().units().size(); ++i) {
            const Unit& u = scene.battle().units()[i];
            if (!u.ally) continue;
            if (u.id == "hanli") {
                hero = static_cast<int>(i);
                r.hpLeft = u.hp;
                r.mpLeft = u.mp;
            } else {
                r.allyHpLeft = u.hp;
            }
        }
        r.pills = start.itemCount("pill_yangjing_dan") - app_.state().itemCount("pill_yangjing_dan");
        r.salves = start.itemCount("pill_jinchuang_yao") - app_.state().itemCount("pill_jinchuang_yao");
        using Kind = fanren::core::battle::BattleEventKind;
        int thisRound = 0;
        for (const auto& e : scene.battle().events()) {
            if (e.kind == Kind::RoundStart) {
                thisRound = 0;
            } else if ((e.kind == Kind::Hit || e.kind == Kind::PoisonTick) && e.target == hero) {
                thisRound += e.value;
                r.worstRound = std::max(r.worstRound, thisRound);
            } else if (e.kind == Kind::Break) {
                ++r.breaks;
            } else if (e.kind == Kind::ChargeDeclare) {
                ++r.charges;
            } else if (e.kind == Kind::ChargeInterrupt) {
                ++r.interrupts;
            }
        }
        // 平衡路量数用：set FANREN_CH07_BATTLE_LOG=1 再跑，打印每一场的出手记录（不设就不打印）。
        if (fanren::test::environmentFlagSet("FANREN_CH07_BATTLE_LOG")) {
            std::cout << "---- " << battleId << " ----" << std::endl;
            for (const std::string& line : scene.battle().log()) std::cout << "  " << line << std::endl;
        }
        return r;
    }

    static std::string describe(const Result& r, const GameState& start) {
        std::ostringstream out;
        out << phaseName(r.phase) << " " << r.rounds << " 回合，血 " << start.hp << "→" << r.hpLeft << "/" << start.maxHp
            << "，法力 " << start.mp << "→" << r.mpLeft << "，养精丹 " << r.pills << "、金疮药 " << r.salves
            << "，最重一轮 " << r.worstRound << "，破势 " << r.breaks << "、蓄势 " << r.charges << "（打断 " << r.interrupts
            << "）";
        if (r.allyHpLeft >= 0) out << "，友军剩血 " << r.allyHpLeft;
        return out.str();
    }

    // docs/octopath-battle.md 7.3 的斩杀线口径，搬到本章（平衡路）：
    //   · 一波人人都砍他——这一波每个敌人、每一次行动都打在韩立身上，各挑它此刻最重的一手（法力够就算法术：
    //     敌方 AI 有伤人的法术且法力够就一定放法术，docs/octopath-battle.md 3.1）；
    //   · 重招回合（他防御着）——会蓄势的首领出重招（× mult）、其余照常，全程 × 0.5。
    // 伤害一律问规则层（estimateHitDamage × hitCount），与那只手估对面同一个口（BattleHand::worstHit）。
    struct KillLine {
        int plain = 0;
        int heavy = 0;
        bool charged = false;
    };

    KillLine killLine(const char* battleId, const GameState& start) {
        app_.state() = start;
        BattleScene scene(battleId);
        scene.onEnter(app_);
        const fanren::core::battle::BattleState& b = scene.battle();
        int hero = -1;
        for (std::size_t i = 0; i < b.units().size(); ++i) {
            if (b.units()[i].ally && b.units()[i].id == "hanli") hero = static_cast<int>(i);
        }
        KillLine line;
        EXPECT_GE(hero, 0) << battleId << "：韩立不在场";
        if (hero < 0) return line;
        std::map<int, int> plain;
        std::map<int, int> heavy;
        for (std::size_t i = 0; i < b.units().size(); ++i) {
            const Unit& e = b.units()[i];
            if (e.ally) continue;
            fanren::core::battle::Action swing;
            swing.kind = fanren::core::battle::ActionKind::Attack;
            swing.actorIndex = static_cast<int>(i);
            swing.targetIndex = hero;
            int worst = b.estimateHitDamage(swing, hero);
            for (const std::string& id : e.magics) {
                const fanren::core::Magic* m = b.findMagic(id);
                if (m == nullptr || e.mp < m->needMp || (m->power <= 0 && m->poison <= 0)) continue;
                fanren::core::battle::Action cast = swing;
                cast.kind = fanren::core::battle::ActionKind::Cast;
                cast.magicId = id;
                worst = std::max(worst, b.estimateHitDamage(cast, hero) * b.hitCount(cast));
            }
            const auto half = [](double v) { return std::max(1, static_cast<int>(std::lround(v * 0.5))); };
            plain[e.wave] += worst * e.actions;
            if (e.chargeEvery > 0) {
                line.charged = true;
                heavy[e.wave] += half(worst * e.chargeMult) + half(worst) * (e.actions - 1);
            } else {
                heavy[e.wave] += half(worst) * e.actions;
            }
        }
        for (const auto& [wave, sum] : plain) line.plain = std::max(line.plain, sum);
        for (const auto& [wave, sum] : heavy) line.heavy = std::max(line.heavy, sum);
        return line;
    }

    static const char* phaseName(BattlePhase phase) {
        switch (phase) {
            case BattlePhase::Won: return "胜";
            case BattlePhase::Lost: return "负";
            case BattlePhase::Escaped: return "逃";
            case BattlePhase::EnemyFled: return "敌逃";
            case BattlePhase::Ongoing: return "未完";
        }
        return "?";
    }

    Application app_;
};

TEST_F(Ch07IntentHand, TheIntentPlayerWinsEachOfTheFiveFromAFullBar) {
    const char* kFights[] = {"b07_lu_shixiong", "b07_yixiantian", "b07_fengyue", "b07_zhongxinqu_duoyao",
                             "b07_zhaoze_shouyao"};
    const char* kMarks[] = {"①", "②", "③", "④", "⑤"};
    std::ostringstream table;
    std::vector<BattlePhase> phases;
    for (int i = 0; i < 5; ++i) {
        const GameState start = intentAt(i);
        const Result r = fight(kFights[i], start);
        phases.push_back(r.phase);
        table << "\n  " << kMarks[i] << " " << kFights[i] << "：" << describe(r, start);
    }
    std::cout << "[ch07 意图玩家各打一场]" << table.str() << std::endl;
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(phases[static_cast<std::size_t>(i)], BattlePhase::Won)
            << kMarks[i] << " " << kFights[i] << "：五场都是生死仗（8.2「败」致命），意图玩家满血满法力开打也该打得赢"
            << table.str();
    }
}

// 协调者裁决（施工图 16.5 末行，平衡路 2026-09-29）：② ③ 要吃药；① ④ ⑤ 不许一回合打完——测试路报过「①④⑤ 一回合、
// ②③ 打不过」，难度断成两截。同上一条的切片：意图玩家满血满法力、单场。
TEST_F(Ch07IntentHand, TwoAndThreeTakePillsAndNoneOfTheOtherThreeEndsInOneRound) {
    const char* kFights[] = {"b07_lu_shixiong", "b07_yixiantian", "b07_fengyue", "b07_zhongxinqu_duoyao",
                             "b07_zhaoze_shouyao"};
    const char* kMarks[] = {"①", "②", "③", "④", "⑤"};
    for (int i = 0; i < 5; ++i) {
        const GameState start = intentAt(i);
        const Result r = fight(kFights[i], start);
        const std::string line = std::string(kMarks[i]) + " " + kFights[i] + "：" + describe(r, start);
        EXPECT_EQ(r.phase, BattlePhase::Won) << line;
        if (r.phase != BattlePhase::Won) continue;   // 输了的那一场，下面两条说明不了什么
        if (i == 1 || i == 2) {
            EXPECT_GE(r.eaten(), 1) << line << "——意图玩家一瓶不吃就打完了，② ③ 是本章要吃药的两场（16.5）";
        } else {
            EXPECT_GE(r.rounds, 2) << line << "——一回合就打完了（16.5：① ④ ⑤ 不许一回合打完）";
        }
    }
}

// 斩杀线（docs/octopath-battle.md 7.3 的口径）：施工图 8.2 ② 点名「单轮伤害按 7.3 的斩杀线口径、由平衡路拿 BattleHand 量」；
// 平衡路把同一口径用到五场——单轮最大伤害（一波人人都砍他）与重招回合（他防御着）都不超过韩立气血上限的三成。
// 静态那两列照 Ch05BattleDataTests 的口径算（法力够就算法术，那是敌方 AI 真会放的）；实测那一列是那只手打下来
// 韩立一回合里真挨得最重的一轮，只打印。
TEST_F(Ch07IntentHand, NoRoundOfTheFiveTakesMoreThanThreeTenthsOfHisLife) {
    const char* kFights[] = {"b07_lu_shixiong", "b07_yixiantian", "b07_fengyue", "b07_zhongxinqu_duoyao",
                             "b07_zhaoze_shouyao"};
    const char* kMarks[] = {"①", "②", "③", "④", "⑤"};
    constexpr int kKillLinePercent = 30;
    std::ostringstream table;
    for (int i = 0; i < 5; ++i) {
        const GameState start = intentAt(i);
        const KillLine line = killLine(kFights[i], start);
        const Result r = fight(kFights[i], start);
        table << "\n  " << kMarks[i] << " " << kFights[i] << "：一波人人都砍他 " << line.plain << "，重招回合 "
              << (line.charged ? std::to_string(line.heavy) : std::string("—")) << "，实测最重一轮 " << r.worstRound
              << "（上限 " << start.maxHp << " 的三成 = " << start.maxHp * kKillLinePercent / 100 << "）";
        EXPECT_LE(line.plain * 100, start.maxHp * kKillLinePercent)
            << kMarks[i] << " " << kFights[i] << "：单轮最大 " << line.plain << " 超过上限 " << start.maxHp << " 的三成";
        if (line.charged) {
            EXPECT_LE(line.heavy * 100, start.maxHp * kKillLinePercent)
                << kMarks[i] << " " << kFights[i] << "：重招回合防御着还挨 " << line.heavy << "，超过上限的三成";
        }
    }
    std::cout << "[ch07 斩杀线（7.3 口径）]" << table.str() << std::endl;
}

// 环形山遭遇（施工图 8.4；契约 docs/interfaces-octo-encounters.md「低于同章剧情战」，门禁规则 27 按「凶」判）：
// 遭遇区要 ch07.yueyang（节点 25，③ 之后），拿 ④ 开打时的意图玩家满血去撞——三张都赢、一瓶不吃
//（be07_tiebi_yuan 兼作支线 Z2 东坡那一场，第一侧通关就打它）。
TEST_F(Ch07IntentHand, TheRingMountainEncountersNeedNoPills) {
    std::ostringstream table;
    for (const char* id : {"be07_tuishan_shou", "be07_tiebi_yuan", "be07_huoyan_shu"}) {
        const GameState start = intentAt(3);
        const Result r = fight(id, start);
        table << "\n  " << id << "：" << describe(r, start);
        EXPECT_EQ(r.phase, BattlePhase::Won) << id << "：" << describe(r, start);
        EXPECT_EQ(r.eaten(), 0) << id << "：可逃的小仗，意图玩家满血开打不该要吃药——" << describe(r, start);
    }
    std::cout << "[ch07 环形山遭遇]" << table.str() << std::endl;
}

// 验收 4 末句：「③ 在背包没有天雷子时那只手也赢得了（平衡路实测）」——8.2 ③「不用 killable_by：天雷子是消耗品，
// 玩家可能在前两场就扔了」。背包里一粒天雷子也没有，满血开打。
TEST_F(Ch07IntentHand, FengYueFallsWithoutTheThunderPearl) {
    GameState start = intentAt(2);
    ASSERT_FALSE(HasFailure());
    while (start.removeItem("talisman_tianleizi", 1)) {
    }
    ASSERT_EQ(start.itemCount("talisman_tianleizi"), 0) << "先验：背包里没有天雷子";
    const Result r = fight("b07_fengyue", start);
    std::cout << "[ch07 ③ 没有天雷子] " << describe(r, start) << std::endl;
    EXPECT_EQ(r.phase, BattlePhase::Won) << "验收 4：③ 在背包没有天雷子时那只手也赢得了";
}

// 原著解法（施工图 8.2 ②③）：② 开场甩土牢符给络腮胡子、③ 开场掷天雷子（第一侧通关的打法）各打一遍，与不用符的
// 那一遍并排。两场用不用符都得赢（第一侧通关走的就是用符那一路）。
// 「天雷子是省力、不是唯一解」（协调者裁决，施工图 16.5 末行）只判 ③：掷了天雷子的那一遍回合更少、药不多吃。
// ② 只打印不判「省力」：那只手第一手就能拿蓄劲的金光砖把没破势的胡子砸死（他的血在一砖之内），土牢符占掉的正是
// 这一手——对这只手，土牢符换来的是「胡子一下也没来得及还手」，不是更少的回合（平衡路实测，docs/ch07-balance.md）。
TEST_F(Ch07IntentHand, TheBookSolutionsSaveEffortButAreNotTheOnlyWay) {
    struct Case {
        const char* mark;
        const char* battleId;
        int fight;
        Opener opener;
        bool savesEffort;
    };
    const Case kCases[] = {{"②", "b07_yixiantian", 1, {"talisman_tulao_fu", "luosai_huzi"}, false},
                           {"③", "b07_fengyue", 2, {"talisman_tianleizi", "feng_yue"}, true}};
    for (const Case& c : kCases) {
        const GameState start = intentAt(c.fight);
        ASSERT_GE(start.itemCount(c.opener.itemId), 1) << "先验：" << c.mark << " 开打时身上有 " << c.opener.itemId;
        GameState bare = start;
        while (bare.removeItem(c.opener.itemId, 1)) {
        }
        const Result plain = fight(c.battleId, bare);
        const Result opened = fight(c.battleId, start, c.opener);
        std::cout << "[ch07 " << c.mark << " 原著解法] 不用符：" << describe(plain, bare) << "\n                   用 "
                  << c.opener.itemId << "：" << describe(opened, start) << std::endl;
        EXPECT_EQ(plain.phase, BattlePhase::Won) << c.mark << "：不用符也打得赢（天雷子、土牢符不是唯一解）";
        EXPECT_EQ(opened.phase, BattlePhase::Won) << c.mark << "：用了原著那一招反倒打不赢";
        if (!c.savesEffort) continue;
        EXPECT_LT(opened.rounds, plain.rounds) << c.mark << "：掷了天雷子没省下回合——天雷子该是省力的那一招";
        EXPECT_LE(opened.eaten(), plain.eaten()) << c.mark << "：掷了天雷子反倒多吃了药——天雷子该是省力的那一招";
    }
}

// 那只手在这几场里自己稳不稳：同一局面打三遍，落点、回合、剩血、法力、吃的药、最重一轮一样。
TEST_F(Ch07IntentHand, TheHandIsSteadyOnEachFight) {
    const char* kFights[] = {"b07_lu_shixiong", "b07_yixiantian", "b07_fengyue", "b07_zhongxinqu_duoyao",
                             "b07_zhaoze_shouyao"};
    for (int i = 0; i < 5; ++i) {
        const GameState start = intentAt(i);
        const Result first = fight(kFights[i], start);
        for (int run = 1; run < 3; ++run) {
            const Result again = fight(kFights[i], start);
            EXPECT_EQ(again.phase, first.phase) << kFights[i] << " 第 " << (run + 1) << " 遍";
            EXPECT_EQ(again.rounds, first.rounds) << kFights[i] << " 第 " << (run + 1) << " 遍";
            EXPECT_EQ(again.hpLeft, first.hpLeft) << kFights[i] << " 第 " << (run + 1) << " 遍";
            EXPECT_EQ(again.mpLeft, first.mpLeft) << kFights[i] << " 第 " << (run + 1) << " 遍（平衡路补）";
            EXPECT_EQ(again.eaten(), first.eaten()) << kFights[i] << " 第 " << (run + 1) << " 遍（平衡路补）";
            EXPECT_EQ(again.worstRound, first.worstRound) << kFights[i] << " 第 " << (run + 1) << " 遍（平衡路补）";
        }
    }
}

}  // namespace
