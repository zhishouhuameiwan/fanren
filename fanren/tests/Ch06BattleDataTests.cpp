// 第 6 章五张编成与每个人的数——直接读 data，判据抄自施工图 8.2 原文（docs/ch06-design.md 验收 4）。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件（第 4 章独立校对 MEDIUM-1）
// ---------------------------------------------------------------------------
// 第 4 章复核方把一张编成三波压成一波、把火弹术耗法从 8 改成 1，全套测试一条不红：那些数被**用**着
//（通关测试要靠它们才走得完），却从来没有被**断言**过。本文件与 tests/Ch06AcceptanceTests.cpp 的通关测试
// 刻意不同形状：通关测试问「这一章走不走得完」，这里问「这几样是不是施工图写的那几样」。不起脚本、不打仗、不走位。
//
// 逐张按 id 钉（施工图 8.2 那张表点名写死的五张），不写「所有的 b06_* 都……」：那是普查，不是规则。
// 8.2 只写意图与破绽，**血攻防速、蓄势倍率归平衡路**（8.2 表下第二条）——那几样数这里一个也不判。
// 施工偏差（第 18 节，以它为准）：
//   · 18.2 第 2 条：土甲大汉架势 6 → 4（流沙四发 48 ＋ 剑符 40 = 88 ≤ 九层法力 90）；
//   · 18.2 第 3 条：叶家弟子破绽「金」→「剑、金」，黑木 weapons 剑（① 那一刻韩立手里没有金）；
//   · 18.2 第 1 条：② 新建 b06_shanqiu_xisha，占位 b06_shengxianling_jiesha 由协调者删掉。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "core/model/AttackCategory.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::BattleSetup;
using fanren::core::BattleUnitSpec;
using fanren::core::RoleTemplate;
using fanren::game::Application;
using fanren::rules::Realm;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "battles" / "b06_shanqiu_xisha.json")) return candidate;
    }
    return ".";
}

int cats(std::initializer_list<const char*> names) {
    int mask = 0;
    for (const char* name : names) mask |= fanren::core::categoryFromName(name);
    return mask;
}

// ---------------------------------------------------------------------------
// 判据一：施工图 8.2 那张表（一行一张编成）
// ---------------------------------------------------------------------------
struct CompositionSpec {
    const char* mark;
    const char* id;
    std::multiset<std::string> allies;    // 编成里写死的 faction: ally
    std::multiset<std::string> enemies;
    bool canEscape;
    bool defeatIsFatal;
    int cultivation;                      // 第 9 节来源表「三场必打 修为 60 / 150 / 60」；切磋的编成全 0（规则 26）
    const char* backdrop;                 // 8.2 表下「战斗背景」
};

const std::vector<CompositionSpec>& designSaysSo() {
    static const std::vector<CompositionSpec> kSpecs = {
        // ① 友军：校对整改 16.4（HIGH-4）——吴九指节点 7 才头一回见面，换成节点 4 已引见的熊大力。
        {"①", "b06_yejia_xunxin", {"qingwen_daoshi", "hei_mu", "xiong_dali"}, {"ye_bao", "yejia_dizi", "yejia_dizi"},
         false, false, 60, "herb_valley"},
        {"②", "b06_shanqiu_xisha", {}, {"huangyi_ren", "tujia_dahan"}, false, true, 150, "wild_manor"},
        {"③", "b06_wufeng_qiecuo", {}, {"wu_feng"}, false, false, 60, "sect_courtyard"},
        // 可选两场：切磋，逃得了、输了不死；奖励由路径行动条目发，编成的 rewards 全 0（门禁规则 26）。
        // 背景「各随所在图」：吴九指在小楼（inn_hall），外门弟子在黄枫谷（sect_courtyard）。
        {"可选·吴九指", "b06_qiecuo_wujiuzhi", {}, {"wu_jiuzhi"}, true, false, 0, "inn_hall"},
        {"可选·外门", "b06_huangfenggu_qiecuo", {}, {"huangfenggu_waimen_dizi", "huangfenggu_waimen_dizi"}, true,
         false, 0, "sect_courtyard"},
    };
    return kSpecs;
}

