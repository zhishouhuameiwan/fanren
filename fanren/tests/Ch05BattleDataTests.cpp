// 第 5 章十一张编成与每个人的数——直接读 data，判据抄自施工图 8.2 / 8.3 / 8.4 原文（验收 4）。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件（第 4 章独立校对 MEDIUM-1）
// ---------------------------------------------------------------------------
// 第 4 章复核方把一张编成三波压成一波、把火弹术耗法从 8 改成 1，全套测试一条不红：
// 那些数被**用**着（通关测试要靠它们才走得完），却从来没有被**断言**过。
// 本文件与 tests/Ch05SliceTests.cpp 刻意不同形状：通关测试问「这一章走不走得完」，
// 这里问「这几个数是不是施工图写的那几个」。不起脚本、不打仗、不走位。
//
// 逐张按 id 钉（施工图 8.2 那张表点名写死的十一张），不写「所有的 b05_* 都……」：
// 那是普查，不是规则（docs/README.md「把内容形状当成规格」）。
//
// 8.3 的伤害四列（韩立平砍 / 火弹 / 他打韩立 / 他打曲魂）是施工图照公式算出来的字面量；
// 这里拿 core::battle 的公式**用数据里的数**再算一遍，与字面量比：数据改了、或公式改了，
// 都会在这里说话（与第 4 章 Ch04BattleData 钉「十发」那一句同一个办法）。
// 8.4 的「单轮最大伤害」一列同理——但**口径换了**（八方旅人化改造）：施工图当时的算法是
// **贴身四格**（棋盘上一个人最多被四个人同时贴身砍到），横版里没有格子，一波的人每回合都够得着
// 韩立，于是改成「这一波每个人、每一次行动都砍在他身上」；首领的重招是**预告过的**（蓄势那一句），
// 意图玩家看见就防御（受伤减半），所以重招那一回合按「全体减半、重招乘倍数」另算一列。
// 两列的字面量是按新口径手算的（docs/octopath-battle.md 7.3 那张表），这里照公式重算一遍比对。
// 2026-09-26 横版重调（B2）改了 8.2 / 8.3 的几行（②③ 拆波、⑥⑧⑨ 抬血、⑧ 输了不死、潇湘院压轻），
// 施工图那几张表与这里的字面量同一次改，理由写在施工图 8.4 表下「横版重调」那一段。
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/battle/Damage.h"
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

// ---------------------------------------------------------------------------
// 判据一：施工图 8.2 那张表（一行一张编成）
// ---------------------------------------------------------------------------
struct WaveSpec {
    int wave;
    const char* roleId;
    int count;
};

struct CompositionSpec {
    const char* mark;
    const char* id;
    bool mustFight;
    bool heroAbsent;
    std::vector<WaveSpec> allies;    // 编成里写死的 faction: ally（只有 ⑤ 有）
    std::vector<WaveSpec> enemies;
    int totalHp;                     // 敌方总血
    // 场子宽高（16×11 那一列）随横版改造删了：没有格子。
    bool canEscape;
    bool defeatIsFatal;
    int cultivation, spiritStones;   // 收获（修为 / 碎银）
    int maxRoundDamage;              // 新口径：一波里每个人每次行动都砍他（⑤ 打的是曲魂）
    int heavyRoundDamage;            // 新口径：重招那一回合、他防御着（没有会蓄势的首领为 0）
};

