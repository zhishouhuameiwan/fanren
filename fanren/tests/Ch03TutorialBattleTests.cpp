// 第 3 章三场战斗，各验一条（设计文档 docs/ch03-design.md 第 9 节验收第 3 条）：
//
//   一 · b03_gu_wai_elang（谷外饿狼，教学一）—— 可打赢，也可逃走。
//   二 · b03_andao_shishou（暗道僵兽，教学二）—— 这一场教物品与用毒。
//   三 · b03_shihai_duoshe（识海，两团）—— 第一团能吞掉；第二团必然逃脱，
//        战果按咬下的比例计。
//
// 这几条钉的是**玩家手上的那一局**，不是规则层。规则层已经有人钉过了，不重复造：
//   * tests/Ch03BattleKitTests.cpp  —— 法术/物品登记表、背包扣减、识海那条例外。
//   * tests/Ch03BattleMenuTests.cpp —— 菜单每一条的可否与禁用理由。
//   * tests/Ch03ShihaiTests.cpp     —— 吞噬、逃遁阈值、战果百分比的算术。
//   * tests/Ch03PoisonTests.cpp     —— 中毒状态本身的结算与显示。
//   * 本文件                        —— 玩家真按那几个键，这三场仗会走成什么样。
//
// ---------------------------------------------------------------------------
// 「玩家真按那几个键」是什么意思
// ---------------------------------------------------------------------------
// 两条纪律，少一条这几个用例就测不到玩家会遇到的那一局：
//
//   1. **回合序是真的。** 一律走 BattleScene::runToAllyTurn()，敌方由同一套 AI
//      走完。韩立的身法写死是 5，狼是 9、笼中兽是 10 —— 「开场第一个动的就是
//      玩家」在真实编成里根本不成立。
//   2. **出手走菜单。** 攻击/物品/防御/逃跑四项都经 openMenu + menuChoose，
//      也就是 issueFromMenu → issuePlayerAction 那一条唯一的出口。绕开它直接
//      喂 BattleState::apply，扣背包这件事就漏掉了（技术债 G-5）。
//
// 从前还有第三条「移动不结束回合」（玩家挪一步之后还能再出一次手，是他比 AI 多出来的
// 那一手）。八方旅人化改造之后没有格子、没有移动：每一手都是这一位的整个回合。
// 玩家比 AI 多出来的那一手换成了**破绽与蓄劲**——打哪一类、什么时候把劲一口气用掉。
//
// ---------------------------------------------------------------------------
// 两处如实记录，不在测试里替产品打圆场
// ---------------------------------------------------------------------------
//   * **韩立一条法术也不会**（技术债 G-4：data 里没有 hanli 这个 role，
//     GameState 也没有「已习得法术」）。所以教学二只教物品与用毒，本文件不写
//     任何「玩家施法」的断言 —— 写了就是红的。
//   * **尸毒爪放不放得出来**（技术债 C3-1）。战棋那几年它的射程 1 与近战同距，
//     而 decideAi 的挑选顺序是「近战 → 法术 → 移动」，于是它一次也放不出来：
//     玩家在这一场里中不了毒，清毒散成了一件永远用不上的东西，主控裁决把射程改成 2。
//     横版没有射程：敌方 AI「有伤人的法术、法力又够，就先放法术」
//    （docs/octopath-battle.md 第 3 节），僵兽开场第一手就放爪，上毒与解毒两个方向
//     在同一场里都教得成，下面 TheAntidoteLessonNowActuallyHappensInThisFight 那一条实跑钉住它。
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "core/battle/Battle.h"
#include "core/battle/Damage.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "ui/Widgets.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::Item;
using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::BattleState;
using fanren::core::battle::Unit;
using fanren::game::Application;
using fanren::game::BattleScene;
using fanren::rules::Realm;

constexpr const char* kElangBattle = "b03_gu_wai_elang";
constexpr const char* kBeastBattle = "b03_andao_shishou";
constexpr const char* kMindBattle = "b03_shihai_duoshe";

constexpr const char* kShixinId = "pill_shixin_san";
constexpr const char* kQingduId = "pill_qingdu_san";
constexpr const char* kQiduId = "pill_qidu_shui";

constexpr const char* kBeastId = "jiang_shou";      // 僵兽：防 9，砍不动的那一只
constexpr const char* kCagedId = "long_zhong_shou"; // 笼中兽：防 3，砍得动的那两只
constexpr const char* kSoulId = "mo_juren_yuanshen";
constexpr const char* kYuId = "yu_zitong";

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "battles" / "b03_gu_wai_elang.json")) {
            return candidate;
        }
    }
    return ".";
}