// ---------------------------------------------------------------------------
// 判据二：8.2 那张表里每个人的境界、五行、架势、破绽（-1 = 那一格没写）
// ---------------------------------------------------------------------------
struct RoleSpec {
    const char* roleId;
    Realm realm;
    int element;
    int toughness;
    int weaknesses;
    const char* why;
};

const std::vector<RoleSpec>& rolesSaySo() {
    using fanren::core::kElementEarth;
    using fanren::core::kElementNone;
    using fanren::core::kElementWater;
    using fanren::core::kElementWood;
    static const std::vector<RoleSpec> kRoles = {
        {"ye_bao", Realm::QiRefining8, kElementWood, 4, cats({"金", "火"}), "① 叶豹（八层，木，架势 4，破绽金、火）"},
        {"yejia_dizi", Realm::QiRefining7, kElementWood, 3, cats({"剑", "金"}),
         "① 叶家弟子（七层，木，架势 3；破绽「金」→「剑、金」施工偏差 18.2 第 3 条）"},
        {"huangyi_ren", Realm::QiRefining8, kElementWater, -1, cats({"土", "火"}), "② 黄衣人（八层，水；破绽土、火）"},
        {"tujia_dahan", Realm::QiRefining9, kElementEarth, 4, cats({"土", "金"}),
         "② 土甲大汉（九层，土；架势 6 → 4 施工偏差 18.2 第 2 条；破绽土、金）"},
        // 校对整改 16.4（MEDIUM-4）：吴风的破绽去掉「金」（免得引玩家当众放剑符）、留「火」、加一样拳剑类「拳」。
        {"wu_feng", Realm::QiRefining10, kElementWood, 5, cats({"火", "拳"}), "③ 吴风（十层，木，架势 5，破绽火、拳）"},
        {"wu_jiuzhi", Realm::QiRefining8, kElementNone, 3, cats({"拳", "剑"}), "可选 吴九指（八层，无属性，架势 3，破绽拳、剑）"},
        {"huangfenggu_waimen_dizi", Realm::QiRefining6, -1, -1, -1, "可选 外门弟子 ×2（六层）"},
        // 友军（8.2 ① 友军一列）：青纹道士十层、黑木八层、熊大力八层（校对整改 16.4 HIGH-4：原为吴九指）。
        {"qingwen_daoshi", Realm::QiRefining10, -1, -1, -1, "① 友军 青纹道士（十层）"},
        {"hei_mu", Realm::QiRefining8, -1, -1, -1, "① 友军 黑木（八层）"},
        {"xiong_dali", Realm::QiRefining8, -1, -1, -1, "① 友军 熊大力（八层，刀）"},
    };
    return kRoles;
}

class Ch06BattleData : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    Application app_;
};