const std::vector<CompositionSpec>& designSaysSo() {
    static const std::vector<CompositionSpec> kSpecs = {
        {"①", "b05_yesu_yelang", true, false, {}, {{0, "wild_wolf", 6}}, 108, true, true, 10, 0, 6, 0},
        {"②", "b05_heishuixiang", true, false, {},
         {{0, "matou_dashou", 4}, {1, "matou_dashou", 4}, {1, "hei_xiong", 1}}, 146, false, true, 40, 0, 26, 0},
        {"③", "b05_matou_zhuishao", true, false, {},
         {{0, "tiequanhui_quanshi", 1}, {0, "matou_dashou", 2}, {1, "tiequanhui_quanshi", 1}, {1, "matou_dashou", 2}},
         128, false, true, 30, 10, 16, 0},
        {"④", "b05_tiequanhui_jieren", true, false, {},
         {{0, "tiequanhui_quanshi", 2}, {0, "tiequanhui_fushou", 1}}, 132, false, true, 40, 15, 35, 0},
        {"⑤", "b05_duobang", true, true, {{0, "qu_hun", 1}, {0, "matou_dashou", 2}},
         {{0, "matou_dashou", 4}, {1, "sipingbang_toumu", 1}, {1, "matou_dashou", 1}, {2, "sipingbang_toumu", 1}},
         118, false, false, 0, 0, 16, 0},
        {"⑥", "b05_yange_qiecuo", true, false, {}, {{0, "yan_ge", 1}}, 110, true, false, 30, 0, 13, 12},
        {"⑦", "b05_mofu_shigui", true, false, {}, {{0, "mofu_shigui", 1}}, 88, false, true, 90, 15, 34, 0},
        {"⑧", "b05_wu_jianming", true, false, {}, {{0, "wu_jianming", 1}}, 110, true, false, 60, 0, 20, 18},
        {"⑨", "b05_xunzhuang", true, false, {},
         {{0, "dubashanzhuang_zhuangding", 2}, {0, "dubashanzhuang_xuntou", 1}}, 142, false, true, 40, 0,
         34, 0},
        // ⑩ can_escape 假（复验 MEDIUM-B 方案 1）：只有持符的人进这一仗，逃了再来会拆掉「乘其不备」。
        {"⑩", "b05_ouyang_feitian", true, false, {}, {{0, "ouyang_feitian", 1}}, 90, false, true, 100, 0, 22, 22},
        {"条件", "b05_xiaoxiangyuan", false, false, {},
         {{0, "shen_san", 1}, {0, "fan_ju", 1}, {1, "shen_zhongshan", 1}, {1, "qian_jin", 1}}, 222, true,
         true, 60, 30, 21, 16},
    };
    return kSpecs;
}

// ---------------------------------------------------------------------------
// 判据二：施工图 8.3 那张表（一行一个人；-1 = 那一格是「—」）
// ---------------------------------------------------------------------------
struct RoleSpec {
    const char* roleId;
    Realm realm;
    int hp, attack, defence, speed;
    int hanliHit, fireBolt, hitsHanli, hitsQuhun;
};

const std::vector<RoleSpec>& rolesSaySo() {
    static const std::vector<RoleSpec> kRoles = {
        {"wild_wolf", Realm::Mortal, 18, 4, 3, 9, 43, 36, 1, 1},
        {"matou_dashou", Realm::Mortal, 14, 6, 4, 4, 42, 35, 3, 4},
        {"hei_xiong", Realm::Mortal, 34, 14, 8, 6, 35, 32, 14, 20},
        {"tiequanhui_quanshi", Realm::Mortal, 36, 11, 6, 5, 38, 34, 10, 14},
        {"tiequanhui_fushou", Realm::Mortal, 60, 15, 8, 6, 35, 32, 15, 22},
        {"sipingbang_toumu", Realm::Mortal, 24, 9, 6, 5, -1, -1, -1, 10},
        {"yan_ge", Realm::Mortal, 110, 13, 14, 8, 26, 27, 13, -1},
        {"mofu_shigui", Realm::QiRefining6, 88, 13, 11, 5, 21, 20, 17, -1},
        {"wu_jianming", Realm::Mortal, 110, 18, 10, 9, 32, 30, 20, -1},
        {"dubashanzhuang_zhuangding", Realm::Mortal, 36, 11, 6, 5, 38, 34, 10, -1},
        {"dubashanzhuang_xuntou", Realm::Mortal, 70, 14, 8, 6, 35, 32, 14, -1},
        {"ouyang_feitian", Realm::Mortal, 90, 20, 32, 4, 2, 13, 22, -1},
        {"shen_san", Realm::Mortal, 38, 11, 6, 4, 38, 34, 10, -1},
        {"fan_ju", Realm::Mortal, 38, 10, 6, 5, 38, 34, 8, -1},
        {"shen_zhongshan", Realm::Mortal, 86, 13, 12, 6, 29, 29, 13, -1},
        {"qian_jin", Realm::Mortal, 60, 10, 8, 3, 35, 32, 8, -1},
    };
    return kRoles;
}