int firstUnitWithId(const BattleState& battle, const std::string& id) {
    for (std::size_t i = 0; i < battle.units().size(); ++i) {
        if (battle.units()[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

// 这一轮站在场上的敌人加起来能打掉多少。算的是引擎那一个 physicalDamage，
// 不另写一份 —— 另写一份的话，规则一改这里会悄悄算出一个对不上的数。
int incomingPerRound(const BattleState& battle, int actor) {
    const Unit& me = battle.units()[static_cast<std::size_t>(actor)];
    int total = 0;
    for (const Unit& foe : battle.units()) {
        if (!foe.alive() || foe.ally == me.ally) continue;
        total += fanren::core::battle::physicalDamage(foe.attack, me.defence, foe.realm, me.realm,
                                                      foe.element, me.element);
    }
    return total;
}

bool logHas(const BattleState& battle, const std::string& needle) {
    for (const std::string& line : battle.log()) {
        if (line.find(needle) != std::string::npos) return true;
    }
    return false;
}

int logCount(const BattleState& battle, const std::string& needle) {
    int hits = 0;
    for (const std::string& line : battle.log()) {
        if (line.find(needle) != std::string::npos) ++hits;
    }
    return hits;
}

// ---------------------------------------------------------------------------
// 识海那一场的战果判据
// ---------------------------------------------------------------------------
// 单独拎成具名判据，为的是能拿同一份判据去跑「故意摆错 → 判据确实说不」。
//
// 这一场最容易写错的断言是 `EXPECT_FALSE(won)` —— 它在「敌人带着伤跑了」与
// 「韩立输了」两种局面下都成立，而这两件事在剧情上天差地别（后者要走 game_over）。
// 所以判据必须同时问三件事：结局是哪一种、两团光球各自的下场、咬下了多少。
struct MindOutcome {
    BattlePhase phase = BattlePhase::Ongoing;
    bool firstBallDevoured = false;   // 第一团：吞掉了（死，且不是跑掉的）
    bool secondBallFled = false;      // 第二团：跑了（活着离场）
    bool secondBallKilled = false;
    int spoilsPercent = 0;
    int heroVolumeGrowth = 0;         // 韩立的体积长了多少
};

struct MindVerdict {
    bool ok = true;
    std::string why;
};

MindVerdict mindBattleVerdict(const MindOutcome& o) {
    const auto fail = [](std::string why) { return MindVerdict{false, std::move(why)}; };

    if (o.phase == BattlePhase::Lost) {
        return fail("这一场落在了「韩立输了」：那是 game_over 的那一条，"
                    "而原著里跑掉的是绿光球");
    }
    if (o.phase == BattlePhase::Won) {
        return fail("这一场落在了「全歼」：第二团是设计上打不死的，"
                    "落在 Won 说明逃遁那个落点没接上（技术债 G-2）");
    }
    if (o.phase != BattlePhase::EnemyFled) {
        return fail("这一场没走到「敌方逃遁」那个落点，实为编号 " +
                    std::to_string(static_cast<int>(o.phase)));
    }
    if (!o.firstBallDevoured) {
        return fail("第一团没被吞掉：原著写的是「靠体积轻易吞掉，很快结束」");
    }
    if (o.heroVolumeGrowth <= 0) {
        return fail("吞了却没长：体积即实力，吞噬必须让韩立变大，"
                    "否则第一场就只是一次普通的击杀");
    }
    if (o.secondBallKilled) {
        return fail("第二团被打死了：它必须是逃脱的");
    }
    if (!o.secondBallFled) {
        return fail("第二团既没死也没跑，还站在场上——那这一场根本没打完");
    }
    // 战果按咬下的比例计。原著是三分之一，引擎的逃遁阈值也是三分之一，
    // 所以下限就是 33；上限留一点余地给最后那一口咬得深的情形。
    if (o.spoilsPercent < 33) {
        return fail("咬下的比例只有 " + std::to_string(o.spoilsPercent) +
                    "%：原著是三分之一，逃遁阈值也是三分之一，不该更少");
    }
    if (o.spoilsPercent > 50) {
        return fail("咬下的比例到了 " + std::to_string(o.spoilsPercent) +
                    "%：它一被咬到三分之一就该脱开逃走，咬不了这么多");
    }
    return MindVerdict{};
}

// ---------------------------------------------------------------------------
// 夹具
// ---------------------------------------------------------------------------
class Ch03TutorialBattle : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        // 玩家走到本章三场仗跟前时的样子：第 2 章四年苦修的终点。
        //
        // 从前这里只写一行境界，气血就留在 GameState 的默认值 10 上——那正是缺陷 B
        // 的症状：突破只换一个编号，全作没有任何一处让 maxHp / maxMp 长过。现在
        // 境界给出基准（core/rules/Realm.h），真机上玩家推到炼气三层时属性是跟着
        // 上来的（修炼面板当场补齐；老存档由 v3→v4 迁移补齐），夹具得照着摆。
        //
        // 写成**字面量再当场与规则层对表**，不是直接调 realmMaxHp 填进去：下面
        // 每一条实测数字（几刀砍死、挨几轮、毒够不够）都建在这两个数上，曲线一旦
        // 调平衡，该有人重新量一遍这三场仗，而不是让断言自己跟着漂过去。
        state().realm = Realm::QiRefining3;
        state().hp = state().maxHp = 60;
        state().mp = state().maxMp = 30;
        ASSERT_EQ(state().maxHp, fanren::rules::realmMaxHp(Realm::QiRefining3))
            << "炼气三层该给的气血变了：这三场仗的实测数字要重新量一遍";
        ASSERT_EQ(state().maxMp, fanren::rules::realmMaxMp(Realm::QiRefining3));
    }

    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }

    // ---- 玩家的四个动作，一律经菜单 ----

    // 目标那一级里，某个单位对应的是第几行。
    // 两只野狼同名，靠名字是分不开的，所以这里调的是场景自己用的那个
    // buildTargetItems，按下标对齐 —— 判据与被测对象共用同一份列表。
    int targetRowFor(const BattleScene& scene, ActionKind kind, const std::string& itemId,
                     int actor, int target) {
        Action shape;
        shape.kind = kind;
        shape.actorIndex = actor;
        shape.magicId = itemId;
        std::vector<int> indices;
        static_cast<void>(BattleScene::buildTargetItems(state(), scene.battle(), shape, indices));
        for (std::size_t i = 0; i < indices.size(); ++i) {
            if (indices[i] == target) return static_cast<int>(i);
        }
        return -1;
    }

    int itemRowFor(const BattleScene& scene, int actor, const std::string& itemId) {
        std::vector<std::string> ids;
        static_cast<void>(
            BattleScene::buildItemItems(app_.data(), state(), scene.battle(), actor, ids));
        for (std::size_t i = 0; i < ids.size(); ++i) {
            if (ids[i] == itemId) return static_cast<int>(i);
        }
        return -1;
    }

    bool playerAttacks(BattleScene& scene, int actor, int target) {
        scene.openMenu(app_);
        if (!scene.menuChoose(app_, fanren::game::kBattleMenuAttack)) return false;
        const int row = targetRowFor(scene, ActionKind::Attack, std::string{}, actor, target);
        return row >= 0 && scene.menuChoose(app_, row);
    }

    bool playerDefends(BattleScene& scene) {
        scene.openMenu(app_);
        return scene.menuChoose(app_, fanren::game::kBattleMenuDefend);
    }

    bool playerRunsAway(BattleScene& scene) {
        scene.openMenu(app_);
        return scene.menuChoose(app_, fanren::game::kBattleMenuEscape);
    }

    bool playerThrows(BattleScene& scene, int actor, const std::string& itemId, int target) {
        scene.openMenu(app_);
        if (!scene.menuChoose(app_, fanren::game::kBattleMenuItem)) return false;
        const int row = itemRowFor(scene, actor, itemId);
        if (row < 0) return false;
        if (!scene.menuChoose(app_, row)) return false;
        const int targetRow = targetRowFor(scene, ActionKind::Item, itemId, actor, target);
        return targetRow >= 0 && scene.menuChoose(app_, targetRow);
    }

    // 场上最容易砍死的那一个敌人（横版里人人够得着人人）；一个也没有返回 -1。
    int weakestFoe(const BattleState& battle, int actor) {
        int best = -1;
        int bestEndurance = 0;
        for (std::size_t i = 0; i < battle.units().size(); ++i) {
            Action probe;
            probe.kind = ActionKind::Attack;
            probe.actorIndex = actor;
            probe.targetIndex = static_cast<int>(i);
            if (!battle.isLegal(probe)) continue;
            const int endurance = battle.units()[i].hp;
            if (best < 0 || endurance < bestEndurance) {
                best = static_cast<int>(i);
                bestEndurance = endurance;
            }
        }
        return best;
    }

    // 一个会打仗的玩家：撑不过这一轮就先防御，撑得过就动手。
    //
    // ---- 这一段改写过两次 ----
    // 第一次（护体罡气有了失效时机之后）：从前的打法是「先把罡气攒到够撑完剩下的仗，
    // 再动手」，罡气只护到下一次出手之后那个目标永远达不到，照旧写下去的结果是韩立
    // 一直运功、一刀不砍。换成一条**不依赖累积**的判据：「这一轮挨下来还活不活得成」。
    // 第二次（八方旅人化改造）：罡气换成了防御减半，那一问原样保留；「够不着就挪过去」
    // 那一支随格子一起删了。
    void playSensibly(BattleScene& scene, int maxTurns) {
        for (int turn = 0; turn < maxTurns; ++turn) {
            const int actor = scene.runToAllyTurn();
            if (actor < 0) return;   // 战斗已经分出胜负

            const int threat = incomingPerRound(scene.battle(), actor);
            const Unit& me = scene.battle().units()[static_cast<std::size_t>(actor)];
            if (threat > 0 && me.hp <= threat) {
                if (playerDefends(scene)) continue;
            }

            const int target = weakestFoe(scene.battle(), actor);
            if (target >= 0) {
                if (playerAttacks(scene, actor, target)) continue;
            }
            if (!playerDefends(scene)) return;   // 一件也做不了，别空转
        }
    }

    // ---- 暗道试兽那一场的实测账目 ----
    //
    // 「砍不动就下毒」这一课在罡气有了失效时机之后还成不成立，只能实跑来答。
    // 同一场仗跑两趟：一趟只用刀，一趟先下毒再补刀，两边的账分开记。
    struct BeastRun {
        int swings = 0;          // 砍在僵兽身上的刀数
        int swingDamage = 0;     // 这些刀真砍进去多少
        int poisonDamage = 0;    // 剩下的那部分气血损失，来源只可能是毒
        int beastBreaks = 0;     // 僵兽破势几次（破绽是毒与火：撒一包毒就是一击「毒」）
        int dosesUsed = 0;
        int beastHp = 0;
        bool beastFelled = false;
        int heroHp = 0;
        int heroMaxHp = 0;
        int round = 0;
        BattlePhase phase = BattlePhase::Ongoing;
        // ---- 尸毒爪射程从 1 改成 2（主控裁决 C3-1）之后新记的几笔 ----
        int clawCasts = 0;          // 僵兽放出尸毒爪的次数
        bool heroPoisoned = false;  // 韩立中过毒没有
        int heroFirstPoisonRound = 0;
        int antidotesUsed = 0;
        bool heroCured = false;
        int guards = 0;             // 防御过几个回合
    };

    // 一趟仗的打法。三条开关分别对应一个**玩家会不会想到**的着数，
    // 全关就是最笨的打法，全开就是一个会打仗的玩家。
    // 写成开关而不是三份拷贝，是为了让三趟之间的差别只有这几行，好归因。
    struct BeastPlan {
        bool usePoison = false;      // 撒蚀心散
        bool cullTheQuickOnesFirst = false;  // 先清掉两只打得动的笼中兽
        bool guardWhenLethal = false;        // 这一轮撑不住就先防御
        bool cureWhenLethal = false;         // 毒会要命时才吃清毒散
    };

    // 挑一个打。cullFirst 为真时先挑笼中兽——它们防 3、两刀一只，
    // 而且两只加起来每轮 18 点，是这一场真正的伤害来源；僵兽防 9、身法 3，
    // 是个慢的。先清快的再磨慢的，是这一场唯一一条站得住的结论。
    int pickTarget(const BattleState& battle, int me, bool cullFirst, int beastIndex) {
        if (cullFirst) {
            int best = -1;
            int bestEndurance = 0;
            for (std::size_t i = 0; i < battle.units().size(); ++i) {
                if (static_cast<int>(i) == beastIndex) continue;
                Action probe;
                probe.kind = ActionKind::Attack;
                probe.actorIndex = me;
                probe.targetIndex = static_cast<int>(i);
                if (!battle.isLegal(probe)) continue;
                const int endurance = battle.units()[i].hp;
                if (best < 0 || endurance < bestEndurance) {
                    best = static_cast<int>(i);
                    bestEndurance = endurance;
                }
            }
            if (best >= 0) return best;
        }
        Action probe;
        probe.kind = ActionKind::Attack;
        probe.actorIndex = me;
        probe.targetIndex = beastIndex;
        if (battle.isLegal(probe)) return beastIndex;
        return weakestFoe(battle, me);
    }

    // 从头打一场暗道试兽。
    //
    // 最笨那一趟（开关全关）的打法是：**一头扎向僵兽**，砍不了它才打最弱的，
    // 什么都做不了才防御。它是对照组，不是推荐打法。
    BeastRun runBeastFight(const BeastPlan& plan) {
        const bool usePoison = plan.usePoison;
        const fanren::core::Item* dose = app_.data().findItem(kShixinId);
        const fanren::core::Item* cure = app_.data().findItem(kQingduId);
        BeastRun run;
        if (dose == nullptr || cure == nullptr) return run;

        // 背包与气血回到节点 7「备毒」取第一项之后的样子：上面的用例已经撒掉过一包。
        const int inBag = state().itemCount(kShixinId);
        if (inBag < 2) state().addItem(kShixinId, 2 - inBag);
        const int cures = state().itemCount(kQingduId);
        if (cures < 3) state().addItem(kQingduId, 3 - cures);
        state().hp = state().maxHp;
        state().mp = state().maxMp;

        BattleScene scene(kBeastBattle);
        scene.onEnter(app_);
        const int beast = firstUnitWithId(scene.battle(), kBeastId);
        if (beast < 0) return run;
        const auto beastIdx = static_cast<std::size_t>(beast);
        const int beastFullHp = scene.battle().units()[beastIdx].maxHp;

        for (int turn = 0; turn < 160; ++turn) {
            if (scene.battle().phase() != BattlePhase::Ongoing) break;
            if (!scene.battle().units()[beastIdx].alive()) break;
            const int me = scene.runToAllyTurn();
            if (me < 0) break;
            const auto meIdx = static_cast<std::size_t>(me);

            // 零 · 中毒记一笔。记的时机是**我方回合开头**，与玩家看见状态栏同步。
            if (scene.battle().units()[meIdx].poison > 0 && !run.heroPoisoned) {
                run.heroPoisoned = true;
                run.heroFirstPoisonRound = scene.battle().round();
            }

            // 一 · 毒要命了才吃清毒散。
            //
            // **不写成「一中毒就解」**：解一次要整整一个回合，而一包尸毒爪的毒
            // 总共 2 × 4 = 8 点，同样一个回合砍笼中兽是 14 点。一中毒就解的打法
            // 三个回合全花在喝药上，这一场必输——那不是清毒散没用，是用错了时候。
            // 判据：这一轮的毒加上敌人的一轮输出，能不能把他打到 0。
            if (plan.cureWhenLethal) {
                const Unit& hero = scene.battle().units()[meIdx];
                const int fromPoison = hero.poison * hero.poisonPower;
                const int fromFoes = incomingPerRound(scene.battle(), me);
                if (hero.poison > 0 && state().itemCount(kQingduId) > 0 &&
                    hero.hp <= fromPoison + fromFoes) {
                    if (playerThrows(scene, me, kQingduId, me)) {
                        ++run.antidotesUsed;
                        if (scene.battle().units()[meIdx].poison == 0) run.heroCured = true;
                        continue;
                    }
                }
            }

            // 二 · 这一轮撑不住就先防御。守势只护到自己下一次出手，所以问的就是这一轮。
            if (plan.guardWhenLethal) {
                const Unit& hero = scene.battle().units()[meIdx];
                const int threat = incomingPerRound(scene.battle(), me) +
                                   hero.poison * hero.poisonPower;
                if (threat > 0 && hero.hp <= threat) {
                    if (playerDefends(scene)) {
                        ++run.guards;
                        continue;
                    }
                }
            }

            // 三 · 毒散尽了就补一包（还有存货就补；横版没有「够不够得着」）。
            if (usePoison && scene.battle().units()[beastIdx].poison == 0 &&
                state().itemCount(kShixinId) > 0) {
                if (playerThrows(scene, me, kShixinId, beast)) {
                    ++run.dosesUsed;
                    continue;
                }
            }

            // 四 · 挑一个打。笨打法先挑僵兽，会打的先清掉两只快的。
            const int target = pickTarget(scene.battle(), me, plan.cullTheQuickOnesFirst, beast);
            if (target >= 0) {
                const int hpBefore = scene.battle().units()[beastIdx].hp;
                if (playerAttacks(scene, me, target)) {
                    if (target == beast) {
                        ++run.swings;
                        run.swingDamage += hpBefore - scene.battle().units()[beastIdx].hp;
                    }
                    continue;
                }
            }
            if (!playerDefends(scene)) break;
            ++run.guards;
        }

        const Unit& beastAtEnd = scene.battle().units()[beastIdx];
        run.beastHp = beastAtEnd.hp;
        run.beastFelled = !beastAtEnd.alive();
        // 僵兽的气血只会被两样东西动到：玩家的刀，与每回合开头结算的毒。
        run.poisonDamage = (beastFullHp - beastAtEnd.hp) - run.swingDamage;
        run.clawCasts = logCount(scene.battle(), "尸毒爪");
        for (const fanren::core::battle::BattleEvent& event : scene.battle().events()) {
            if (event.kind == fanren::core::battle::BattleEventKind::Break && event.target == beast) {
                ++run.beastBreaks;
            }
        }
        if (scene.battle().units()[0].poison > 0) run.heroPoisoned = true;
        run.heroHp = scene.battle().units()[0].hp;
        run.heroMaxHp = scene.battle().units()[0].maxHp;
        run.round = scene.battle().round();
        run.phase = scene.battle().phase();
        return run;
    }

    Application app_;
};

// ===========================================================================
// 一 · 谷外饿狼：可打赢
// ===========================================================================
TEST_F(Ch03TutorialBattle, TheWolvesGoDownIfThePlayerActuallyPlaysIt) {
    BattleScene scene(kElangBattle);
    scene.onEnter(app_);

    // 先验：这一场确实是两只狼，而且确实允许逃 —— 下一条用例要用到后半句。
    ASSERT_EQ(scene.battle().units().size(), 3u) << "韩立加两只狼";
    ASSERT_EQ(firstUnitWithId(scene.battle(), "hanli"), 0) << "主角永远是 0 号";
    const int hpAtStart = scene.battle().units()[0].hp;
    ASSERT_GT(hpAtStart, 0);

    playSensibly(scene, /*maxTurns=*/120);

    EXPECT_EQ(scene.battle().phase(), BattlePhase::Won)
        << "会打的玩家该拿得下这一场：狼的破绽里有「拳」，空手就打得出破势";
    for (const Unit& unit : scene.battle().units()) {
        if (unit.ally) continue;
        EXPECT_FALSE(unit.alive()) << unit.name << " 还站着，这一场却算赢了";
    }
    EXPECT_TRUE(scene.battle().units()[0].alive()) << "赢了的人得活着";

    std::cout << "\n[ch03 教学一实测] 打赢用了 " << scene.battle().round() << " 回合，"
              << "韩立余血 " << scene.battle().units()[0].hp << "/"
              << scene.battle().units()[0].maxHp << "，劲 "
              << scene.battle().units()[0].bp << std::endl;
}

// ===========================================================================
// 一 · 谷外饿狼：也可逃走
// ===========================================================================
TEST_F(Ch03TutorialBattle, TheWolvesCanAlsoJustBeWalkedAwayFrom) {
    BattleScene scene(kElangBattle);
    scene.onEnter(app_);

    // 先验：这一场的「逃跑」那一项是亮着的，而且理由是空的。
    // 识海那一场同一项写着「此战无法回避」（tests/Ch03BattleMenuTests.cpp 钉过），
    // 两相对照，「可逃」才是这一场自己的性质，不是所有仗都这样。
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0);
    const std::vector<fanren::ui::ListItem> root =
        BattleScene::buildRootItems(app_.data(), state(), scene.battle(), actor);
    ASSERT_GT(root.size(), static_cast<std::size_t>(fanren::game::kBattleMenuEscape));
    const fanren::ui::ListItem& escapeRow =
        root[static_cast<std::size_t>(fanren::game::kBattleMenuEscape)];
    EXPECT_TRUE(escapeRow.enabled) << escapeRow.disabledReason;
    EXPECT_TRUE(escapeRow.disabledReason.empty());

    // 逃跑是掷骰的（身法差得越多越难逃），一次不成就再来一次 —— 玩家也是这么干的。
    // 种子由战斗 id 派生，所以这个循环的次数是可复现的，不是碰运气。
    int attempts = 0;
    for (int turn = 0; turn < 24; ++turn) {
        const int me = scene.runToAllyTurn();
        if (me < 0) break;
        ++attempts;
        if (!playerRunsAway(scene)) break;
        if (scene.battle().phase() != BattlePhase::Ongoing) break;
    }

    EXPECT_EQ(scene.battle().phase(), BattlePhase::Escaped)
        << "设计写明教学一可逃（第 2 节那张表），" << attempts << " 次都没走脱";
    // 「逃走了」不等于「打赢了」，也不等于「输了」：三者是三个不同的落点，
    // 而 elang.lua 只分「赢」与「没赢」两档 —— 逃与败走同一条台词。
    EXPECT_NE(scene.battle().phase(), BattlePhase::Won);
    EXPECT_NE(scene.battle().phase(), BattlePhase::Lost);
    bool anyWolfLeft = false;
    for (const Unit& unit : scene.battle().units()) {
        if (!unit.ally && unit.alive()) anyWolfLeft = true;
    }
    EXPECT_TRUE(anyWolfLeft) << "走的是人，狼还在那儿";

    std::cout << "\n[ch03 教学一实测] 第 " << attempts << " 次才脱身" << std::endl;
}