// 验收 4：五张编成与 8.2 一致——谁在场、敌友、逃不逃得、输了死不死、收获、背景。
TEST_F(Ch06BattleData, TheFiveCompositionsAreTheOnesTableEightTwoDraws) {
    ASSERT_EQ(designSaysSo().size(), 5u) << "施工图 8.2：三张必打 ＋ 两张可选";
    for (const CompositionSpec& spec : designSaysSo()) {
        const BattleSetup* setup = app_.battleSetup(spec.id);
        ASSERT_NE(setup, nullptr) << spec.mark << " " << spec.id << " 不在 data/battles";
        EXPECT_EQ(setup->chapter, 6) << spec.id;
        std::multiset<std::string> allies;
        std::multiset<std::string> enemies;
        for (const BattleUnitSpec& unit : setup->units) {
            (unit.ally ? allies : enemies).insert(unit.roleId);
            EXPECT_EQ(unit.wave, 0) << spec.id << "：8.2 没有波次，全员开场在场";
        }
        EXPECT_EQ(allies, spec.allies) << spec.mark << " " << spec.id << " 的友军与 8.2 不符";
        EXPECT_EQ(enemies, spec.enemies) << spec.mark << " " << spec.id << " 的敌方与 8.2 不符";
        EXPECT_EQ(setup->canEscape, spec.canEscape) << spec.mark << " 「逃」一列";
        EXPECT_EQ(setup->defeatIsFatal, spec.defeatIsFatal) << spec.mark << " 「败」一列";
        EXPECT_FALSE(setup->heroAbsent) << spec.mark << "：韩立在场";
        EXPECT_EQ(setup->reward.cultivation, spec.cultivation) << spec.mark << " 修为（第 9 节来源表 / 规则 26）";
        EXPECT_EQ(setup->reward.spiritStones, 0) << spec.mark << "：第 9 节第 4 条，编成不掉灵石";
        EXPECT_TRUE(setup->reward.drops.empty()) << spec.mark << "：编成不掉东西";
        EXPECT_EQ(app_.battleBackdrop(spec.id), spec.backdrop) << spec.mark << "：8.2 表下的战斗背景（走 Application 的真实入口）";
    }
    // 验收 4 原文单点：② killable_by == ["金"]、can_escape false、defeat_is_fatal true；①③ defeat_is_fatal false；① 友军恰三人。
    const BattleSetup* xunxin = app_.battleSetup("b06_yejia_xunxin");
    const BattleSetup* xisha = app_.battleSetup("b06_shanqiu_xisha");
    const BattleSetup* wufeng = app_.battleSetup("b06_wufeng_qiecuo");
    ASSERT_NE(xunxin, nullptr);
    ASSERT_NE(xisha, nullptr);
    ASSERT_NE(wufeng, nullptr);
    EXPECT_FALSE(xisha->canEscape);
    EXPECT_TRUE(xisha->defeatIsFatal);
    EXPECT_FALSE(xunxin->defeatIsFatal);
    EXPECT_FALSE(wufeng->defeatIsFatal);
    const auto allies = std::count_if(xunxin->units.begin(), xunxin->units.end(), [](const BattleUnitSpec& u) { return u.ally; });
    EXPECT_EQ(allies, 3) << "验收 4：① 友军恰三人";
    const RoleTemplate* dahan = app_.data().findRole("tujia_dahan");
    ASSERT_NE(dahan, nullptr);
    EXPECT_EQ(dahan->killableBy, cats({"金"})) << "验收 4：② killable_by == [\"金\"]（土甲术护体，只有剑符要得了命）";
    // 占位编成已由协调者删掉（施工偏差 18.2 第 1 条）：劫修那一份不该还在。
    EXPECT_EQ(app_.battleSetup("b06_shengxianling_jiesha"), nullptr) << "施工偏差 18.2 第 1 条：占位编成该删";
}

// 8.2 每个人的境界、五行、架势、破绽。
TEST_F(Ch06BattleData, EveryoneInTheFiveIsTheRealmElementStanceAndWeaknessOfTableEightTwo) {
    for (const RoleSpec& spec : rolesSaySo()) {
        const RoleTemplate* role = app_.data().findRole(spec.roleId);
        ASSERT_NE(role, nullptr) << spec.roleId;
        EXPECT_EQ(role->realm, spec.realm) << spec.why;
        if (spec.element >= 0) EXPECT_EQ(role->element, spec.element) << spec.why;
        if (spec.toughness >= 0) EXPECT_EQ(role->toughness, spec.toughness) << spec.why;
        if (spec.weaknesses >= 0) EXPECT_EQ(role->weaknesses, spec.weaknesses) << spec.why;
    }
    // 黑木 weapons 剑（施工偏差 18.2 第 3 条：规则 25 按这一场我方必有手段算，友军兵刃算在内）。
    const RoleTemplate* heimu = app_.data().findRole("hei_mu");
    ASSERT_NE(heimu, nullptr);
    EXPECT_EQ(heimu->weapons, cats({"剑"}));
}