// 施工图 8.1：韩立炼气八层 攻 15 / 防 8 / 气血 120 / 法力 80，速 5；火弹 power 12、耗 8。
// 曲魂：凡人 55 / 攻 9 / 防 8 / 速 2。
constexpr int kHanliAttack = 15;
constexpr int kHanliDefence = 8;
constexpr int kHanliHp = 120;
constexpr int kHanliMp = 80;
constexpr int kFirePower = 12;
constexpr int kFireNeedMp = 8;
constexpr int kQuhunHp = 55;
constexpr int kQuhunAttack = 9;
constexpr int kQuhunDefence = 8;
constexpr int kQuhunSpeed = 2;
// 施工图 8.4：斩杀线——单轮最大伤害不超过上限的三成（曲魂那一仗按曲魂的 55 算）。
constexpr int kKillLinePercent = 30;
// 施工图 8.2 ⑩ 从前钉的是「赏月亭开场距离 3 格（祭剑符 castRange 3，正好够得着）」。
// 横版没有距离，那一条作废；「乘其不备」落在先手上：身法 5 对 4，行动序的扰动是 ±10%
//（docs/octopath-battle.md 2.1），5 × 0.9 = 4.5 仍大于 4 × 1.1 = 4.4——扰动翻不过来。
constexpr int kSpeedJitterPercent = 10;
constexpr int kHanliSpeed = 5;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "data" / "battles" / "b05_duobang.json") &&
            fs::exists(root / "scripts" / "ch05" / "shuijiao.lua")) {
            return candidate;
        }
    }
    return ".";
}

class Ch05BattleData : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    const BattleSetup* setupOf(const char* id) { return app_.battleSetup(id); }
    const RoleTemplate* role(const std::string& id) { return app_.data().findRole(id); }

    // 韩立与曲魂在战场上的那几个数：韩立按境界曲线（Realm.cpp），曲魂按 data/roles。
    Realm hanliRealm() const { return Realm::QiRefining8; }

    Application app_;
};

// ---------------------------------------------------------------------------
// 先验：十一张都在，每张都真的有人；十六个人都在
// ---------------------------------------------------------------------------
TEST_F(Ch05BattleData, AllElevenCompositionsAndSixteenRolesAreReallyInTheData) {
    ASSERT_EQ(designSaysSo().size(), 11u) << "判据表的行数变了。先改 docs/ch05-design.md 8.2";
    ASSERT_EQ(rolesSaySo().size(), 16u) << "判据表的行数变了。先改 docs/ch05-design.md 8.3";
    int mustFight = 0;
    for (const CompositionSpec& spec : designSaysSo()) {
        const BattleSetup* setup = setupOf(spec.id);
        ASSERT_NE(setup, nullptr) << "data/battles 里没有 " << spec.id;
        ASSERT_FALSE(setup->units.empty()) << spec.id << " 一个人也没有";
        mustFight += spec.mustFight ? 1 : 0;
    }
    EXPECT_EQ(mustFight, 10) << "施工图 8.0：本章必打 10 场";
    for (const RoleSpec& spec : rolesSaySo()) {
        ASSERT_NE(role(spec.roleId), nullptr) << "data/roles 里没有 " << spec.roleId;
    }
}