// ===========================================================================
// 一 · 同一场仗交给自动驾驶，赢不下来
// ===========================================================================
// 整章走一趟时战斗在无头模式下是自动打完的（BattleScene::update 的那一支），
// 这一条钉的就是那一趟会落在哪儿 —— 通关测试里「谷外那一仗打赢了 / 退了」那个
// 分支断言的来处。
//
// ===== 这一条的结论反过来了，原因写清楚 =====
// 曾用名：TheAutopilotDoesNotWinTheSameFight。那时断言的是「自动驾驶赢不下来」，
// 依据是 decideAi 第一条：血掉到四分之一就掉头跑。那条依据在韩立只有 10 点气血
// 的年代必然成立 —— 两只狼一轮打掉 8 点，第一轮结束他就到线了。
//
// 境界给出气血基准之后（core/rules/Realm.h），炼气三层的韩立有 60 点：两只狼
// 一轮 8 点，而他一刀 14 点、两刀放倒一只，整场挨不到 15 点那条线就已经打完了。
// 于是自动驾驶**现在赢得下来**。这不是数值失控，这正是缺陷 B 修好之后该有的样子
// ——「修为让他变强了」在这一场上第一次看得见。
//
// 所以断言改成正面钉住那条因果，而不是只把 EXPECT_FALSE 翻成 EXPECT_TRUE：
// 赢，而且**全程没跌到逃跑线以下**。后者才是解释「为什么赢」的那一句，
// 也是气血一旦被改小就会先红的那一句。
//
// 八方旅人化改造之后我方 AI 不再有「掉到四分之一就跑」那一条（docs/octopath-battle.md
// 第 3 节），四分之一这条线留作尺子：全程没跌破它，才说得上赢是靠境界给的那点底子。
TEST_F(Ch03TutorialBattle, TheAutopilotNowWinsBecauseHisRealmFinallyGivesHimHp) {
    BattleScene scene(kElangBattle);
    scene.onEnter(app_);

    // 从前 decideAi 的逃跑线：hp * 4 <= maxHp（如今只当尺子用）。逐回合盯着它，别只看结局。
    int lowest = scene.battle().units()[0].hp;
    const int fleeLine = scene.battle().units()[0].maxHp / 4;
    for (int guard = 0; guard < 200 && scene.battle().phase() == BattlePhase::Ongoing; ++guard) {
        scene.runToCompletion(1);
        lowest = std::min(lowest, scene.battle().units()[0].hp);
    }

    EXPECT_EQ(scene.battle().phase(), BattlePhase::Won)
        << "自动驾驶没能拿下教学一，实为编号 " << static_cast<int>(scene.battle().phase());
    ASSERT_GT(fleeLine, 0) << "先验：逃跑线不能是 0，否则下面那条恒真";
    EXPECT_GT(lowest, fleeLine)
        << "他最低掉到 " << lowest << " 点，已经跌破四分之一那条线 " << fleeLine
        << " —— 那这一场赢得靠运气，不是靠境界给的那点底子";

    std::cout << "\n[ch03 教学一·自动驾驶实测] 第 " << scene.battle().round()
              << " 回合收场，韩立最低掉到 " << lowest << "/"
              << scene.battle().units()[0].maxHp << "（逃跑线 " << fleeLine << "）"
              << std::endl;
}