// 8.2 ② 的「顺序」与两个蓄势；③ 吴风的法术与行动；② 黄衣人冰锥连发。
TEST_F(Ch06BattleData, TheLifeOrDeathFightAndTheSparringCarryTheMovesTableEightTwoNames) {
    const RoleTemplate* huangyi = app_.data().findRole("huangyi_ren");
    const RoleTemplate* dahan = app_.data().findRole("tujia_dahan");
    const RoleTemplate* wufeng = app_.data().findRole("wu_feng");
    ASSERT_NE(huangyi, nullptr);
    ASSERT_NE(dahan, nullptr);
    ASSERT_NE(wufeng, nullptr);
    // 黄衣人：蓄势「葫芦黑球」每 3 回合、全体。
    EXPECT_EQ(huangyi->charge.every, 3) << "8.2 ②：黄衣人蓄势每 3 回合";
    EXPECT_TRUE(huangyi->charge.all) << "8.2 ②：葫芦黑球打全体";
    EXPECT_NE(app_.data().lookupText(huangyi->charge.textKey), huangyi->charge.textKey) << "蓄势预告那一句查不到";
    // 冰锥术三连发：数据里用的是一门水属性、蓄劲连发的法术（名字见测试路的回复）。
    bool icicles = false;
    for (const std::string& id : huangyi->magics) {
        const fanren::core::Magic* m = app_.data().findMagic(id);
        ASSERT_NE(m, nullptr) << id;
        icicles = icicles || (m->element == fanren::core::kElementWater && m->boost == fanren::core::MagicBoost::Hits);
    }
    EXPECT_TRUE(icicles) << "8.2 ②：黄衣人「冰锥术多段」——要一门水属性、能连发的法术";
    // 土甲大汉：蓄势「黑索缠身」每 3 回合、单体；引擎没有「不能动」，改成伤害 ×2 的重击；行动 1。
    EXPECT_EQ(dahan->charge.every, 3) << "8.2 ②：大汉蓄势每 3 回合";
    EXPECT_FALSE(dahan->charge.all) << "8.2 ②：黑索缠身是单体";
    EXPECT_DOUBLE_EQ(dahan->charge.mult, 2.0) << "8.2 ②：「改成伤害 ×2 的重击」";
    EXPECT_EQ(dahan->actions, 1) << "8.2 ②：大汉行动 1";
    EXPECT_TRUE(dahan->magics.empty()) << "8.2 ②：大汉靠土甲、巨力、黑索，不放法术";
    // 吴风：法术 飞剑术、火弹术；行动 1。
    const std::set<std::string> wufengMagics(wufeng->magics.begin(), wufeng->magics.end());
    EXPECT_EQ(wufengMagics, (std::set<std::string>{"magic_feijian_shu", "magic_huodan_shu"})) << "8.2 ③：飞剑术、火弹术";
    EXPECT_EQ(wufeng->actions, 1) << "8.2 ③：行动 1";
}

// 规则 25 的意思落到这一章（8.2 表下第一条、施工偏差 18.2 第 3 条）：每一个敌人至少一样破绽，是**那一刻**
// 我方必有的手段打得到的。手段照施工图写死：
//   ①（节点 6，9b 之前）：拳、软剑（剑）、火弹术（火）；友军兵刃算在内——青纹拳 ＋ 火球术（火）、黑木剑、熊大力刀
//     （校对整改 16.4 HIGH-4：原为吴九指拳、暗器）；
//   ②（节点 12，9b 之后、16a 之前）：拳、剑、火弹术、流沙术（土）、冰冻术（水）、祭剑符（金）；
//   ③（节点 16b，16a 之后）：②的全部 ＋ 冷月刀（刀）；
//   可选·吴九指（7 之后 11 之前）：至少有 ① 的韩立那一份；可选·外门（16a 之后）：同 ③。
TEST_F(Ch06BattleData, EveryFoeHasAWeaknessTheHandAtThatNodeCanReach) {
    const int before9b = cats({"拳", "剑", "火"});
    const int alliesAtOne = cats({"拳", "火", "剑", "刀"});
    const int after9b = before9b | cats({"土", "水", "金"});
    const int after16a = after9b | cats({"刀"});
    const std::vector<std::pair<const char*, int>> kMeans = {
        {"b06_yejia_xunxin", before9b | alliesAtOne}, {"b06_shanqiu_xisha", after9b}, {"b06_wufeng_qiecuo", after16a},
        {"b06_qiecuo_wujiuzhi", before9b},            {"b06_huangfenggu_qiecuo", after16a}};
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
    // 手段的来路本身（施工图 E5）：流沙术是土、冰冻术是水、祭剑符是金——上面那张表才站得住。
    for (const auto& [magicId, element] : std::vector<std::pair<const char*, int>>{
             {"magic_liusha_shu", fanren::core::kElementEarth},
             {"magic_bingdong_shu", fanren::core::kElementWater},
             {"magic_ji_jianfu", fanren::core::kElementMetal},
             {"magic_huodan_shu", fanren::core::kElementFire}}) {
        const fanren::core::Magic* m = app_.data().findMagic(magicId);
        ASSERT_NE(m, nullptr) << magicId;
        EXPECT_EQ(m->element, element) << magicId;
    }
    const auto* lengyue = app_.data().findItem("weapon_lengyue_dao");
    ASSERT_NE(lengyue, nullptr) << "施工图 E9：冷月刀";
}