// ---------------------------------------------------------------------------
// 正题一 · 波次与每一波的人（按 role_id 记数，不只记人数）
// ---------------------------------------------------------------------------
TEST_F(Ch05BattleData, EachCompositionHasExactlyTheWavesAndPeopleOfSectionEightTwo) {
    for (const CompositionSpec& spec : designSaysSo()) {
        const BattleSetup* setup = setupOf(spec.id);
        ASSERT_NE(setup, nullptr) << spec.id;
        std::map<std::pair<int, std::string>, int> enemies;
        std::map<std::pair<int, std::string>, int> allies;
        for (const BattleUnitSpec& unit : setup->units) {
            (unit.ally ? allies : enemies)[std::make_pair(unit.wave, unit.roleId)] += 1;
        }
        for (const WaveSpec& wave : spec.enemies) {
            const auto key = std::make_pair(wave.wave, std::string(wave.roleId));
            EXPECT_EQ(enemies[key], wave.count)
                << spec.mark << " " << spec.id << " 第 " << wave.wave << " 波的 " << wave.roleId << " 应有 "
                << wave.count << " 个（施工图 8.2）";
        }
        EXPECT_EQ(enemies.size(), spec.enemies.size()) << spec.mark << " " << spec.id << " 里有判据表没列过的敌人";
        for (const WaveSpec& wave : spec.allies) {
            const auto key = std::make_pair(wave.wave, std::string(wave.roleId));
            EXPECT_EQ(allies[key], wave.count)
                << spec.mark << " " << spec.id << " 的友军 " << wave.roleId << " 应有 " << wave.count << " 个";
        }
        EXPECT_EQ(allies.size(), spec.allies.size())
            << spec.mark << " " << spec.id << " 的友军与施工图 8.2 不符（我方除 ⑤ 外都是韩立 ＋ 队伍）";
    }
}

// 正题二 · 场子、逃跑、输赢、韩立在不在场（施工图 8.2 的 `can_escape` / `defeat_is_fatal` 两列与 E2）
TEST_F(Ch05BattleData, FieldEscapeDefeatAndHeroAbsenceAreWhatTheTableSays) {
    for (const CompositionSpec& spec : designSaysSo()) {
        const BattleSetup* setup = setupOf(spec.id);
        ASSERT_NE(setup, nullptr) << spec.id;
        EXPECT_EQ(setup->canEscape, spec.canEscape) << spec.mark << " " << spec.id << " 的 can_escape";
        EXPECT_EQ(setup->defeatIsFatal, spec.defeatIsFatal) << spec.mark << " " << spec.id << " 的 defeat_is_fatal";
        EXPECT_EQ(setup->heroAbsent, spec.heroAbsent)
            << spec.mark << " " << spec.id << " 的 hero_absent（施工图 10.1 E2：只有 ⑤ 夺帮韩立不在场）";
    }
    // ⑩ 赏月亭：乘其不备——韩立先手，而且行动序的扰动翻不过来（见 kSpeedJitterPercent）。
    const BattleSetup* ouyang = setupOf("b05_ouyang_feitian");
    ASSERT_NE(ouyang, nullptr);
    ASSERT_EQ(ouyang->units.size(), 1u);
    const RoleTemplate* him = role(ouyang->units[0].roleId);
    ASSERT_NE(him, nullptr);
    EXPECT_GT(kHanliSpeed * (100 - kSpeedJitterPercent), him->speed * (100 + kSpeedJitterPercent))
        << "施工图 8.3：速 4 是赏月毫无防备，韩立先手——扰动最坏的一次也不许翻过来";
    // 剑符是取他首级的唯一办法（施工图 3.2 12d、16.1 第 13 条，第 5 章校对 MEDIUM-1）：破绽只有「金」，
    // 也只有「金」要得了他的命——刀剑拳、火弹、毒，打得再重也只能把他压到一口气（规则层 Unit::killableBy）。
    EXPECT_EQ(him->weaknesses, fanren::core::kCategoryMetal) << "欧阳飞天的破绽只许有「金」";
    EXPECT_EQ(him->killableBy, fanren::core::kCategoryMetal) << "只有「金」要得了欧阳飞天的命";
    const fanren::core::Magic* talisman = app_.data().findMagic("magic_ji_jianfu");
    ASSERT_NE(talisman, nullptr);
    EXPECT_NE(fanren::core::categoriesOfElement(talisman->element) & him->killableBy, 0)
        << "祭剑符本身得是那一类：不然练成了也取不了首级";
}