// ===========================================================================
// 二 · 暗道僵兽：这一场教物品与用毒
// ===========================================================================
TEST_F(Ch03TutorialBattle, TheBeastThatCannotBeCutDownGoesDownToPoison) {
    const Item* poison = app_.data().findItem(kShixinId);
    const Item* antidote = app_.data().findItem(kQingduId);
    ASSERT_NE(poison, nullptr);
    ASSERT_NE(antidote, nullptr);

    // 节点 7「备毒」取第一项之后，玩家身上正是这三样。数目与 beidu.lua 对齐。
    state().addItem(kShixinId, 2);
    state().addItem(kQingduId, 3);
    state().addItem(kQiduId, 1);

    BattleScene scene(kBeastBattle);
    scene.onEnter(app_);

    const int beast = firstUnitWithId(scene.battle(), kBeastId);
    const int caged = firstUnitWithId(scene.battle(), kCagedId);
    ASSERT_GE(beast, 0) << "这一场得有那只砍不动的僵兽";
    ASSERT_GE(caged, 0) << "也得有笼中兽当对照";

    // ---- 先验一：这一课的前提是「砍不动」。前提没了，这一整条就没意义 ----
    // 判据用引擎那一个 physicalDamage，不自己另写一份。
    const Unit& hero = scene.battle().units()[0];
    const Unit& beastUnit = scene.battle().units()[static_cast<std::size_t>(beast)];
    const Unit& cagedUnit = scene.battle().units()[static_cast<std::size_t>(caged)];
    const int swingAtBeast = fanren::core::battle::physicalDamage(
        hero.attack, beastUnit.defence, hero.realm, beastUnit.realm, hero.element,
        beastUnit.element);
    const int swingAtCaged = fanren::core::battle::physicalDamage(
        hero.attack, cagedUnit.defence, hero.realm, cagedUnit.realm, hero.element,
        cagedUnit.element);
    ASSERT_GT(swingAtCaged, 0);
    ASSERT_GE(beastUnit.hp, swingAtBeast * 5)
        << "僵兽 " << beastUnit.hp << " 点气血，一刀 " << swingAtBeast
        << " 点，五刀之内就砍完的话，「砍不动就下毒」这一课根本立不住";
    EXPECT_LT(swingAtBeast * 2, swingAtCaged)
        << "僵兽与笼中兽挨刀的差别不够大，玩家自己得不出结论";

    // ---- 先验二：一包毒真的比刀管用 ----
    const int poisonTotal = poison->poison * poison->poisonPower;
    EXPECT_GT(poisonTotal, swingAtBeast * poison->poison)
        << "撒一包毒三回合掉 " << poisonTotal << " 点，同样三回合砍三刀掉 "
        << swingAtBeast * poison->poison << " 点 —— 毒不比刀强的话，这一课白教";

    // ---- 先验三：这一场确实把背包摆上了桌 ----
    // 识海那一场一件都不登记（那是明写的例外）；这一场必须登记得上，
    // 否则下面「撒一包」测的就只是一条走不到的路。
    EXPECT_TRUE(scene.battle().hasItem(kShixinId)) << "蚀心散该在这一场的物品表里";
    EXPECT_TRUE(scene.battle().hasItem(kQingduId)) << "清毒散也该在";
    // 七毒水同样登记得上（它带着 poison 字段，core::battleUsable 就收）。
    // 这不是错：登记表的口径是「背包里战斗中真有效果的那些」，它确实有效果。
    // 但它是章末处决余子童要用的那一瓶——玩家在这里把它扔出去，节点 11 就会走
    // chujue.lua 的 nopoison 那一支。脚本接得住，所以下面只如实钉住这个事实，
    // 并且本用例自己一滴也不动它。
    EXPECT_TRUE(scene.battle().hasItem(kQiduId))
        << "七毒水带着 poison 字段，登记表收得下它——这一条钉的是「引擎确实会把"
           "章末那瓶毒摆上桌」，将来若要把它挡在战斗外，要红的正是这里";

    // ---- 轮到我方就撒 ----
    // 战棋那几年这里要先走到三格之内（蚀心散的使用距离）。横版没有距离。
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0) << "轮不到我方出手，仗就打完了";
    ASSERT_EQ(scene.battle().units()[static_cast<std::size_t>(beast)].poison, 0)
        << "撒之前它不该已经中毒";

    // ---- 撒一包 ----
    const int beastHpBeforePoison = scene.battle().units()[static_cast<std::size_t>(beast)].hp;
    ASSERT_TRUE(playerThrows(scene, actor, kShixinId, beast))
        << "从菜单里撒不出去：" << scene.feedback();
    EXPECT_EQ(state().itemCount(kShixinId), 1)
        << "撒出去的那一包必须真的从背包里没了 —— 这是物品那一课的全部内容";

    const Unit& poisoned = scene.battle().units()[static_cast<std::size_t>(beast)];
    EXPECT_EQ(poisoned.poison, poison->poison);
    EXPECT_EQ(poisoned.poisonPower, poison->poisonPower);
    EXPECT_EQ(poisoned.hp, beastHpBeforePoison) << "撒毒当场不掉血，掉的是往后每一轮";
    // 撒毒也是一击「毒」（docs/octopath-battle.md 2.7）：僵兽的破绽是毒与火，这一包揭开「毒」。
    EXPECT_NE(poisoned.revealed & fanren::core::kCategoryPoison, 0)
        << "撒了毒，僵兽的破绽「毒」却没揭开——「砍不动就下毒」在横版里也落在破绽上";
    // 中毒必须在界面上看得出来（契约第 2.2 节）：玩家掉血却不知道为什么，
    // 一律会被当成 bug。先验它真有内容，再验两个数都写在里面。
    const std::string shown = fanren::game::poisonStatusText(poisoned);
    ASSERT_FALSE(shown.empty());
    EXPECT_NE(shown.find(std::to_string(poison->poison)), std::string::npos) << shown;
    EXPECT_NE(shown.find(std::to_string(poison->poisonPower)), std::string::npos) << shown;

    // ---- 毒不过防：这才是「砍不动就下毒」成立的算术根据 ----
    // 下一轮开头结算一次毒。掉的必须正好是 poisonPower，与它那 9 点防无关。
    const int hpBeforeTick = scene.battle().units()[static_cast<std::size_t>(beast)].hp;
    const int roundBefore = scene.battle().round();
    for (int guard = 0; guard < 16 && scene.battle().round() == roundBefore &&
                        scene.battle().phase() == BattlePhase::Ongoing;
         ++guard) {
        const int me = scene.runToAllyTurn();
        if (me < 0) break;
        if (!playerDefends(scene)) break;
    }
    const Unit& afterTick = scene.battle().units()[static_cast<std::size_t>(beast)];
    // 不写成 if (alive()) 再断言：那样一旦它死了，下面两条就静悄悄地不跑了。
    // 毒不杀人（kPoisonFloorHp 留一口气），所以它到这一刻必然还站着。
    ASSERT_TRUE(afterTick.alive()) << "毒把它毒死了？kPoisonFloorHp 说不该";
    EXPECT_EQ(hpBeforeTick - afterTick.hp, poison->poisonPower)
        << "毒每轮掉的该正好是 " << poison->poisonPower << " 点，不受那 "
        << beastUnit.defence << " 点防护的折扣";
    EXPECT_GT(poison->poisonPower, swingAtBeast)
        << "毒一轮掉的还不如砍一刀，玩家没有理由改用毒";
    EXPECT_TRUE(logHas(scene.battle(), "毒发")) << "整场打下来没有一次毒发";

    // ---- 没中毒时清毒散点不动，而且**点不动就不会少** ----
    //
    // 这一条钉的是一处静默失败：applyItem 对「解毒药用在没中毒的人身上」什么也
    // 不做、照样成功返回，而 issuePlayerAction 只看 result.ok 就扣掉一件——
    // 点一下，东西没了，屏幕上一句「并无变化」。从前这里断言它是**亮的**，
    // 等于把那处静默失败钉成了正确行为。
    //
    // 另开一场空白的仗来验，不接着上面这一局：尸毒爪射程改成 2 之后，韩立在
    // 这一局的这一刻很可能**已经中毒了**，那时「没中毒该置灰」就无从谈起。
    // 用第一个我方回合，此刻他必然干净。
    {
        BattleScene fresh(kBeastBattle);
        fresh.onEnter(app_);
        const int me = fresh.runToAllyTurn();
        ASSERT_GE(me, 0) << "轮不到我方出手，下面整段都验不成——不许静悄悄地跳过去";
        ASSERT_EQ(fresh.battle().units()[static_cast<std::size_t>(me)].poison, 0)
            << "先验：韩立此刻确实没中毒，否则下面那条禁用断言说的是另一回事";

        const int row = itemRowFor(fresh, me, kQingduId);
        ASSERT_GE(row, 0) << "清毒散该出现在这一场的物品列表里（背包里有，战斗里也用得上）";
        std::vector<std::string> ids;
        const std::vector<fanren::ui::ListItem> rows = BattleScene::buildItemItems(
            app_.data(), state(), fresh.battle(), me, ids);
        const fanren::ui::ListItem& entry = rows[static_cast<std::size_t>(row)];
        EXPECT_FALSE(entry.enabled) << "没中毒时清毒散不该是可点的";
        EXPECT_NE(entry.disabledReason.find("没有中毒"), std::string::npos)
            << "置灰必须写明原因，只变灰不写原因玩家一律当成 bug，实得："
            << entry.disabledReason;

        // 真正的症结在背包上：点下去不该少一件。这一条走的是玩家那条路
        // （issuePlayerAction），不是规则层——扣背包本来就发生在 game 层。
        const int before = state().itemCount(kQingduId);
        ASSERT_GT(before, 0) << "先验：背包里真有这东西，否则「没少」是句废话";
        Action waste;
        waste.kind = ActionKind::Item;
        waste.actorIndex = me;
        waste.targetIndex = me;     // 用在自己身上
        waste.magicId = kQingduId;
        const auto refused = fresh.issuePlayerAction(app_, waste);
        EXPECT_FALSE(refused.ok) << "没中毒还能把清毒散灌下去";
        EXPECT_EQ(state().itemCount(kQingduId), before)
            << "点了一下清毒散，背包里少了一包，屏幕上只有一句「并无变化」";
        EXPECT_FALSE(fresh.battle().units()[static_cast<std::size_t>(me)].acted)
            << "白丢一个回合与白丢一件东西一样不许发生";
    }
    // 先验：「谁毒发了」这条判据咬得动。名字从场上那个单位身上取，不写字面量：
    // 主角的显示名现在是 data 给不出名字时的兜底，写死「韩立」的话它永远匹配不到，
    // 于是永远绿 —— 正是「找不到某个词就算过」那一类空转法。
    const std::string heroName = scene.battle().units()[0].name;
    ASSERT_FALSE(heroName.empty());
    EXPECT_TRUE(logHas(scene.battle(), beastUnit.name + " 毒发"))
        << "僵兽自己一次毒发也没有？那「玩家撒的那包毒」整件事就没发生";

    // ---- 「砍不动就下毒」的结账 ----
    //
    // ===== 这一段为什么整个重写（护体罡气有了失效时机之后）=====
    // 原来的写法把这一课押在一个**引擎缺陷**上：僵兽身法 3、追不上人，decideAi
    // 的兜底是运功，而罡气从不清零，于是它一路攒到 190 点，玩家砍 36 刀真砍进去
    // 0 点。原断言写的正是这个——「罡气峰值必须厚过一刀」「它那三十点气血是毒压
    // 下去的，不是刀砍下去的」。罡气现在一轮一轮地散（core/battle/Battle.h），
    // 那两条的前提没有了，留着它们等于把缺陷钉成正确行为。
    //（八方旅人化改造之后罡气整个换成了防御减半，下面的问法不变。）
    //
    // 于是这一课要重新问一遍，答案由下面两次**实跑**给，不由注释给：
    //   甲 · 只用刀：一刀砍进去几点、几刀放倒它、人撑不撑得住。
    //   乙 · 先下毒再补刀：毒扣掉多少、刀补几下、这一场收在哪个落点。
    // 两次都从头开一场，互不干扰。
    const fanren::core::BattleSetup* setup = app_.battleSetup(kBeastBattle);
    ASSERT_NE(setup, nullptr);
    EXPECT_FALSE(setup->defeatIsFatal) << "设计写明这一场可败，输了是爬出暗道，不是死";
    EXPECT_TRUE(setup->canEscape) << "也走得掉";

    // ---- 打法扫描：这一场到底还打不打得过 ----
    //
    // 「可败」写在 defeat_is_fatal=false 上（上面刚验过）；「不必败」只能实跑来答。
    // 用**一个**精心调过的打法去答是靠不住的：调到赢为止，答的就成了「我找得到
    // 一条路」而不是「这一场有路」。所以把三条开关的 8 种组合全跑一遍，
    // 判据是「**至少有一种打法赢得下来**」——这正是「不必败」的定义。
    //
    //   先清快的 —— 先砍两只笼中兽（防 3、两刀一只，每轮 18 点，真正的伤害来源），
    //                还是一头扎向砍不动的僵兽。
    //   会防御   —— 这一轮撑不住就先摆开守势。
    //   会解毒   —— 毒会要命时吃清毒散（不是一中毒就吃，那要白丢一个回合）。
    //
    // 甲（只用刀）单列，它是「砍不动」这条前提的对照组，不参与「打得过」的判定。
    const BeastRun knifeOnly = runBeastFight(BeastPlan{});

    struct Sweep {
        BeastPlan plan;
        BeastRun run;
    };
    std::vector<Sweep> sweep;
    for (int bits = 0; bits < 8; ++bits) {
        BeastPlan plan;
        plan.usePoison = true;
        plan.cullTheQuickOnesFirst = (bits & 1) != 0;
        plan.guardWhenLethal = (bits & 2) != 0;
        plan.cureWhenLethal = (bits & 4) != 0;
        sweep.push_back(Sweep{plan, runBeastFight(plan)});
    }
    // 乙（只会撒毒、别的都不会）就是 bits == 0 那一行，它是「砍不动就下毒」的
    // 直接对照；不另起变量，免得多一个没人读的名字。
    // 赢得最漂亮的那一趟（赢了的里面剩血最多的）。一趟也没赢时留在 end()。
    int bestIndex = -1;
    for (std::size_t i = 0; i < sweep.size(); ++i) {
        if (sweep[i].run.phase != BattlePhase::Won) continue;
        if (bestIndex < 0 || sweep[i].run.heroHp > sweep[static_cast<std::size_t>(bestIndex)].run.heroHp) {
            bestIndex = static_cast<int>(i);
        }
    }

    // 实测数据先打出来，再走断言：断言一旦红了，报告里要的正是这几个数，
    // 而 ASSERT_* 会让后面的 std::cout 一个字也印不出来。
    std::cout << "\n[ch03 教学二实测] 一刀砍僵兽 " << swingAtBeast << " 点（砍笼中兽 "
              << swingAtCaged << " 点），一包蚀心散 " << poison->poison << " 回合 × "
              << poison->poisonPower << " 点 = " << poisonTotal << " 点"
              << "\n  甲·只用刀：砍 " << knifeOnly.swings << " 刀进去 "
              << knifeOnly.swingDamage << " 点（僵兽破势 " << knifeOnly.beastBreaks
              << " 次），僵兽 " << beastUnit.maxHp << " → " << knifeOnly.beastHp << "，韩立 "
              << knifeOnly.heroHp << "/" << knifeOnly.heroMaxHp << "，落点编号 "
              << static_cast<int>(knifeOnly.phase) << "，第 " << knifeOnly.round << " 回合止，放爪 "
              << knifeOnly.clawCasts << " 次";
    for (const Sweep& entry : sweep) {
        std::cout << "\n  毒+" << (entry.plan.cullTheQuickOnesFirst ? "清快" : "扎僵")
                  << (entry.plan.guardWhenLethal ? "+防御" : "     ")
                  << (entry.plan.cureWhenLethal ? "+解毒" : "     ")
                  << "：毒扣 " << entry.run.poisonDamage << " 刀补 " << entry.run.swings
                  << " 下共 " << entry.run.swingDamage << " 点，僵兽 → " << entry.run.beastHp
                  << (entry.run.beastFelled ? "（倒下）" : "（还站着）") << "，破势 "
                  << entry.run.beastBreaks << " 次，韩立 "
                  << entry.run.heroHp << "/" << entry.run.heroMaxHp << "，放爪 "
                  << entry.run.clawCasts << " 次，中毒 "
                  << (entry.run.heroPoisoned ? "是" : "否") << "，解毒 "
                  << entry.run.antidotesUsed << " 包，防御 " << entry.run.guards
                  << " 回合，落点编号 " << static_cast<int>(entry.run.phase) << "，第 "
                  << entry.run.round << " 回合止";
    }
    std::cout << std::endl;

    // ---- 一 · 刀砍得动，但砍得**慢** ----
    // 「砍不动」从字面意义（0 点）退成了程度问题，这是修掉缺陷 A 的直接代价，
    // 如实钉在这里。判据取「砍僵兽要的刀数是砍笼中兽的三倍以上」：同一把刀、
    // 同一个玩家，差别只来自那 9 点防 —— 这才是设计文档想让玩家自己看出来的事。
    const int swingsOnBeast = (beastUnit.maxHp + swingAtBeast - 1) / swingAtBeast;
    const int swingsOnCaged = (cagedUnit.maxHp + swingAtCaged - 1) / swingAtCaged;
    ASSERT_GT(swingsOnCaged, 0) << "先验：分母不能是 0，否则下面那条倍数判据恒真";
    EXPECT_GE(swingsOnBeast, swingsOnCaged * 3)
        << "砍僵兽 " << swingsOnBeast << " 刀、砍笼中兽 " << swingsOnCaged
        << " 刀 —— 差距不够大，玩家自己得不出「这只得换个办法」";

    // ---- 二 · 毒仍然是更划算的那一手，而且**是唯一能绕开那 9 点防的** ----
    // 一个回合的动作：撒一包毒换 18 点，砍一刀换 5 点。这条比值才是这一课的核心，
    // 它与罡气无关，所以修掉缺陷 A 一点也没动摇它。
    EXPECT_GT(poisonTotal, swingAtBeast * 3)
        << "撒一包毒 " << poisonTotal << " 点，同样三个回合砍三刀 " << swingAtBeast * 3
        << " 点 —— 毒不比刀强的话，这一课白教";

    // ---- 三 · 毒杀不死人，最后那一下必须是刀 ----
    // 这是修完之后**新成立**的一条，而且它让这一课比从前完整：从前僵兽罡气无限，
    // 玩家既砍不动它、毒又只能把它压到一口气，那一场根本没有「打赢」这个落点，
    // 只能逃。现在是「毒压下去 + 补一刀」，两样都得用上。
    EXPECT_GE(poisonTotal * 2, beastUnit.maxHp - fanren::core::battle::kPoisonFloorHp)
        << "两包蚀心散压不到只剩一口气，玩家会以为毒没用";
    EXPECT_LT(poisonTotal * 2, beastUnit.maxHp + poisonTotal)
        << "毒多到一包半就结束的话，那 9 点防写了等于没写";

    // ---- 四 · 实跑的结账 ----
    //
    // ===== 这一段为什么又动了（尸毒爪射程 1 → 2 之后）=====
    // 射程改了以后僵兽从第四回合起就能隔着两格放爪，「一头扎向僵兽」那种打法
    // **打输了**（乙那一行，实测数字见上面）。原来那条
    // `EXPECT_TRUE(withPoison.beastFelled)` 钉的正是「乙那一趟能赢」。
    // **所以搬家而不是放宽**——「这一场还打得过」这条要求原样保留，只是不再
    // 押在某一条打法上，改成「8 种打法里至少有一种赢得下来」。
    EXPECT_GT(knifeOnly.swings, 0) << "只用刀那一趟一刀也没砍过，「砍不动」无从谈起";
    EXPECT_FALSE(knifeOnly.beastFelled)
        << "只用刀居然把僵兽砍倒了（它停在 " << knifeOnly.beastHp
        << " 点）——那这一课的前提「砍不动」就不成立了";

    // ---- 五 · 这一场可败，但**不该必败**（设计第 2 节 + 主控裁决 C3-1）----
    ASSERT_GE(bestIndex, 0)
        << "8 种打法一种也没赢下来，这一场成了必败。"
           "横版之后笼中兽、僵兽与尸毒爪的数要重新量（交给平衡路）；各趟的实测数见上面那几行";
    const BeastRun& best = sweep[static_cast<std::size_t>(bestIndex)].run;
    EXPECT_TRUE(best.beastFelled)
        << "赢了却没把僵兽放倒？那这一场的落点判定有问题";
    EXPECT_GT(best.heroHp, 0) << "赢是赢了，人却是 0 血 —— 落点判定有问题";
    // 赢得不该太轻松：这一场是教学，玩家得真的被逼着用毒。
    EXPECT_LT(best.heroHp, best.heroMaxHp)
        << "一滴血没掉就打赢了，那三只东西摆在那儿等于没摆";
    EXPECT_GT(best.poisonDamage, best.swingDamage)
        << "赢下来的那一趟里刀砍进去 " << best.swingDamage << " 点、毒扣掉 "
        << best.poisonDamage << " 点 —— 毒不挑大梁的话，玩家没有理由改用毒";

    EXPECT_EQ(state().itemCount(kQiduId), 1)
        << "七毒水一滴也不该在这一场用掉：它是章末处决余子童要用的";
}