// 施工图 10.1 E5：两门新的攻击法术的数值写死在施工图里（「数值归平衡路」指的是编成，法术这三格是 E5 原文）。
TEST_F(Ch06BattleData, TheTwoNewSpellsAreTheOnesE5Writes) {
    struct SpellSpec {
        const char* id;
        const char* name;
        int element;
        int power;
        int needMp;
    };
    const std::vector<SpellSpec> kSpells = {
        {"magic_liusha_shu", "流沙术", fanren::core::kElementEarth, 14, 12},
        {"magic_bingdong_shu", "冰冻术", fanren::core::kElementWater, 12, 10},
    };
    for (const SpellSpec& spec : kSpells) {
        const fanren::core::Magic* m = app_.data().findMagic(spec.id);
        ASSERT_NE(m, nullptr) << spec.id;
        EXPECT_EQ(m->name, spec.name);
        EXPECT_EQ(m->element, spec.element) << spec.id;
        EXPECT_EQ(m->power, spec.power) << spec.id << "：E5";
        EXPECT_EQ(m->needMp, spec.needMp) << spec.id << "：E5";
        EXPECT_EQ(m->boost, fanren::core::MagicBoost::Power) << spec.id << "：E5「boost power」";
        EXPECT_EQ(m->effect, fanren::core::MagicEffect::None) << spec.id;
    }
    // 祭剑符不动（E5「magic_ji_jianfu 不动」）：第 5 章那几格原样。
    const fanren::core::Magic* jianfu = app_.data().findMagic("magic_ji_jianfu");
    ASSERT_NE(jianfu, nullptr);
    EXPECT_EQ(jianfu->needMp, 40);
    EXPECT_EQ(jianfu->power, 40);
    // 18.2 第 2 条的算术：流沙四发 ＋ 剑符一发 ≤ 九层法力 90（施工图 1.3）。
    const fanren::core::Magic* liusha = app_.data().findMagic("magic_liusha_shu");
    const RoleTemplate* dahan = app_.data().findRole("tujia_dahan");
    ASSERT_NE(liusha, nullptr);
    ASSERT_NE(dahan, nullptr);
    EXPECT_LE(liusha->needMp * dahan->toughness + jianfu->needMp, fanren::rules::realmMaxMp(Realm::QiRefining9))
        << "施工偏差 18.2 第 2 条：破大汉的势再放剑符，九层的法力要够";
}

// 验收 4 的扫描器自检：判据表里的五张编成两两不同（否则 Ch06AcceptanceTests 那只手认不出是哪一场）。
TEST_F(Ch06BattleData, TheFiveCompositionsCanBeToldApartByTheirFoes) {
    std::set<std::multiset<std::string>> seen;
    for (const CompositionSpec& spec : designSaysSo()) {
        EXPECT_TRUE(seen.insert(spec.enemies).second) << spec.id << " 的敌方与另一张相同，通关测试认不出来";
    }
}

}  // namespace