// 正题三 · 总血与收获（施工图 8.2「总血」「收获（修为 / 碎银）」两列；掉落一律不写）
TEST_F(Ch05BattleData, TotalHpAndTheSpoilsAreWrittenInFull) {
    for (const CompositionSpec& spec : designSaysSo()) {
        const BattleSetup* setup = setupOf(spec.id);
        ASSERT_NE(setup, nullptr) << spec.id;
        int total = 0;
        for (const BattleUnitSpec& unit : setup->units) {
            if (unit.ally) continue;
            const RoleTemplate* r = role(unit.roleId);
            ASSERT_NE(r, nullptr) << unit.roleId;
            total += r->maxHp;
        }
        EXPECT_EQ(total, spec.totalHp) << spec.mark << " " << spec.id << " 的敌方总血";
        EXPECT_EQ(setup->reward.cultivation, spec.cultivation) << spec.mark << " " << spec.id << " 的修为";
        EXPECT_EQ(setup->reward.spiritStones, spec.spiritStones) << spec.mark << " " << spec.id << " 的碎银";
        EXPECT_TRUE(setup->reward.drops.empty()) << spec.mark << " " << spec.id << "：施工图 8.2「掉落一律不写」";
    }
}

// ---------------------------------------------------------------------------
// 正题四 · 每个人的数（施工图 8.3 前五列）与伤害四列（照公式重算）
// ---------------------------------------------------------------------------
TEST_F(Ch05BattleData, EveryRoleCarriesTheNumbersOfSectionEightThree) {
    for (const RoleSpec& spec : rolesSaySo()) {
        const RoleTemplate* r = role(spec.roleId);
        ASSERT_NE(r, nullptr) << spec.roleId;
        EXPECT_EQ(r->realm, spec.realm) << spec.roleId << " 的境界";
        EXPECT_EQ(r->maxHp, spec.hp) << spec.roleId << " 的血";
        EXPECT_EQ(r->attack, spec.attack) << spec.roleId << " 的攻";
        EXPECT_EQ(r->defence, spec.defence) << spec.roleId << " 的防";
        EXPECT_EQ(r->speed, spec.speed) << spec.roleId << " 的速";
        EXPECT_TRUE(r->magics.empty()) << spec.roleId << "：施工图 8.3 的对手全无法术（斩杀线只按普攻算的前提）";
    }
}