// ---------------------------------------------------------------------------
// 二之二 · 「给自己解毒」那一半（主控裁决 C3-1：尸毒爪射程 1 → 2）
// ---------------------------------------------------------------------------
// 设计第 2 节要的是**两个方向**：给别人下毒，和给自己解毒。上一条测的是前者。
// 后者从前根本不发生——尸毒爪射程 1 与近战同距，而 decideAi 的挑选顺序是
// 「近战 → 法术 → 移动」，于是僵兽永远先平砍，那条法术一次也放不出来；
// 玩家在这一场里中不了毒，节点 7 拿到的 3 包清毒散全章没有一个时刻用得上
// （独立校对 MEDIUM-1）。射程改成 2 之后两格外才放爪、贴身仍用近战。
// 横版没有射程之后，敌方 AI 法力够就先放伤人的法术，僵兽开场第一手就放爪。
//
// 判据刻意分成三段，缺一段这一条就会退化成别的意思：
//   一 · 它真的放得出来（日志里有尸毒爪）——只验「韩立中毒了」的话，
//        将来若有人给别处加一个上毒来源，这一条照样绿，而爪子仍然放不出来。
//   二 · 韩立真的中了毒，而且是在一个**正常打法**里中的，不是站位摆出来的。
//   三 * 清毒散真的解掉了它，而且背包真的少了对应的包数。
TEST_F(Ch03TutorialBattle, TheAntidoteLessonNowActuallyHappensInThisFight) {
    const Item* antidote = app_.data().findItem(kQingduId);
    ASSERT_NE(antidote, nullptr);
    // 先验：清毒散确实是一件**纯解毒药**。它若还回血，下面「毒解了」这件事
    // 就分不清是解毒起的作用还是回血起的作用。
    ASSERT_TRUE(antidote->curesPoison) << "清毒散不解毒，这一条整个没有意义";

    const fanren::core::Magic* spell = app_.data().findMagic("magic_shidu_zhua");
    ASSERT_NE(spell, nullptr) << "尸毒爪不在 data/magics 里，这一条测的是空气";
    // 从前这里先验「射程大于近战」（C3-1：射程 1 时 decideAi 先挑近战，它一次也放不出来）。
    // 横版没有射程，「放不放得出来」只由敌方 AI 的挑选顺序与法力决定：它只放伤人的法术。
    ASSERT_TRUE(fanren::core::offensiveMagic(*spell)) << "尸毒爪不算伤人的法术，敌方 AI 不会放它";
    ASSERT_GT(spell->poison, 0) << "尸毒爪不上毒的话，这一场教不成解毒";

    // 先验：**这一下整场只放得出一次**，而这件事由两个数的比值定：
    // 僵兽的 maxMp 与尸毒爪的 needMp。两个数分在两个文件里，谁也不知道对方，
    // 所以在这里钉住它们的关系——比值一变，这一场的平衡当场重定标（技术债 C3-1）。
    //   · 比值 < 1：一下也放不出来，玩家又中不了毒，回到 C3-1 的原样；
    //   · 比值 > 1：僵兽连放到法力见底（战棋时实测 8 种打法一种也赢不下来）。
    {
        const fanren::core::RoleTemplate* beastRole = app_.data().findRole(kBeastId);
        ASSERT_NE(beastRole, nullptr);
        ASSERT_GT(spell->needMp, 0) << "不耗法力的话「只放得出一下」无从谈起";
        EXPECT_EQ(beastRole->maxMp / spell->needMp, 1)
            << "僵兽法力 " << beastRole->maxMp << "、尸毒爪耗 " << spell->needMp
            << "，够放 " << beastRole->maxMp / spell->needMp
            << " 下。整场只该放得出一下：放不出来这一课就没了，"
               "放得出好几下这一场就成了必败（实测见 docs/tech-debt.md C3-1）";
    }

    BeastPlan plan;
    plan.usePoison = true;
    plan.cullTheQuickOnesFirst = true;
    plan.cureWhenLethal = true;
    const BeastRun lesson = runBeastFight(plan);

    // 实测数据先打出来，再走断言：断言一旦红了，报告里要的正是这几个数。
    std::cout << "\n[ch03 教学二·解毒实测] 尸毒爪 " << spell->poison << " 回合 × "
              << spell->poisonPower << " 点"
              << "\n  僵兽放爪 " << lesson.clawCasts << " 次；韩立"
              << (lesson.heroPoisoned ? "中过毒" : "一次也没中毒") << "，第 "
              << lesson.heroFirstPoisonRound << " 回合第一次中"
              << "\n  清毒散用掉 " << lesson.antidotesUsed << " 包，解掉了毒："
              << (lesson.heroCured ? "是" : "否") << "；防御 " << lesson.guards << " 回合"
              << "\n  韩立收场 " << lesson.heroHp << "/" << lesson.heroMaxHp << "，落点编号 "
              << static_cast<int>(lesson.phase) << "，第 " << lesson.round << " 回合止"
              << std::endl;

    EXPECT_GT(lesson.clawCasts, 0)
        << "整场打下来僵兽一次尸毒爪也没放出来——敌方 AI 的挑选顺序"
           "又把它挡回去了，这正是 C3-1 的原症状";
    EXPECT_TRUE(lesson.heroPoisoned)
        << "韩立一次也没中毒：那 3 包清毒散在这一章仍旧是永远用不上的东西";
    EXPECT_NE(lesson.phase, BattlePhase::Ongoing)
        << "打了 160 个我方回合还没收场——这一场变成耗不完的拉锯了";
}