TEST_F(Ch05BattleData, TheFourDamageColumnsStillFollowFromTheFormulaAndTheData) {
    // 先验：韩立与曲魂的底数就是施工图 8.1 写的那几个。
    ASSERT_EQ(fanren::rules::realmAttack(hanliRealm()), kHanliAttack) << "施工图 8.1：炼气八层攻 15";
    ASSERT_EQ(fanren::rules::realmDefence(hanliRealm()), kHanliDefence) << "施工图 8.1：防 8";
    ASSERT_EQ(fanren::rules::realmMaxHp(hanliRealm()), kHanliHp) << "施工图 8.1：气血 120";
    ASSERT_EQ(fanren::rules::realmMaxMp(hanliRealm()), kHanliMp) << "施工图 8.1：法力 80";
    const fanren::core::Magic* fire = app_.data().findMagic("magic_huodan_shu");
    ASSERT_NE(fire, nullptr);
    ASSERT_EQ(fire->power, kFirePower) << "施工图 8.1：火弹 power 12";
    ASSERT_EQ(fire->needMp, kFireNeedMp) << "施工图 8.1：耗 8";
    const RoleTemplate* quhun = role("qu_hun");
    ASSERT_NE(quhun, nullptr);
    ASSERT_EQ(quhun->maxHp, kQuhunHp);
    ASSERT_EQ(quhun->attack, kQuhunAttack);
    ASSERT_EQ(quhun->defence, kQuhunDefence);
    ASSERT_EQ(quhun->speed, kQuhunSpeed);
    ASSERT_EQ(quhun->realm, Realm::Mortal);

    using fanren::core::battle::magicDamage;
    using fanren::core::battle::physicalDamage;
    for (const RoleSpec& spec : rolesSaySo()) {
        const RoleTemplate* r = role(spec.roleId);
        ASSERT_NE(r, nullptr) << spec.roleId;
        if (spec.hanliHit >= 0) {
            EXPECT_EQ(physicalDamage(kHanliAttack, r->defence, hanliRealm(), r->realm, 0, r->element), spec.hanliHit)
                << spec.roleId << "：韩立平砍一下（施工图 8.3）";
        }
        if (spec.fireBolt >= 0) {
            EXPECT_EQ(magicDamage(fire->power, r->defence, hanliRealm(), r->realm, fire->element, r->element),
                      spec.fireBolt)
                << spec.roleId << "：火弹一发（施工图 8.3）";
        }
        if (spec.hitsHanli >= 0) {
            EXPECT_EQ(physicalDamage(r->attack, kHanliDefence, r->realm, hanliRealm(), r->element, 0), spec.hitsHanli)
                << spec.roleId << "：他打韩立一下（施工图 8.3）";
        }
        if (spec.hitsQuhun >= 0) {
            EXPECT_EQ(physicalDamage(r->attack, quhun->defence, r->realm, quhun->realm, r->element, quhun->element),
                      spec.hitsQuhun)
                << spec.roleId << "：他打曲魂一下（施工图 8.3）";
        }
    }
}

// ---------------------------------------------------------------------------
// 正题五 · 斩杀线（施工图 8.4：单轮最大伤害 ≤ 上限三成）——横版新口径
// ---------------------------------------------------------------------------
// 一击的伤害走 core::battle 的公式与规则层同一套倍数：防御 ×0.5、重招 ×mult，四舍五入离零、至少 1。
int oneBlow(int base, bool guarding, double mult) {
    double damage = base;
    if (guarding) damage *= 0.5;
    damage *= mult;
    const auto rounded = static_cast<int>(std::lround(damage));
    return rounded < 1 ? 1 : rounded;
}

TEST_F(Ch05BattleData, NoRoundCanTakeMoreThanThreeTenthsOfTheTargetsLife) {
    const RoleTemplate* quhun = role("qu_hun");
    ASSERT_NE(quhun, nullptr);
    for (const CompositionSpec& spec : designSaysSo()) {
        const BattleSetup* setup = setupOf(spec.id);
        ASSERT_NE(setup, nullptr) << spec.id;
        // ⑤ 韩立不在场，挨打的是曲魂；其余挨打的是韩立。
        const bool onQuhun = spec.heroAbsent;
        const int targetDefence = onQuhun ? quhun->defence : kHanliDefence;
        const Realm targetRealm = onQuhun ? quhun->realm : hanliRealm();
        const int cap = onQuhun ? quhun->maxHp : kHanliHp;
        std::map<int, int> plain;    // 每一波：人人每次行动都砍他
        std::map<int, int> heavy;    // 每一波：有人出重招的那一回合，他防御着
        std::set<int> charged;       // 有会蓄势的首领的那几波
        for (const BattleUnitSpec& unit : setup->units) {
            if (unit.ally) continue;
            const RoleTemplate* r = role(unit.roleId);
            ASSERT_NE(r, nullptr) << unit.roleId;
            const int base = fanren::core::battle::physicalDamage(r->attack, targetDefence, r->realm, targetRealm,
                                                                  r->element, 0);
            plain[unit.wave] += oneBlow(base, false, 1.0) * r->actions;
            if (r->charge.every > 0) {
                charged.insert(unit.wave);
                heavy[unit.wave] += oneBlow(base, true, r->charge.mult) + oneBlow(base, true, 1.0) * (r->actions - 1);
            } else {
                heavy[unit.wave] += oneBlow(base, true, 1.0) * r->actions;
            }
        }
        int worst = 0;
        int worstHeavy = 0;
        for (const auto& entry : plain) worst = std::max(worst, entry.second);
        for (const int wave : charged) worstHeavy = std::max(worstHeavy, heavy[wave]);
        EXPECT_EQ(worst, spec.maxRoundDamage)
            << spec.mark << " " << spec.id << " 的单轮最大伤害（一波人人都砍他）与新口径那一列不符";
        EXPECT_EQ(worstHeavy, spec.heavyRoundDamage)
            << spec.mark << " " << spec.id << " 的重招回合（他防御着）与新口径那一列不符";
        EXPECT_LE(worst * 100, cap * kKillLinePercent)
            << spec.mark << " " << spec.id << "：单轮最大 " << worst << " 超过上限 " << cap
            << " 的三成（施工图 8.4 斩杀线规则；横版里一波人人都够得着他）";
        EXPECT_LE(worstHeavy * 100, cap * kKillLinePercent)
            << spec.mark << " " << spec.id << "：重招回合防御着还挨 " << worstHeavy << "，超过上限 " << cap
            << " 的三成——预告过的重招，防御了也该扛得住";
    }
}

// 墨府尸傀：遗留编成接入，「编成 id、单位、场子一律不动」（施工图 8.3 末段、验收 4）。
// 施工图写了 11×9、单位一个、收获 90 / 15、角色数值不动（上面几条已按表判过）。
// 11×9 那一句随横版改造作废（没有格子），其余照判。
TEST_F(Ch05BattleData, TheLegacyPuppetKeepsItsFieldAndItsOneUnit) {
    const BattleSetup* setup = setupOf("b05_mofu_shigui");
    ASSERT_NE(setup, nullptr);
    ASSERT_EQ(setup->units.size(), 1u);
    EXPECT_EQ(setup->units[0].roleId, "mofu_shigui");
    EXPECT_FALSE(setup->units[0].ally);
    const RoleTemplate* r = role("mofu_shigui");
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->realm, Realm::QiRefining6) << "施工图 8.3：Realm.h 注释拿它当炼气六层的数据点，数值不动";

    // 施工图 8.3：「data/encounters/village_wilds.json 把它列为野外随机遭遇……从这张表里删掉这一条」。
    // 扫整个遭遇目录（换一张表写进去也不行：墨府地窖里的东西不该在野地里撞上）。
    const auto readAll = [](const fs::path& path) {
        std::ifstream in(path, std::ios::binary);
        std::ostringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    };
    int tables = 0;
    for (const auto& entry : fs::directory_iterator(fs::path(assetRoot()) / "data" / "encounters")) {
        if (entry.path().extension() != ".json") continue;
        ++tables;
        EXPECT_EQ(readAll(entry.path()).find("mofu_shigui"), std::string::npos)
            << entry.path().filename().string() << " 还把墨府尸傀列为野外遭遇（施工图 8.3：删掉这一条）";
    }
    EXPECT_GT(tables, 0) << "先验：遭遇表读得到";
    // 两处 note 改成 8.3 那张表的由头：修仙者的遗物，封在马厩下的地窖。
    for (const fs::path& path : {fs::path(assetRoot()) / "data" / "roles" / "mofu_shigui.json",
                                 fs::path(assetRoot()) / "data" / "battles" / "b05_mofu_shigui.json"}) {
        const std::string body = readAll(path);
        ASSERT_GT(body.size(), 100u) << "先验：读得到 " << path.string();
        EXPECT_NE(body.find("修仙者"), std::string::npos) << path.filename().string() << " 的 note 没写 8.3 的由头";
        EXPECT_NE(body.find("地窖"), std::string::npos) << path.filename().string() << " 的 note 没写 8.3 的由头";
    }
}

}  // namespace