// ---------------------------------------------------------------------------
// 二之三 · 清毒散真的解得掉尸毒爪的毒（规则层，不靠打得好不好）
// ---------------------------------------------------------------------------
// 上一条问的是「这一场里会不会发生」，这一条问的是「发生了解不解得掉」。
// 分成两条是因为两件事会因为完全不同的原因坏掉：前者坏在 AI 的挑选口径上，
// 后者坏在物品判定上。合成一条的话，僵兽哪一天不放爪了，连「解得掉」也一并
// 测不到了，而那正是本项目那张空转法表里「先验没了，断言跟着失效」的长相。
TEST_F(Ch03TutorialBattle, TheAntidoteClearsExactlyWhatTheClawPutOn) {
    state().addItem(kQingduId, 3);
    const fanren::core::Magic* spell = app_.data().findMagic("magic_shidu_zhua");
    ASSERT_NE(spell, nullptr);

    BattleScene scene(kBeastBattle);
    scene.onEnter(app_);

    // 站着不动（防御），等它放爪。**不喂状态**：韩立身上这一口毒
    // 必须真的是僵兽放上来的，否则这一条测的就是「清毒散解得掉我自己塞的毒」。
    int me = -1;
    for (int turn = 0; turn < 40; ++turn) {
        me = scene.runToAllyTurn();
        if (me < 0) break;
        if (scene.battle().units()[static_cast<std::size_t>(me)].poison > 0) break;
        if (!playerDefends(scene)) break;
    }
    ASSERT_GE(me, 0) << "仗都打完了韩立还没中过毒";
    const auto meIdx = static_cast<std::size_t>(me);
    const Unit& hero = scene.battle().units()[meIdx];
    ASSERT_GT(hero.poison, 0)
        << "四十个回合过去僵兽一次也没把毒挂上来——C3-1 的原症状回来了";
    // 这一口毒确实是尸毒爪那一口：每轮掉的点数与 data 里那条法术一字不差。
    // 少了这一句，将来若别处多出一个上毒来源，这一条会在测另一件事而不自知。
    EXPECT_EQ(hero.poisonPower, spell->poisonPower)
        << "韩立身上这口毒每轮掉 " << hero.poisonPower << " 点，而尸毒爪是 "
        << spell->poisonPower << " 点——它不是爪子挂上来的";
    EXPECT_LE(hero.poison, spell->poison) << "剩余回合数比这条法术给的还多";

    const int before = state().itemCount(kQingduId);
    ASSERT_GT(before, 0);
    ASSERT_TRUE(playerThrows(scene, me, kQingduId, me))
        << "中着毒却点不动清毒散：" << scene.feedback();
    EXPECT_EQ(scene.battle().units()[meIdx].poison, 0) << "清毒散没把毒解掉";
    EXPECT_EQ(scene.battle().units()[meIdx].poisonPower, 0) << "毒解了，每轮掉血却还挂着";
    EXPECT_EQ(state().itemCount(kQingduId), before - 1) << "药吃了，背包里却没少";
}

// ===========================================================================
// 三 · 识海：第一团吞掉，第二团必然逃脱
// ===========================================================================
TEST_F(Ch03TutorialBattle, TheFirstBallIsSwallowedAndTheSecondAlwaysGetsAway) {
    // 玩家身上带着毒与药，但这一场一件也用不上 —— 那是明写的例外。
    state().addItem(kShixinId, 2);
    state().addItem(kQingduId, 3);

    BattleScene scene(kMindBattle);
    scene.onEnter(app_);

    const int soul = firstUnitWithId(scene.battle(), kSoulId);
    const int yu = firstUnitWithId(scene.battle(), kYuId);
    ASSERT_GE(soul, 0) << "第一团：墨居仁元神";
    ASSERT_GE(yu, 0) << "第二团：余子童元神";

    // ---- 先验：原著那三条体积关系必须先成立 ----
    const int heroVolumeAtStart = scene.battle().units()[0].maxHp;
    const int soulVolume = scene.battle().units()[static_cast<std::size_t>(soul)].maxHp;
    const int yuVolume = scene.battle().units()[static_cast<std::size_t>(yu)].maxHp;
    ASSERT_LT(soulVolume, heroVolumeAtStart)
        << "第一团该比韩立小好几倍（拇指大），才谈得上「靠体积轻易吞掉」";
    ASSERT_GT(yuVolume, heroVolumeAtStart)
        << "第二团该比韩立大一圈有余，才谈得上「每被咬住就脱开那块继续跑」";
    // 这一场没有法术、没有物品、不能逃。前两样另有专测，这里只取「不能逃」
    // 那一条当先验：它一旦松了，「第二场打不死」就会被玩家用逃跑绕过去。
    const int firstActor = scene.runToAllyTurn();
    ASSERT_GE(firstActor, 0);
    const std::vector<fanren::ui::ListItem> root =
        BattleScene::buildRootItems(app_.data(), state(), scene.battle(), firstActor);
    ASSERT_GT(root.size(), static_cast<std::size_t>(fanren::game::kBattleMenuEscape));
    EXPECT_FALSE(root[static_cast<std::size_t>(fanren::game::kBattleMenuEscape)].enabled)
        << "识海这一场逃不得（硬约束：败即结束）";

    // ---- 第一场：把那一团吞掉 ----
    // 原著里它一照面就咬上来，所以这一段是「站着对咬」，玩家只管往它身上砍。
    for (int turn = 0; turn < 60; ++turn) {
        if (!scene.battle().units()[static_cast<std::size_t>(soul)].alive()) break;
        const int actor = scene.runToAllyTurn();
        if (actor < 0) break;
        if (!playerAttacks(scene, actor, soul)) {
            if (!playerDefends(scene)) break;
        }
    }
    const Unit& firstBall = scene.battle().units()[static_cast<std::size_t>(soul)];
    EXPECT_FALSE(firstBall.alive()) << "第一团该被吞掉，原著写的是「很快结束」";
    EXPECT_FALSE(firstBall.fled) << "第一团是被吞掉的，不是跑掉的 —— 跑的是第二团";
    const int volumeAfterFirst = scene.battle().units()[0].maxHp;
    EXPECT_GT(volumeAfterFirst, heroVolumeAtStart)
        << "吞了却没长：体积即实力，这一条是这一场全部规则的根";
    EXPECT_EQ(scene.battle().phase(), BattlePhase::Ongoing)
        << "第一团倒下不该收场：第二团是编成里的第二波（两场连着的），这时该杀到了";
    EXPECT_TRUE(scene.battle().units()[static_cast<std::size_t>(yu)].onField)
        << "第一团一倒，第二团就该上场——横版里它不再站在对角上等韩立追过去";

    // ---- 第二场：咬到三分之一它就脱开逃走 ----
    for (int turn = 0; turn < 120; ++turn) {
        if (scene.battle().phase() != BattlePhase::Ongoing) break;
        const int actor = scene.runToAllyTurn();
        if (actor < 0) break;
        if (!playerAttacks(scene, actor, yu)) {
            if (!playerDefends(scene)) break;
        }
    }

    MindOutcome outcome;
    outcome.phase = scene.battle().phase();
    outcome.firstBallDevoured = !firstBall.alive() && !firstBall.fled;
    const Unit& secondBall = scene.battle().units()[static_cast<std::size_t>(yu)];
    outcome.secondBallFled = secondBall.fled;
    outcome.secondBallKilled = !secondBall.fled && secondBall.hp <= 0;
    outcome.spoilsPercent = scene.battle().devourSpoilsPercent();
    outcome.heroVolumeGrowth = volumeAfterFirst - heroVolumeAtStart;

    const MindVerdict verdict = mindBattleVerdict(outcome);
    EXPECT_TRUE(verdict.ok) << verdict.why;

    // 咬下的那一块与它剩下的体积要对得上：战果不是另算的一个数。
    EXPECT_LE(secondBall.maxHp * 3, yuVolume * 2)
        << "它逃走时剩下的体积还在三分之二以上，说明逃遁阈值与战果各算各的";

    std::cout << "\n[ch03 识海实测] 韩立入梦体积 " << heroVolumeAtStart << " → 吞掉第一团后 "
              << volumeAfterFirst << "；第二团 " << yuVolume << " → " << secondBall.maxHp
              << "，咬下 " << outcome.spoilsPercent << "%" << std::endl;
}

// ===========================================================================
// 三之二 · 识海战果的两条分支都真的走得到（主控裁决 C3-2 的前提）
// ===========================================================================
// scripts/ch03/duoshe.lua 现在按 flag.get("ch03.shihai_yaoxia") >= 40 分两句话。
// 本项目的硬标准是：**分支接上之前，先穷举证明两条都可达**——第 1 章出过某结局
// 81 条路径 0 次可达的事故，而「两条分支说同一句话」的假分叉比不接更坏。
//
// ---------------------------------------------------------------------------
// 穷举的是什么，以及为什么这个空间可信
// ---------------------------------------------------------------------------
// 识海那一场玩家手上只剩两样东西：咬哪一团、不咬。没有法术、没有物品、不能逃
//（契约 3.5）；横版之后也没有走位（战棋那几年走位只通过「够不够得着」起作用）。
// 所以把一局的打法写成一个**字母序列**，一个我方回合一个字母：
//
//   黄 —— 这一手咬黄光球（墨居仁元神）。
//   绿 —— 这一手咬绿光球（余子童元神）。
//   等 —— 这一手防御，不咬。
//
// 被点名的那一团若不在场上（吞掉了、逃走了，或者是还没杀到的第二波），这个字母
// 退化成「咬另一团」，都不在就防御——这样任何一条字母序列都是一局完整的打法，
// 不存在「无效计划」。
//
// 枚举**前 kMindPlanDepth 个我方回合的全部 3^n 种字母组合**，第 n 个之后照最后
// 一个字母续到收场。为什么这个截断说得过去：绿光球体积单调下降（吞噬只有我方
// 吃得下去，见 BattleAction.cpp 的注释），所以「咬下了多少」这个数只由**咬的
// 顺序与次数**决定，而它在头几个回合就定型了——第一团只挨得住四口，
// 逃遁阈值又是固定的三分之二。后半程再怎么变，只会改最后那一口落在哪个边界上，
// 而那正是前几个字母已经决定的事。
//
// 判据本身（阈值 40）写死在这里，不从 duoshe.lua 里读：从被测物推导出来的判据
// 发现不了被测物与规格不一致——那是第 3 章那条最要害的缺陷的教训。
// 脚本与这个数对不对得上，由下面另一条用例单独问。

// 深度 11 是**跑得起**与**够深**之间的取舍，两头都有实测：
//   · 够深 —— 同一套穷举在深度 9（19,683 条）与深度 14（4,728,837 条，68 秒）
//     下跑过，战果区间都是 33%-38%，一分不差。深度从 9 加到 14 只把条数放大了
//     240 倍，没有推开任何一头，所以 11 这一档报的区间是可信的。
//   · 跑得起 —— 深度每加一级条数约 ×2.9（剪枝之后），14 要 68 秒，
//     那是一条没人愿意在本地重跑的用例，而不重跑的用例迟早会被跳过。
// 换深度之前请把上面这两句实测一起换掉，不要只改数字。
//（这两句是战棋时代两团同场时的实测。八方旅人化改造把第二团改成第二波之后没有重测，
// 区间以本条每次打印的那一行为准；平衡路重测之后把这两句一起换掉。）
constexpr int kMindPlanDepth = 11;
constexpr char kMindLetters[] = {'Y', 'G', 'W'};

struct MindSurvey {
    // 阈值从 duoshe.lua 里读；脚本不分档时是 -1，此时 deep/shallow 一律不计。
    int threshold = -1;
    long long plays = 0;        // 走到分叉的打法条数（提前收场的不再往下分叉）
    long long fled = 0;         // 绿光球脱身，战果就此定死
    long long deep = 0;         // 落在 >= 阈值 那一侧
    long long shallow = 0;      // 落在 < 阈值 那一侧
    long long lost = 0;         // 韩立输了，脚本走 game_over，不到分支那一句
    long long noFlee = 0;       // 赢了或逃了：契约说这一场不该有这两个落点
    long long unfinished = 0;   // 一直防御耗到回合上限
    int minSpoils = 1000;
    int maxSpoils = -1;
    std::map<int, long long> histogram;
};

// duoshe.lua 的原文，供两条用例共用。
std::string duosheSource() {
    std::ifstream input(fs::path(assetRoot()) / "scripts" / "ch03" / "duoshe.lua",
                        std::ios::binary);
    if (!input.good()) return {};
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

// 这个位置是不是在注释里：从行首到它之间出现过 `--` 就是。
//
// **两条用例都必须只认正文。** duoshe.lua 的首部写着「将来接上时照这个样子写」的
// 示例，里头有一整段 `flag.get("ch03.shihai_yaoxia")` 与 `if bitten >= <阈值>`；
// 把注释也算进来的话，这两条会以为脚本已经接上了，而它一个字也没接。
// 第一版正是这么写的，当场红了一条。
bool inLuaComment(const std::string& source, std::string::size_type at) {
    const std::string::size_type lineStart = source.rfind('\n', at);
    const std::string::size_type from = (lineStart == std::string::npos) ? 0 : lineStart + 1;
    const std::string::size_type marker = source.find("--", from);
    return marker != std::string::npos && marker < at;
}

// 正文里第一次真的读那条旗标的位置；只出现在注释里就当没读。
std::string::size_type liveFlagRead(const std::string& source) {
    static const std::string needle = "flag.get(\"ch03.shihai_yaoxia\")";
    std::string::size_type at = 0;
    while ((at = source.find(needle, at)) != std::string::npos) {
        if (!inLuaComment(source, at)) return at;
        at += needle.size();
    }
    return std::string::npos;
}

// 正文里第一次真的引某个文案 key 的位置；只出现在注释里就当没引。
std::string::size_type liveTextRef(const std::string& source, const std::string& key) {
    std::string::size_type at = 0;
    while ((at = source.find(key, at)) != std::string::npos) {
        if (!inLuaComment(source, at)) return at;
        at += key.size();
    }
    return std::string::npos;
}

// 脚本此刻按哪个数分档；没接（或读了却没分档）就返回 -1。
int scriptBranchThreshold() {
    const std::string source = duosheSource();
    const std::string::size_type read = liveFlagRead(source);
    if (read == std::string::npos) return -1;
    std::string::size_type mark = read;
    while ((mark = source.find(">=", mark + 1)) != std::string::npos) {
        if (inLuaComment(source, mark)) continue;
        std::string::size_type digits = mark + 2;
        while (digits < source.size() && source[digits] == ' ') ++digits;
        int value = -1;
        while (digits < source.size() && source[digits] >= '0' && source[digits] <= '9') {
            value = (value < 0 ? 0 : value) * 10 + (source[digits] - '0');
            ++digits;
        }
        return value;
    }
    return -1;
}

bool mindAttack(BattleState& battle, int me, int target) {
    Action a;
    a.kind = ActionKind::Attack;
    a.actorIndex = me;
    a.targetIndex = target;
    if (!battle.isLegal(a)) return false;
    return battle.apply(a).ok;
}

// 推进到下一个我方回合；没有了（分出胜负或步数耗尽）返回 -1。
// 敌方由引擎自己那一套 decideAi 走，与 BattleScene::stepAi 一模一样：
// 一个动作，然后交回合。
int mindAdvanceToAlly(BattleState& battle) {
    for (int guard = 0; guard < 512; ++guard) {
        if (battle.phase() != BattlePhase::Ongoing) return -1;
        const int actor = battle.currentActor();
        if (actor < 0) {
            battle.endTurn();
            continue;
        }
        if (battle.units()[static_cast<std::size_t>(actor)].ally) return actor;
        const Action action = battle.decideAi(actor);
        if (action.actorIndex < 0) {
            battle.endTurn();
            continue;
        }
        static_cast<void>(battle.apply(action));
        battle.endTurn();
    }
    return -1;
}

void mindPlayLetter(BattleState& battle, int me, char letter, int yellow, int green) {
    if (letter == 'W') {
        Action guard;
        guard.kind = ActionKind::Defend;
        guard.actorIndex = me;
        if (battle.isLegal(guard)) static_cast<void>(battle.apply(guard));
        battle.endTurn();
        return;
    }
    int want = (letter == 'Y') ? yellow : green;
    const int other = (letter == 'Y') ? green : yellow;
    if (want < 0 || !battle.units()[static_cast<std::size_t>(want)].alive()) want = other;
    if (want >= 0 && battle.units()[static_cast<std::size_t>(want)].alive()) {
        mindAttack(battle, me, want);
    }
    battle.endTurn();
}

void mindRecord(const BattleState& battle, int green, MindSurvey& survey) {
    ++survey.plays;
    // **先看绿光球跑没跑，再看落点。** 剪枝在它脱身的那一刻就收，此时 phase 多半
    // 还是 Ongoing（黄光球还站着），照 phase 分类会把它记成「耗到上限」。
    // 战果那一刻已经定死，剧情那边读的也正是这个数。
    if (battle.units()[static_cast<std::size_t>(green)].fled) {
        const int spoils = battle.devourSpoilsPercent();
        ++survey.fled;
        survey.minSpoils = std::min(survey.minSpoils, spoils);
        survey.maxSpoils = std::max(survey.maxSpoils, spoils);
        ++survey.histogram[spoils];
        if (survey.threshold >= 0) {
            if (spoils >= survey.threshold) {
                ++survey.deep;
            } else {
                ++survey.shallow;
            }
        }
        return;
    }
    switch (battle.phase()) {
        case BattlePhase::EnemyFled:
            // 绿的没跑却报 enemy_fled，只可能是别处出了事，记成「不该出现的落点」。
            ++survey.noFlee;
            break;
        case BattlePhase::Lost:
            ++survey.lost;
            break;
        case BattlePhase::Won:
        case BattlePhase::Escaped:
            ++survey.noFlee;
            break;
        case BattlePhase::Ongoing:
            ++survey.unfinished;
            break;
    }
}

// 计划用完之后照最后一个字母续到收场。
//
// 续多少手有上限，上限要**够得着最长的那一种收场**，否则「耗到上限」报的是上限太短，
// 不是真的收不了场。战棋那几年两团同场、绿光球一口 41，200 手绰绰有余。第二团改成
// 第二波之后，最长的一种是：把黄光球咬到只剩一丁点、然后一直防御——它一口只咬得动
// 1 分（devourBite 下限 1），而韩立此刻的体积不超过 480 ＋ 120 = 600，要六百来手才输。
// 所以上限取 700：每一手韩立至少掉 1 分、体积有上界，打法再磨也必然在这之前收场。
constexpr int kMindTailTurns = 700;

void mindRunTail(BattleState battle, char letter, int yellow, int green, MindSurvey& survey) {
    for (int turn = 0; turn < kMindTailTurns; ++turn) {
        if (battle.phase() != BattlePhase::Ongoing) break;
        if (battle.units()[static_cast<std::size_t>(green)].fled) break;
        const int me = mindAdvanceToAlly(battle);
        if (me < 0) break;
        mindPlayLetter(battle, me, letter, yellow, green);
    }
    mindRecord(battle, green, survey);
}

// 绿光球一脱身，战果就定死了，再往下分叉一个字也改不了它。
//
// 为什么这条剪枝是**对的**而不是省事：spoilsPercent_ 只在 devourTransfer 里
// 那一处写，条件是「这一团进来时比吞噬者大 + 已经被咬掉三分之一」。黄光球
// 120 比韩立 480 小，devourCanBreakAway 对它恒假，所以全场只有绿光球跑得掉，
// 那个 std::max 至多只被写一次。绿的跑了之后剩下的事（把黄的吃掉）不碰这个数。
//
// 剪掉它之后树的深度由「绿光球什么时候脱身」封顶，而不是由 kMindPlanDepth 封顶
// ——不剪的话每条路都要多跑十几个回合，而那十几个回合的分叉全是同一个战果。
bool mindOutcomeSettled(const BattleState& battle, int green) {
    return battle.units()[static_cast<std::size_t>(green)].fled;
}

void mindExplore(BattleState battle, int depth, char lastLetter, int yellow, int green,
                 MindSurvey& survey) {
    if (battle.phase() != BattlePhase::Ongoing || mindOutcomeSettled(battle, green)) {
        mindRecord(battle, green, survey);
        return;
    }
    if (depth >= kMindPlanDepth) {
        mindRunTail(std::move(battle), lastLetter, yellow, green, survey);
        return;
    }
    const int me = mindAdvanceToAlly(battle);
    if (me < 0) {
        mindRecord(battle, green, survey);
        return;
    }
    for (const char letter : kMindLetters) {
        BattleState child = battle;
        mindPlayLetter(child, me, letter, yellow, green);
        mindExplore(std::move(child), depth + 1, letter, yellow, green, survey);
    }
}

// duoshe.lua 若按阈值分档，这个阈值必须两侧都真的走得到。
//
// **判据的形状**：本条不规定阈值该取多少——那是主控的事。它只问一件事：
// 脚本此刻写着的那个数，把玩家的打法真的分成了非空的两堆吗。于是三种状态
// 各有各的下场：
//
//   · 脚本不分档（现状）      —— 如实报出战果分布，不假装接上了；
//   · 脚本按一个可达的数分档  —— 绿；
//   · 脚本按一个不可达的数分档 —— **红**，而且报得出「哪一侧是 0」。
//
// 第三种正是 2026-09-21 实际发生的事：主控裁决阈值取 40，依据是「实测落在
// 33-45%」，而穷举实测是 33%-38%——40 一条也到不了。按裁决自己那一句
// 「若枚举证明某一条 0 次可达，就不要接」，脚本保持不分档，等新的数。
//
// 阈值从脚本里读，是有意的：这一条要回答的命题就是「**脚本现在这个数**是不是
// 假分叉」。而「这个数该是多少」由设计裁决，不在这里判——两件事分开，
// 才不会变成拿被测物给自己打分。
TEST_F(Ch03TutorialBattle, EveryPlanLandsOnOneOfTheTwoBranchesAndBothAreReached) {
    BattleScene scene(kMindBattle);
    scene.onEnter(app_);
    const int yellow = firstUnitWithId(scene.battle(), kSoulId);
    const int green = firstUnitWithId(scene.battle(), kYuId);
    ASSERT_GE(yellow, 0) << "识海那一场没有黄光球，下面穷举的是别的东西";
    ASSERT_GE(green, 0) << "识海那一场没有绿光球";
    ASSERT_TRUE(scene.battle().devourMode()) << "不是吞噬模式，战果恒为 0，穷举没有意义";

    const int threshold = scriptBranchThreshold();
    MindSurvey survey;
    survey.threshold = threshold;
    mindExplore(scene.battle(), 0, 'G', yellow, green, survey);

    std::string spread;
    for (const auto& [spoils, count] : survey.histogram) {
        spread += " " + std::to_string(spoils) + "%×" + std::to_string(count);
    }
    std::cout << "\n[ch03 识海分支穷举] 字母表 {黄,绿,等}，前 " << kMindPlanDepth
              << " 个我方回合全组合（上界 3^" << kMindPlanDepth
              << "），绿光球脱身即剪枝，其后照最后一手续到收场"
              << "\n  走到分叉的打法 " << survey.plays << " 条：敌人逃走 "
              << survey.fled << "、韩立输 " << survey.lost << "、没人逃走 "
              << survey.noFlee << "、耗到上限 " << survey.unfinished
              << "\n  战果区间 " << survey.minSpoils << "%-" << survey.maxSpoils
              << "%，分布：" << spread
              << "\n  duoshe.lua 此刻的阈值：" << (threshold < 0 ? std::string("不分档")
                                                             : std::to_string(threshold))
              << std::endl;

    // 先验分母：真的跑了很多条，而且真的有人逃走过。塌成 0 的话，
    // 下面每一条都会变成在比较两个 0。
    ASSERT_GT(survey.plays, 1000) << "只走了 " << survey.plays << " 条，穷举没跑起来";
    ASSERT_GT(survey.fled, 0) << "一条 enemy_fled 也没走到";
    ASSERT_GE(survey.minSpoils, 0);
    ASSERT_LE(survey.maxSpoils, 100);

    // 这条旗标值得被读，前提是它真的会变。恒为一个数的话，任何阈值都是假分叉，
    // 而那件事在这里就该发现，不必等到有人写了 if 才发现。
    EXPECT_LT(survey.minSpoils, survey.maxSpoils)
        << "所有打法的战果都是同一个数（" << survey.minSpoils
        << "%），那 ch03.shihai_yaoxia 分不出任何档";

    // 契约 3.4 / 5.3：这一场必然 enemy_fled 或者韩立输。绿光球打不死、逃不掉、
    // 也赢不了；玩家这一侧连逃跑动作都不合法（can_escape=false）。
    EXPECT_EQ(survey.noFlee, 0)
        << "有 " << survey.noFlee << " 条打法走到了「赢」或「逃」，"
           "而契约钉死了这一场只有「敌人带伤逃走」与「韩立输」两个落点";
    EXPECT_EQ(survey.unfinished, 0)
        << "有 " << survey.unfinished << " 条打法耗到了回合上限，收不了场";

    // 正题：脚本此刻那个阈值不许是假分叉。
    if (threshold >= 0) {
        EXPECT_GT(survey.deep, 0)
            << "duoshe.lua 按 >= " << threshold << " 分档，可是没有任何一种打法到得了（最高 "
            << survey.maxSpoils << "%）——ch03.duoshe.flee_deep 那一句玩家永远看不见";
        EXPECT_GT(survey.shallow, 0)
            << "duoshe.lua 按 >= " << threshold << " 分档，可是没有任何一种打法落在它以下（最低 "
            << survey.minSpoils << "%）——ch03.duoshe.flee_shallow 那一句玩家永远看不见";
    }
}

// ---------------------------------------------------------------------------
// 脚本那一侧：要么老老实实不分档，要么分得干净
// ---------------------------------------------------------------------------
// 上一条问的是「阈值可不可达」，这一条问的是「分支本身是不是真的」。
// 两条分开，是因为假分叉有两种长相：一种是阈值到不了（上一条抓），
// 另一种是两条分支念同一句话（这一条抓）。后者 grep 看得见 flag.get、
// 像是接上了，而没有任何测试分得出两条——正是裁决里明令禁止的那一种。
TEST_F(Ch03TutorialBattle, TheScriptEitherDoesNotBranchOrBranchesForReal) {
    const std::string source = duosheSource();
    ASSERT_FALSE(source.empty()) << "先验：脚本不是空的，否则下面每一条 find 都恒假";
    // 先验：这份源码真的是那一个脚本（不是读到了别的文件），否则下面「没分档」
    // 这个结论只是因为什么都没读到。
    ASSERT_NE(source.find("b03_shihai_duoshe"), std::string::npos)
        << "读到的不是识海那一段脚本";

    // 先验：注释里那段示例确实在，否则「只认正文」这件事就没被验到——
    // 示例哪天被删了，下面那句 reads 恒假，这条用例会安静地变成一句废话。
    ASSERT_NE(source.find("flag.get(\"ch03.shihai_yaoxia\")"), std::string::npos)
        << "duoshe.lua 里连注释里的示例都没有了，这一条的「只认正文」验不到";

    const bool reads = liveFlagRead(source) != std::string::npos;
    const int threshold = scriptBranchThreshold();
    if (!reads) {
        // 现状（2026-09-21）：没接。那就**不许**有半截接线留在那儿。
        EXPECT_LT(threshold, 0) << "没有 flag.get 却出现了分档阈值，接线接了一半";
        EXPECT_EQ(liveTextRef(source, "ch03.duoshe.flee_deep"), std::string::npos)
            << "脚本没读那条旗标，却在正文里引了分档才用得上的文案";
        EXPECT_EQ(liveTextRef(source, "ch03.duoshe.flee_shallow"), std::string::npos);
        return;
    }

    ASSERT_GE(threshold, 0) << "读了旗标却没有 `>= <数>` 的分档，这条读是空转的";
    ASSERT_NE(liveTextRef(source, "ch03.duoshe.flee_deep"), std::string::npos);
    ASSERT_NE(liveTextRef(source, "ch03.duoshe.flee_shallow"), std::string::npos);
    const std::string deep = app_.data().lookupText("ch03.duoshe.flee_deep");
    const std::string shallow = app_.data().lookupText("ch03.duoshe.flee_shallow");
    EXPECT_FALSE(deep.empty()) << "分档用的文案 key 在 data/text 里查不到";
    EXPECT_FALSE(shallow.empty());
    EXPECT_NE(deep, shallow) << "两条分支念的是同一句话，那就是个假分叉";
}

// ---------------------------------------------------------------------------
// 判据自检：证明识海那条判据分得开「敌人跑了」与「韩立输了」
// ---------------------------------------------------------------------------
// 这一场最容易写错的断言是 EXPECT_FALSE(won)：它在两种局面下都成立，
// 而那两件事一件是章末的转折、一件是 game_over。判据必须咬得动这个分别。
MindOutcome faithfulMindOutcome() {
    MindOutcome o;
    o.phase = BattlePhase::EnemyFled;
    o.firstBallDevoured = true;
    o.secondBallFled = true;
    o.secondBallKilled = false;
    o.spoilsPercent = 36;
    o.heroVolumeGrowth = 120;
    return o;
}

TEST(Ch03MindOutcome, TheVerdictAcceptsTheFaithfulEnding) {
    const MindVerdict verdict = mindBattleVerdict(faithfulMindOutcome());
    EXPECT_TRUE(verdict.ok) << verdict.why;
}

TEST(Ch03MindOutcome, TheVerdictTellsALostFightApartFromAFledEnemy) {
    // 两者的 won 都是 false。只看 won 的判据在这里会一路绿灯。
    MindOutcome lost = faithfulMindOutcome();
    lost.phase = BattlePhase::Lost;
    lost.secondBallFled = false;
    const MindVerdict caught = mindBattleVerdict(lost);
    EXPECT_FALSE(caught.ok) << "韩立被打败也算过关，那这一章的高潮就没了";
    EXPECT_NE(caught.why.find("输"), std::string::npos) << caught.why;
}

TEST(Ch03MindOutcome, TheVerdictCatchesASecondBallThatWasKilledInstead) {
    // 第二团是设计上打不死的（技术债 G-2 那个落点）。把它写成能打死，
    // 原著「体积少了三分之一」那一笔就没了着落。
    MindOutcome killed = faithfulMindOutcome();
    killed.phase = BattlePhase::Won;
    killed.secondBallFled = false;
    killed.secondBallKilled = true;
    EXPECT_FALSE(mindBattleVerdict(killed).ok) << "第二团被打死了也算过关？";
}

TEST(Ch03MindOutcome, TheVerdictCatchesADevourThatNeverFed) {
    // 「吞掉」与「打死」的分别：吞了要长。不长的话，第一场就只是一次普通击杀，
    // 而体积即实力这条规则也就白写了。
    MindOutcome hollow = faithfulMindOutcome();
    hollow.heroVolumeGrowth = 0;
    const MindVerdict caught = mindBattleVerdict(hollow);
    EXPECT_FALSE(caught.ok) << "吞了没长也算吞掉？";
    EXPECT_NE(caught.why.find("长"), std::string::npos) << caught.why;
}

TEST(Ch03MindOutcome, TheVerdictCatchesSpoilsThatDriftedOffTheOriginal) {
    // 战果按咬下的比例计。两头都要咬住：太少说明逃得太早，太多说明它该跑不跑。
    MindOutcome thin = faithfulMindOutcome();
    thin.spoilsPercent = 0;
    EXPECT_FALSE(mindBattleVerdict(thin).ok) << "一口没咬到也算「咬下三分之一」？";

    MindOutcome fat = faithfulMindOutcome();
    fat.spoilsPercent = 80;
    EXPECT_FALSE(mindBattleVerdict(fat).ok) << "咬掉八成还不跑，那它就不是「逃脱」了";
}

TEST(Ch03MindOutcome, TheVerdictCatchesAFirstBallThatRanInsteadOfBeingEaten) {
    // 两团的下场不许对调：小的那团是被吞掉的，大的那团才是跑掉的。
    MindOutcome swapped = faithfulMindOutcome();
    swapped.firstBallDevoured = false;
    EXPECT_FALSE(mindBattleVerdict(swapped).ok) << "第一团没被吞掉也算过关？";
}

}  // namespace
