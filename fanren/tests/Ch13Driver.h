#pragma once

// 第 13 章通关驱动（测试路 B）：从真实的第 12 章终档（T1 重生成版：lingquan.lua 末行 teleport 之后落在
// ch13_nanming 的 haishang (6,29)）起步，走真实入口——WorldScene 走图与 interact、脚本 teleport 串起的四张新图、
// 脚本 shop() 开的 ShopScene（意图玩家不买不卖，真按「离开」：ShopScene::leave）、openFacility 开的
// CultivationScene（雾中岛静室真按「闭关一年」与冲关）、BattleScene 真打（tests/BattleHand.h 那只手）。
// 不改 state 注入结果；切片里的扰动只在用例里写明。判据一律抄 docs/ch13-design.md：3.2 动作、3.3 日历、
// 第 9 节的账、第 15 节验收第 2 条的字面量；意图玩家的选项照 3.2 末条（两侧同一组）。
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include "Ch13Data.h"
#include "BattleHand.h"
#include "ChapterFixture.h"
#include "core/rules/Calendar.h"
#include "core/rules/Cultivation.h"
#include "core/rules/Realm.h"
#include "game/CultivationScene.h"
#include "game/ShopScene.h"
#include "game/WorldScene.h"
#include "script/Command.h"

namespace fanren::test::ch13 {
using core::GameState;
using core::MapObject;
using core::Point;
using core::battle::BattlePhase;
using game::Application;
using game::BattleScene;
using game::CultivationScene;
inline constexpr Point kDirections[]{{0,-1},{1,0},{0,1},{-1,0}};

// 施工图 8.1 的意图玩家：第 12 章那只手（五成门槛、不算下一轮、不吃药）。起点背包里能回血的仍只有养精丹（55）
// 与金疮药（15），对 700 气血不值一手（notes/ch12/balance.md 第 1 节）；8.1 末条的最低合法起点本就是无丹药。
// 五行环（power 0、effect stagger）不在这只手的出手单上：BattleHand::scoredOptions 只列伤人的法术
//（power > 0 或带毒），两场的胜负于是一律不靠五行环——比施工图 8.2「先锁一个」的意图更紧
//（notes/ch13/balance.md 第 4 节）。
inline HandPolicy intentPolicy() {
    HandPolicy result;
    result.healAtPercent = 50;
    result.healWhenDoomed = false;
    result.pills = {};
    return result;
}

inline std::vector<std::string> partyIds(const GameState& state) {
    std::vector<std::string> ids;
    for (const auto& member : state.party) ids.push_back(member.roleId);
    return ids;
}

// 两侧起点：协调者备注（2026-10-13 20:40）用真档核过、本路重读 T1 重生成版再核的值（验收第 2 条
//「day、hp / mp、cultivation、两种灵石、两种妖丹、story_jinsi_can 按真档写死」）。
// retreats 是验收第 2 条的 N：该侧在雾中岛静室按「闭关一年」的次数——真档 C0 与固定冲关种子（面板
// breakthroughSeed：day、修为、境界的哈希）实测后写死的字面量，不从驱动推导（notes/ch13/balance.md 第 3 节）。
struct Side {
    const char* file;
    const char* ending;
    int day;
    int low;
    int mid;
    int hp;
    int cultivation;
    int retreats;
};
inline constexpr Side kFirst{"ch12-end-first.sav", "ch13-end-first.sav", 49560, 2331, 20, 614, 4072, 2};
inline constexpr Side kSecond{"ch12-end-second.sav", "ch13-end-second.sav", 49535, 2311, 20, 614, 4011, 2};

// 两侧相同、同样按真档写死的起点：五级 / 六级妖丹 0 / 0、金丝蚕 3（ch12.jincan == 1）、霓裳草 1。
inline constexpr int kStartWuji = 0;
inline constexpr int kStartLiuji = 0;
inline constexpr int kStartSilk = 3;
inline constexpr int kStartHerbs = 1;
// 施工图 8.1 的上界（16.1 第 48 条；真档前「约 6 次以内」）。两侧的 N 都是字面量，「N ≤ 6」在编译期就定了，
// 不写成运行时的常量比较（测试审查 T-4）。
inline constexpr int kRetreatCeiling = 6;
static_assert(kFirst.retreats <= kRetreatCeiling && kSecond.retreats <= kRetreatCeiling,
              "acceptance #2: each side's retreat count N must stay within design 8.1's ceiling");
// 3.3 末行：章末 day − 起点 day ＝ 11790 ＋ 360 × N。
inline constexpr int kChapterDays = 11790;
inline constexpr int kRetreatDays = 360;

// 验收第 2 条：「learnedMagics 恰第 12 章终点那 12 门（第 10 章 11 门 ＋ magic_shijin_chongqun）」。
inline const std::set<std::string>& twelveMagics() {
    static const std::set<std::string> kTwelve{
        "magic_huodan_shu", "magic_liusha_shu", "magic_bingdong_shu", "magic_tianyan_shu", "magic_yufeng_jue",
        "magic_ji_jinfu", "magic_ji_qingjiao", "magic_qingyuan_jianmang", "magic_ji_qinghuozhang",
        "magic_ji_xiaodao", "magic_ji_hongxianzhen", "magic_shijin_chongqun"};
    return kTwelve;
}

// 验收第 2 条终点：「本章新 story 八件各 1」（第 9 节第 8 条的列名次序）。
inline constexpr std::array<const char*, 8> kNewStories{
    "story_mihuan_yan", "story_baiyu_dayin", "story_huangse_zhihuan", "story_guyu_yujian", "story_qiyuan_haitu",
    "story_xuanyin_jing", "story_tongpian", "story_banyao_cao"};
// 验收第 2 条终点：「story_wuse_yuanzhu、story_xuangu_leigu、material_lianjing、story_jinsi_can、
// story_liuqi_cailiao 各 0」。
inline constexpr std::array<const char*, 5> kSpentStories{
    "story_wuse_yuanzhu", "story_xuangu_leigu", "material_lianjing", "story_jinsi_can", "story_liuqi_cailiao"};

inline constexpr const char* kLow = "material_lingshi";
inline constexpr const char* kMid = "material_lingshi_zhong";
inline constexpr const char* kWuji = "material_yaodan_wuji";
inline constexpr const char* kLiuji = "material_yaodan_liuji";
inline constexpr const char* kHerb = "herb_nichang_cao";
inline constexpr const char* kSilk = "story_jinsi_can";
inline constexpr const char* kCargo = "story_liuqi_cailiao";

// 3.2 每一处 give / take 与第 9 节的四本账（外加两种吃得到的药、三件只读不扣的旧物）。
inline constexpr std::array<const char*, 31> kAccounts{
    kLow, kMid, kWuji, kLiuji, kHerb, "herb_huangjing_cao", "pill_zhiyuan_dan", kSilk, kCargo,
    "story_miaoyin_yaopai", "story_xuangu_leigu", "material_lianjing", "story_wuse_yuanzhu", "story_yanghun_mugen",
    "story_yanghun_zhu", "story_mihuan_yan", "story_baiyu_dayin", "story_huangse_zhihuan", "story_guyu_yujian",
    "story_qiyuan_haitu", "story_xuanyin_jing", "story_tongpian", "story_banyao_cao", "story_manao_jiao",
    "weapon_qingzhu_fengyun_jian", "story_xuese_pifeng", "story_wuse_tonghuan", "story_jiuqu_lingshen",
    "pill_yangjing_dan", "pill_jinchuang_yao", "pill_huiqi_dan"};

// 3.3 日历：每个节点 advance_days 的合计（意图玩家）。下标 = 节点号；n28 另加静室里按的 360 × N
//（窗口算进 n28：驱动在踩出关带之前坐关）。表里没列的节点一律 0。
inline constexpr std::array<int, 35> kNodeDays{
    0, 15, 0, 0, 0, 1, 2, 0, 0, 1, 1, 1, 0, 0, 0, 0, 14, 10,
    5, 15, 360, 5, 375, 30, 6, 2415, 1095, 30, 7200, 3, 20, 5, 1, 180, 0};

// 第 15 节验收第 2 条与 3.4：每一节点走完必置的旗标（n24 是计数旗 ch13.lieyao，n34 是 ch13.done）。
inline const char* doneFlagOf(int node) { return kChain.at(static_cast<std::size_t>(node - 1)); }

// 意图玩家每一节点排进去的选项（choiceIndex 从 0 数；Lua 侧 pick = index + 1）。照 3.2 末条：
// n03 看下去；n04 应下；n05 随行；n06 婉拒；n07 报名；n11 直说；n12 第三支（只列得出这一支）；n14 看下去；
// n27 就此闭关；n29 回话取第一支（3.2 末条没指定，两支只换一句后合流）、至元丹第二支（六级 1：只列得出
//「六级 / 开炉」两支，取第一项）；n30 这就走；n31 铜片第一支（五级 1：灵石不到 5000，只列「五级 / 以材料抵」）、
//「以整堆材料换」、回绝范夫人。② 两轮、往返、可选支线一样不碰。
inline std::deque<int> intentPicks(int node) {
    switch (node) {
    case 3: case 4: case 5: case 6: case 7: case 11: case 12: case 14: case 27: case 30:
        return {0};
    case 29: return {0, 0};
    case 31: return {0, 0, 0};
    default: return {};
    }
}

// 战斗里我方怎么出手。Intent = 那只手；Flee = 那只手一开打就逃（② 可逃那一支）；Defend = 只防（败的扰动）；
// Custom = 交给 Driver::customFight（切片在用例里经真菜单出手，比如 ① 第一手放五行环；同第 12 章驱动）。
enum class FightMode { Intent, Flee, Defend, Custom };

struct Fight {
    std::string id;
    FightMode mode = FightMode::Intent;
    GameState before;
    GameState after;
    BattlePhase phase = BattlePhase::Ongoing;
    int rounds = 0;
    int enemyHpLeft = 0;
    int heroHpLeft = 0;
    bool interrupted = false;     // 出手记录里有「蓄势被打断」
    std::vector<std::string> allies;
    std::vector<std::string> log;
};

struct Entry {
    int node;
    GameState before;
    GameState after;
};

// 一场仗的结局指纹：手稳不稳就比它（回合、胜负、出手记录、战后存档逐行）。
inline std::vector<std::string> fightFingerprint(const Fight& fight) {
    std::vector<std::string> out{fight.id, std::to_string(static_cast<int>(fight.phase)),
                                 std::to_string(fight.rounds), std::to_string(fight.enemyHpLeft),
                                 std::to_string(fight.heroHpLeft)};
    out.insert(out.end(), fight.log.begin(), fight.log.end());
    const auto lines = comparableSaveLines(fight.after);
    out.insert(out.end(), lines.begin(), lines.end());
    return out;
}

class Driver {
public:
    Driver() = default;
    ~Driver() { if (app_) app_->shutdown(); }
    Driver(const Driver&) = delete;
    Driver& operator=(const Driver&) = delete;
    Application& app() { return *app_; }
    GameState& state() { return app().state(); }
    const GameState& start() const { return start_; }
    const std::vector<Fight>& fights() const { return fights_; }
    // 只给检查自己的负向用：在内存里改坏某一场的战前 / 战后存档，看逐节点账抓不抓得住。
    std::vector<Fight>& mutableFights() { return fights_; }
    const std::vector<Entry>& ledger() const { return ledger_; }
    const std::vector<std::string>& keys() const { return keys_; }
    const std::vector<std::string>& shops() const { return shops_; }
    const std::vector<rules::BreakthroughAttempt>& attempts() const { return attempts_; }
    int retreats() const { return retreats_; }
    const std::string& problem() const { return problem_; }
    // 下一场（或几场）仗换打法；空就按意图玩家。
    std::deque<FightMode> nextModes;
    // 每一场仗用的那只手（平衡扫描可以换取舍，比如 withheldMagics 捂住某一类）。
    HandPolicy policy = intentPolicy();
    // 逐节点账：扰动过的切片关掉（账照意图玩家写死，扰动之后本就不该对上）。
    bool audit = true;
    // 某一节点改走别的选项（切片用）；不在表里的按意图玩家。
    std::map<int, std::deque<int>> picks;
    // FightMode::Custom 的那一场怎么打：拿到真 BattleScene 与那只手（policy 已装好），打到分出胜负为止。
    // 返回 false 即驱动失败（problem() 里带上 customProblem）。
    std::function<bool(BattleScene&, BattleHand&)> customFight;
    std::string customProblem;

    bool load(const Side& side) {
        side_ = &side;
        app_ = std::make_unique<Application>();
        const auto ready = app().init(root().string(), true);
        if (!ready.ok) return fail("Application init: " + ready.error);
        const auto path = chapterFixturePath(root().string(), side.file).string();
        const auto loaded = io::loadGame(path);
        if (!loaded.ok) return fail("C12 fixture: " + loaded.error);
        start_ = loaded.value;
        // 验收第 2 条 SetUp 先验：T1 重生成的那两份（lingquan.lua 末行 teleport 之后落在 ch13_nanming 的 haishang）。
        if (start_.mapId != "ch13_nanming" || start_.position != Point{6,29})
            return fail("C12 fixture is not the regenerated T1 hand-over: " + start_.mapId);
        EXPECT_EQ(start_.day, side.day);
        EXPECT_EQ(start_.flag("ch12.done"), 1);
        EXPECT_EQ(rules::toValue(start_.realm), 31);
        EXPECT_EQ(rules::toValue(start_.realmCap), 31);
        EXPECT_EQ(rules::toValue(start_.formerRealm), 22);
        EXPECT_EQ(start_.hp, side.hp);
        EXPECT_EQ(start_.maxHp, 700);
        EXPECT_EQ(start_.mp, 470);
        EXPECT_EQ(start_.maxMp, 470);
        EXPECT_EQ(start_.cultivation, side.cultivation);
        EXPECT_EQ(start_.itemCount(kLow), side.low);
        EXPECT_EQ(start_.itemCount(kMid), side.mid);
        EXPECT_EQ(start_.itemCount(kWuji), kStartWuji);
        EXPECT_EQ(start_.itemCount(kLiuji), kStartLiuji);
        EXPECT_EQ(start_.itemCount(kSilk), kStartSilk);
        EXPECT_EQ(start_.itemCount(kHerb), kStartHerbs);
        EXPECT_TRUE(start_.party.empty()) << start_.party.size();
        EXPECT_EQ(std::set<std::string>(start_.learnedMagics.begin(), start_.learnedMagics.end()), twelveMagics());
        EXPECT_EQ(start_.learnedMagics.size(), 12u);
        for (const char* id : {"story_miaoyin_yaopai", "story_xuangu_leigu", "material_lianjing",
                               "weapon_qingzhu_fengyun_jian"})
            EXPECT_EQ(start_.itemCount(id), 1) << id;
        EXPECT_EQ(start_.flag("ch13.wenxun"), 0);
        const auto resumed = app().continueJourney(path);
        if (!resumed.ok) return fail("continueJourney: " + resumed.error);
        EXPECT_EQ(comparableSaveLines(state()), comparableSaveLines(start_));
        return true;
    }

    // 从一份真实走到的存档接着走（存盘、重开 Application、读回、落图；同第 10–12 章驱动）。
    // side：这份存档是哪一侧走出来的——开着逐节点账（audit）再 runTo 时必须给（n28 的日数取该侧的字面量 N）。
    bool resumeReached(const GameState& checkpoint, int node, const Side* side = nullptr) {
        side_ = side;
        const TempDir temporary("fanren_ch13_reached_scene");
        const auto file = temporary.path() / "reached.sav";
        const auto saved = io::saveGame(checkpoint, file.string());
        if (!saved.ok) return fail(saved.error);
        if (app_) app_->shutdown();
        app_ = std::make_unique<Application>();
        const auto ready = app().init(root().string(), true);
        if (!ready.ok) return fail(ready.error);
        const auto loaded = io::loadGame(file.string());
        if (!loaded.ok) return fail(loaded.error);
        app().state() = loaded.value;
        const auto map = app().loadMap(checkpoint.mapId, "");
        if (!map.ok) return fail(map.error);
        state().position = checkpoint.position;
        state().facing = checkpoint.facing;
        const auto differences = saveLineDifferences(comparableSaveLines(checkpoint), comparableSaveLines(state()));
        EXPECT_TRUE(differences.empty()) << joinLines(differences);
        next_ = node;
        current_ = node;
        keys_.clear();
        fights_.clear();
        ledger_.clear();
        shops_.clear();
        attempts_.clear();
        retreats_ = 0;
        return true;
    }

    std::string status() {
        std::ostringstream out;
        out << "node=" << current_ << " map=" << state().mapId << " xy=" << state().position.x << ','
            << state().position.y << " day=" << state().day << " realm=" << rules::toValue(state().realm)
            << '/' << rules::toValue(state().realmCap) << " cult=" << state().cultivation
            << " hp=" << state().hp << '/' << state().maxHp << " mp=" << state().mp << '/' << state().maxMp
            << " low=" << state().itemCount(kLow) << " mid=" << state().itemCount(kMid)
            << " wuji=" << state().itemCount(kWuji) << " liuji=" << state().itemCount(kLiuji)
            << " herb=" << state().itemCount(kHerb) << " drops=" << state().bottle.drops
            << " scriptError=" << app().scripts().lastError();
        return out.str();
    }

    MapObject object(const std::string& name) {
        if (const auto* map = app().currentMap())
            for (const auto& o : map->objects) if (o.name == name) return o;
        return {};
    }

    // 此刻打起来的应是哪一场（3.1：南明岛出海口 ①、雾中岛阵心礁 ②；本章没有第三处 battle(，验收 15）。
    std::string expectedBattle() {
        if (state().mapId == "ch13_nanming") return kBattles[0];
        if (state().mapId == "ch13_wuzhong") return kBattles[1];
        return {};
    }

    bool fight(BattleScene& scene, const Fight& record) {
        if (record.mode == FightMode::Intent || record.mode == FightMode::Flee) {
            BattleHand hand(app(), policy);
            hand.fleeAtOnce(record.mode == FightMode::Flee);
            hand.play(scene);
            return true;
        }
        if (record.mode == FightMode::Custom) {
            BattleHand hand(app(), policy);
            return (customFight && customFight(scene, hand)) || fail("Custom fight failed: " + customProblem);
        }
        for (int turn = 0; turn < 8000 && scene.battle().phase() == BattlePhase::Ongoing; ++turn) {
            const int actor = scene.runToAllyTurn();
            if (actor < 0) break;
            core::battle::Action guard;
            guard.kind = core::battle::ActionKind::Defend;
            guard.actorIndex = actor;
            const auto issued = scene.issuePlayerAction(app(), guard);
            if (!issued.ok) return fail("Defend refused: " + issued.error);
        }
        return true;
    }

    // 开打那一刻：记下战前存档、核编成、按 nextModes 打完、记结局。
    bool openFight(BattleScene& battle) {
        Fight record;
        record.id = expectedBattle();
        if (record.id.empty()) return fail("Unexpected battle outside the two design hooks");
        record.before = state();
        const auto* setup = app().battleSetup(record.id);
        if (!setup) return fail("Missing battle " + record.id);
        std::multiset<std::pair<std::string,int>> expected, actual;
        for (const auto& u : setup->units) if (!u.ally) expected.emplace(u.roleId,u.wave);
        for (const auto& u : battle.battle().units()) {
            if (u.ally) record.allies.push_back(u.id);
            else actual.emplace(u.id,u.wave);
        }
        EXPECT_EQ(actual, expected) << record.id;
        if (!nextModes.empty()) {
            record.mode = nextModes.front();
            nextModes.pop_front();
        }
        if (!fight(battle, record)) return false;
        const auto& units = battle.battle().units();
        record.phase = battle.battle().phase();
        record.rounds = battle.battle().round();
        for (const auto& u : units) if (!u.ally) record.enemyHpLeft += std::max(0, u.hp);
        // units[0] 恒是韩立（BattleScene 先摆韩立、再摆编成友军；本章两场都没有友军）。
        record.heroHpLeft = units.empty() || units.front().id != kHandHero ? -1 : units.front().hp;
        record.log = battle.battle().log();
        for (const auto& line : record.log)
            if (line.find("蓄势被打断") != std::string::npos) record.interrupted = true;
        std::cout << "[ch13 battle] " << record.id << " mode=" << static_cast<int>(record.mode)
                  << " phase=" << static_cast<int>(record.phase) << " rounds=" << record.rounds
                  << " enemyHp=" << record.enemyHpLeft << " heroHp=" << record.before.hp << "->"
                  << record.heroHpLeft << " mp=" << record.before.mp << " interrupted="
                  << record.interrupted << ' ' << status() << '\n';
        if (environmentFlagSet("FANREN_CH13_BATTLE_LOG"))
            for (const auto& line : record.log) std::cout << "  " << line << '\n';
        fights_.push_back(std::move(record));
        return true;
    }

    bool pump() {
        int pending = -1;
        const game::ShopScene* handled = nullptr;
        for (int frame = 0; frame < 20000 && app().scripts().isRunning(); ++frame) {
            app().tick(1.0 / 60.0);
            auto* battle = dynamic_cast<BattleScene*>(app().topScene());
            if (pending >= 0 && !battle) {
                fights_[static_cast<std::size_t>(pending)].after = state();
                pending = -1;
            }
            if (battle) {
                if (battle->battle().phase() != BattlePhase::Ongoing) continue;
                if (!openFight(*battle)) return false;
                pending = static_cast<int>(fights_.size()) - 1;
                continue;
            }
            // 脚本 shop()（caigou / mingzhu / cunjin / mishi）：意图玩家不买不卖，按「离开」——ShopScene::leave
            // 回填命令，下一帧它自己退场。同一家店只按一次（退场前那一帧它还在栈顶）。
            if (auto* shop = dynamic_cast<game::ShopScene*>(app().topScene())) {
                if (shop != handled) {
                    handled = shop;
                    shops_.push_back(shop->shopId());
                    shop->leave(app());
                }
                continue;
            }
            handled = nullptr;
            if (!app().awaitingCommand()) continue;
            const bool talked = !app().spokenKeys().empty();
            captureKeys();
            script::CommandResult reply;
            reply.ok = true;
            reply.choiceIndex = 0;
            if (!talked) {
                if (choices_.empty()) return fail("Unplanned choice; queue exhausted");
                reply.choiceIndex = choices_.front();
                choices_.pop_front();
            }
            app().completeCommand(reply);
            app().popScene();
        }
        app().tick(1.0 / 60.0);
        captureKeys();
        if (pending >= 0) fights_[static_cast<std::size_t>(pending)].after = state();
        if (app().scripts().isRunning() || !app().scripts().lastError().empty() || app().quitRequested())
            return fail("Script did not finish normally");
        return true;
    }

    // 走图：与第 10–12 章驱动同一套（BFS 只走玩家真走得上的格；facility、在场 NPC、一踩就响的 enter
    // 触发器都不当路，只有终点例外。本章四张图 0 个 portal）。
    bool passable(Point p, bool destination = false) {
        const auto& map = *app().currentMap();
        if (!map.walkable(p) || game::WorldScene::visibleNpcAt(state(),map,p) || map.objectAt(p,"facility")) return false;
        if (!destination && map.objectAt(p,"portal")) return false;
        const auto* trigger = map.objectAt(p,"trigger");
        return !trigger || trigger->property("mode") != "enter" || !game::WorldScene::triggerReady(state(),*trigger) || destination;
    }

    bool move(Point direction) {
        const bool moved = world_.tryStep(app(),direction.x,direction.y);
        return pump() && moved;
    }

    bool walk(Point goal, bool allowEnter = false) {
        if (state().position == goal) return true;
        const auto& map = *app().currentMap();
        const auto key = [&map](Point p) { return p.y * map.width + p.x; };
        const Point from = state().position;
        std::map<int,Point> previous{{key(from),from}};
        std::deque<Point> queue{from};
        while (!queue.empty() && !previous.count(key(goal))) {
            const Point p = queue.front(); queue.pop_front();
            for (const Point d : kDirections) {
                const Point n{p.x+d.x,p.y+d.y};
                if (n.x < 0 || n.y < 0 || n.x >= map.width || n.y >= map.height || previous.count(key(n))) continue;
                if (!passable(n,allowEnter && n == goal)) continue;
                previous[key(n)] = p;
                queue.push_back(n);
            }
        }
        if (!previous.count(key(goal))) return false;
        std::vector<Point> route;
        for (Point p=goal; p!=from; p=previous.at(key(p))) route.push_back(p);
        std::reverse(route.begin(),route.end());
        const std::string mapId = state().mapId;
        const auto* goalTrigger = allowEnter ? map.objectAt(goal,"trigger") : nullptr;
        const std::string completed = goalTrigger ? goalTrigger->property("set_flag") : "";
        const int wasCompleted = completed.empty() ? 0 : state().flag(completed);
        for (const Point p : route) {
            const Point at = state().position;
            if (!move({p.x-at.x,p.y-at.y})) return false;
            if (!completed.empty() && wasCompleted==0 && state().flag(completed)!=0) return true;
            if (state().mapId != mapId || state().position != p) return p == goal;
        }
        return state().position == goal;
    }

    // 站到 stand 上、转身朝 o（stand 须与 o 相邻）。
    bool faceFrom(const MapObject& o, Point stand) {
        const Point d{o.position.x-stand.x,o.position.y-stand.y};
        if (std::abs(d.x)+std::abs(d.y) != 1 || !walk(stand)) return false;
        world_.tryStep(app(),d.x,d.y);
        return pump() && state().position == stand;
    }

    bool face(const MapObject& o) {
        for (const Point d : kDirections) {
            const Point stand{o.position.x-d.x,o.position.y-d.y};
            if (!passable(stand)) continue;
            if (!passable(o.position)) {
                if (!walk(stand)) continue;
                world_.tryStep(app(),d.x,d.y);
                return pump();
            }
            const Point approach{stand.x-d.x,stand.y-d.y};
            if (!passable(approach) || !walk(approach)) continue;
            if (move(d) && state().position == stand) return true;
        }
        return false;
    }

    bool press(const std::string& name) {
        const auto o = object(name);
        if (o.name.empty() || !face(o) || !world_.interact(app())) return fail("Cannot interact with " + name);
        return pump();
    }

    bool pressFrom(const std::string& name, Point stand) {
        const auto o = object(name);
        if (o.name.empty() || !faceFrom(o,stand) || !world_.interact(app())) return fail("Cannot interact with " + name);
        return pump();
    }

    bool enter(const std::string& name) {
        const auto o = object(name);
        if (o.name.empty()) return fail("Missing enter hook " + name);
        for (int y=0; y<o.height; ++y) for (int x=0; x<o.width; ++x) {
            const Point goal{o.position.x+x,o.position.y+y};
            if (state().position == goal) {
                for (const Point d : kDirections) {
                    const Point beside{goal.x-d.x,goal.y-d.y};
                    if (passable(beside) && walk(beside) && move(d)) return true;
                }
            } else if (walk(goal,true)) return true;
        }
        return fail("Cannot walk onto " + name);
    }

    bool open(const std::string& name) {
        const auto o = object(name);
        if (o.name.empty() || !face(o) || !world_.interact(app())) return fail("Cannot open " + name);
        app().tick(1.0/60.0);
        return true;
    }
    void close() { app().popScene(); app().tick(1.0/60.0); }

    // 打坐设施真打坐（CultivationScene::meditateFor）：静养一日回一成（Application::advanceDays）。
    bool sit(const std::string& facility, int days) {
        if (!open(facility)) return false;
        auto* panel = dynamic_cast<CultivationScene*>(app().topScene());
        if (!panel) return fail("Meditation facility did not open CultivationScene: " + facility);
        const int day = state().day;
        panel->meditateFor(app(),days);
        close();
        return state().day == day+days || fail("Meditation did not advance the calendar");
    }

    // 1.3 / 3.2 末条：n27 抬上限之后，意图玩家在雾中岛静室按「闭关一年」直到冲成。真入口：
    // openFacility(facility_jingshi_wz) → CultivationScene（effectiveness 150）；「闭关一年」那一档从面板自己的
    // 档位表里找（meditateOptions），修为够这一关的门槛就按冲关（breakthrough，种子是面板的 breakthroughSeed）；
    // 失败倒扣两成之后接着坐。retreats_ 只数按了几次「闭关一年」，拿去与 Side::retreats 那个字面量对照。
    bool retreat() {
        if (!open("facility_jingshi_wz")) return false;
        auto* panel = dynamic_cast<CultivationScene*>(app().topScene());
        if (!panel) return fail("The Wuzhong room did not open CultivationScene");
        const auto& options = CultivationScene::meditateOptions(game::wordingStage(state()));
        const auto year = std::find_if(options.begin(), options.end(),
                                       [](const game::MeditateOption& o) { return o.label == "闭关一年"; });
        if (year == options.end() || year->days != rules::kDaysPerYear) return fail("The panel has no year-long retreat");
        for (int round = 0; round < 16 && rules::toValue(state().realm) < 32; ++round) {
            if (state().cultivation >= rules::cultivationNeeded(state().realm)) {
                attempts_.push_back(panel->breakthrough(app()));
                std::cout << "[ch13 retreat] breakthrough success=" << attempts_.back().success
                          << " blocked=" << static_cast<int>(attempts_.back().blocked)
                          << " lost=" << attempts_.back().cultivationLost << ' ' << status() << '\n';
                if (attempts_.back().blocked != rules::BreakthroughBlock::None) break;
                continue;
            }
            const int day = state().day;
            panel->meditateFor(app(), year->days);
            ++retreats_;
            std::cout << "[ch13 retreat] year " << retreats_ << ' ' << status() << '\n';
            if (state().day != day + year->days) return fail("A year in the room did not pass a year");
        }
        close();
        return rules::toValue(state().realm) == 32 || fail("The retreat never broke through to CoreMid");
    }

    // n24：阵心礁按两次（② 胜 2 次、不逃），每次 ch13.lieyao ＋1。
    bool lure(int rounds) {
        for (int round = 0; round < rounds; ++round) {
            const int before = state().flag("ch13.lieyao");
            if (!press("trigger_youyao")) return false;
            if (state().flag("ch13.lieyao") != before + 1) return fail("A lure round did not count");
        }
        return true;
    }

    // 脚本 teleport 的落点（3.1 末表）：#2–#4 落 default spawn、#5 落秘市岸坐标；落地不触发脚下 enter。
    bool landed(const char* mapId, Point cell) {
        if (state().mapId != mapId || state().position != cell)
            return fail(std::string("Script transfer did not land on ") + mapId);
        return true;
    }

    // 章末站位：洞口 trigger_dongkou (61,44) 西侧那一格、面朝东按（18.2 第 7 条「章末站 (60,44)」，genmaps_ch13.py）。
    static constexpr Point kEndCell{60,44};

    bool step(int node) {
        const auto custom = picks.find(node);
        choices_ = custom != picks.end() ? custom->second : intentPicks(node);
        switch (node) {
        case 1: return enter("trigger_wenxun");
        case 2: return press("npc_yuanliao_zhanggui");
        case 3: return enter("trigger_qiuling");
        case 4: return press("npc_cang_yunlong");
        case 5: return enter("trigger_chugang");
        case 6: return press("npc_ling_yuling") && landed("ch13_tianxing", Point{27,36});
        case 7: return enter("trigger_rucheng");
        case 8: return press("trigger_dongfu39");
        case 9: return enter("trigger_xingkong");
        case 10: return press("trigger_zhaiyuan");
        case 11: return enter("trigger_lanlu");
        case 12: return press("npc_gu_laozhe") && landed("ch13_waihai", Point{34,7});
        case 13: return enter("trigger_shiwu");
        case 14: return press("npc_ming_furen");
        case 15: return press("npc_li_xun");
        case 16: return enter("trigger_chucheng") && landed("ch13_wuzhong", Point{59,37});
        case 17: return enter("trigger_lifu");
        case 18: return press("trigger_chongshi");
        case 19: return press("trigger_shichuang");
        case 20: return press("trigger_lianjianshi");
        case 21: return enter("trigger_dizhen");
        case 22: return press("trigger_biguanshi");
        case 23: return enter("trigger_buzhen");
        case 24: return lure(2);
        case 25: return press("trigger_shoushou");
        case 26: return press("trigger_danfang");
        case 27: return press("trigger_jingshimen");
        case 28: return retreat() && enter("trigger_chuguan");
        case 29: return press("npc_gongsun_xing");
        case 30: return enter("trigger_lidao") && landed("ch13_waihai", Point{14,46});
        case 31: return press("trigger_mishi");
        case 32: return press("npc_wen_siyue");
        case 33: return enter("trigger_jiaoshi");
        case 34: return pressFrom("trigger_dongkou", kEndCell);
        default: return fail("No such node");
        }
    }

    bool runTo(int last) {
        for (; next_ <= last; ++next_) {
            current_ = next_;
            Entry entry{next_, state(), {}};
            const std::size_t firstFight = fights_.size();
            if (!step(next_) || state().flag(doneFlagOf(next_)) == 0)
                return fail("Node failed: " + std::to_string(next_) + " " + doneFlagOf(next_));
            // 排进去的选项必须在本节点用完：剩下的会被下一处选择错拿去。
            if (!choices_.empty())
                return fail("Choice queue not drained at node " + std::to_string(next_));
            entry.after = state();
            if (audit) {
                if (side_ == nullptr) return fail("The node ledger needs the side this save came from (load / resumeReached)");
                checkEntry(*side_, entry, firstFight);
            }
            ledger_.push_back(std::move(entry));
        }
        return true;
    }

    bool spoke(const std::string& key) const { return std::find(keys_.begin(),keys_.end(),key) != keys_.end(); }
    void clearKeys() { keys_.clear(); app().clearSpokenKeys(); }
    void setCurrentNode(int node) { current_ = node; }
    // 切片直接 press / enter 某个挂点之前，排好那一下要用的选项（-1 ＝ 取消；越界由 ScriptHost 按取消处理）。
    void choose(std::deque<int> picksForNextHook) { choices_ = std::move(picksForNextHook); }
    bool choicesDrained() const { return choices_.empty(); }

    // 章末：验收第 2 条「终点先验」逐项照抄（字面量，不从驱动推导）。
    void checkEnding(const Side& side) {
        SCOPED_TRACE(std::string("ch13 ending from ") + side.file);
        EXPECT_EQ(state().flag("ch13.done"), 1);
        EXPECT_EQ(rules::toValue(state().realm), 32);
        EXPECT_EQ(rules::toValue(state().realmCap), 32);
        EXPECT_EQ(rules::toValue(state().formerRealm), 22);
        EXPECT_EQ(state().maxHp, rules::realmMaxHp(rules::Realm::CoreMid));
        EXPECT_EQ(state().maxMp, rules::realmMaxMp(rules::Realm::CoreMid));
        std::set<std::string> fourteen = twelveMagics();
        fourteen.insert("magic_ji_wuxinghuan");
        fourteen.insert("magic_fengyun_heji");
        EXPECT_EQ(std::set<std::string>(state().learnedMagics.begin(), state().learnedMagics.end()), fourteen);
        EXPECT_EQ(state().learnedMagics.size(), 14u);
        EXPECT_TRUE(state().party.empty()) << state().party.size();
        const std::map<std::string, int> flags{{"ch13.lieyao", 2}, {"ch13.toudu", 3}, {"ch13.zhiyuan", 2},
                                               {"ch13.tongpian", 1}};
        for (const auto& [flag, value] : flags) EXPECT_EQ(state().flag(flag), value) << flag;
        // 第 9 节第 8 条章末账：低阶 ＝ 起点 ＋ 600、中阶 ＝ 起点、六级 ＝ 起点 ＋ 7、五级 ＝ 起点 ＋ 13、霓裳草 ＝ 起点 ＋ 1。
        EXPECT_EQ(state().itemCount(kLow), side.low + 600);
        EXPECT_EQ(state().itemCount(kMid), side.mid);
        EXPECT_EQ(state().itemCount(kLiuji), kStartLiuji + 7);
        EXPECT_EQ(state().itemCount(kWuji), kStartWuji + 13);
        EXPECT_EQ(state().itemCount(kHerb), kStartHerbs + 1);
        for (const char* id : kNewStories) EXPECT_EQ(state().itemCount(id), 1) << id;
        for (const char* id : kSpentStories) EXPECT_EQ(state().itemCount(id), 0) << id;
        // 腰牌只读不扣（3.2 n04）。
        EXPECT_EQ(state().itemCount("story_miaoyin_yaopai"), 1);
        // day − 起点 day ＝ 11790 ＋ 360 × N，N 是该侧的字面量（N ≤ 6 见 kRetreatCeiling 处的 static_assert）；
        // 驱动自己数的按键次数只拿来对照。
        EXPECT_EQ(state().day - side.day, kChapterDays + kRetreatDays * side.retreats);
        EXPECT_EQ(retreats_, side.retreats) << "静室里按「闭关一年」的次数与字面量 N 不符";
        // 落点：n34 不 teleport，人站在按 interact 的那一格、面朝洞口（东）。
        EXPECT_EQ(state().mapId, "ch13_waihai");
        EXPECT_EQ(state().position, kEndCell);
        EXPECT_EQ(state().facing, 1);
        // 意图玩家一路走零成本那一支、一路有东西可扣：兜底与「不足」的旁白一句也不该出现。
        for (const char* key : {"ch13.wenxun.ring_fail", "ch13.chuansong.paid_stone", "ch13.chuansong.paid_dan",
             "ch13.daoqiyuan.look_stone", "ch13.daoqiyuan.look_dan", "ch13.xuanyin.jing_fail",
             "ch13.chonglian.no_crystal", "ch13.chonglian.join_fail", "ch13.wusezhu.refine_fail",
             "ch13.wusezhu.silk_fail", "ch13.youyao.herb_gone", "ch13.youyao.no_herb", "ch13.youyao.no_drop",
             "ch13.youyao.escaped", "ch13.youyao.closed", "ch13.chongmo.early", "ch13.biguan.wait",
             "ch13.biguan.pill_fail", "ch13.chuguan.not_yet", "ch13.gongsun.got_pill", "ch13.gongsun.furnace",
             "ch13.mishi.paid_stone", "ch13.mishi.paid_pile", "ch13.mishi.wait", "ch13.mishi.price_fail",
             "ch13.mishi.leave", "ch13.jianshi.cost_stone", "ch13.jianshi.cost_pile"})
            EXPECT_FALSE(spoke(key)) << key;
        for (const char* key : {"ch13.wenxun.ring", "ch13.chuansong.paid_blade", "ch13.daoqiyuan.look_blade",
             "ch13.chongshi.moth", "ch13.wusezhu.silk_gone", "ch13.chonglian.join", "ch13.gongsun.got_dan",
             "ch13.mishi.paid_dan", "ch13.mishi.price", "ch13.mishi.refuse", "ch13.jianshi.cost_dan"})
            EXPECT_TRUE(spoke(key)) << key;
        // 意图玩家不买不卖：三家店各进一次、按「离开」（caigou、mingzhu 首谈、mishi；cunjin 首谈不开店）。
        EXPECT_EQ(shops_, (std::vector<std::string>{"ch13_nanming_fangshi", "ch13_mingzhu_shougou", "ch13_mishi_tan"}));
        std::vector<std::string> ids;
        std::vector<BattlePhase> phases;
        for (const auto& fight : fights_) {
            ids.push_back(fight.id);
            phases.push_back(fight.phase);
        }
        // 验收第 15 条（通关那一半）：两侧各记实际打到的编成恰是这 2 个（② 2 次）、都胜。
        EXPECT_EQ(ids, (std::vector<std::string>{kBattles[0], kBattles[1], kBattles[1]}));
        EXPECT_EQ(phases, std::vector<BattlePhase>(3, BattlePhase::Won));
    }

    // 一场仗里账上每一样的应得变化：② 胜则 drops（六级 1 ＋ 五级 2，rate 100，第 9 节第 6 条），其余一律 0
    //（两场 spirit_stones 0；意图手不吃药；① drops []）。
    static int fightDelta(const Fight& fight, const std::string& id) {
        if (fight.id != kBattles[1] || fight.phase != BattlePhase::Won) return 0;
        if (id == kLiuji) return 1;
        if (id == kWuji) return 2;
        return 0;
    }

    // 逐节点账：3.2 的 take / give（「有几扣几」或走退路）＋ 这一节点里各场仗的实收实付；3.3 的日子。
    // firstFight / endFight：这一节点里打的那几场在 fights() 里的下标区间（endFight 缺省 ＝ 到末尾）。
    // n28 的日数取该侧的字面量 N（side.retreats），驱动自己数的按键次数不进断言右边（验收第 2 条、测试审查 T-4）。
    void checkEntry(const Side& side, const Entry& entry, std::size_t firstFight,
                    std::size_t endFight = static_cast<std::size_t>(-1)) {
        SCOPED_TRACE("ch13 node " + std::to_string(entry.node));
        endFight = std::min(endFight, fights_.size());
        const auto& before = entry.before;
        const auto& after = entry.after;
        int days = kNodeDays.at(static_cast<std::size_t>(entry.node));
        if (entry.node == 28) days += kRetreatDays * side.retreats;
        EXPECT_EQ(after.day - before.day, days);
        std::map<std::string, int> deltas;
        const auto add = [&](const char* id, int count = 1) { deltas[id] += count; };
        const auto take = [&](const char* id, int count) { deltas[id] -= std::min(before.itemCount(id), count); };
        int won = 0;
        for (std::size_t i = firstFight; i < endFight; ++i) {
            for (const char* id : kAccounts) deltas[id] += fightDelta(fights_[i], id);
            if (fights_[i].phase == BattlePhase::Won) ++won;
        }
        switch (entry.node) {
        case 5: add("story_mihuan_yan"); add("story_baiyu_dayin"); add(kLow, 600); break;
        case 7: add("story_huangse_zhihuan"); break;
        case 15: add("story_guyu_yujian"); break;
        case 16: add("story_qiyuan_haitu"); break;
        case 17: add(kHerb, 3); break;
        case 18: add("story_wuse_yuanzhu"); break;
        case 19: take("story_xuangu_leigu", 1); add("story_xuanyin_jing"); break;
        case 20: take("material_lianjing", 1); break;
        case 22: take("story_wuse_yuanzhu", 1); take(kSilk, before.itemCount(kSilk)); break;
        case 24: add(kHerb, -won); break;
        case 25: add(kLiuji, 6); add(kWuji, 10); add(kCargo); break;
        case 29: take(kLiuji, 1); break;
        case 31: take(kWuji, 1); add("story_tongpian"); take(kCargo, 1); break;
        case 34: add("story_banyao_cao"); break;
        default: break;
        }
        for (const char* id : kAccounts) {
            for (std::size_t i = firstFight; i < endFight; ++i)
                EXPECT_EQ(fights_[i].after.itemCount(id) - fights_[i].before.itemCount(id), fightDelta(fights_[i], id))
                    << id << " changed inside " << fights_[i].id;
            EXPECT_EQ(after.itemCount(id) - before.itemCount(id), deltas[id]) << id;
        }
        // 境界：上限只在 n27 抬（biguan.lua 的 realm.cap），境界只在 n28 那一格之前的静室里由玩家冲（1.3）。
        EXPECT_EQ(rules::toValue(after.realmCap), rules::toValue(before.realmCap) + (entry.node == 27 ? 1 : 0));
        EXPECT_EQ(rules::toValue(after.realm), rules::toValue(before.realm) + (entry.node == 28 ? 1 : 0));
        EXPECT_EQ(after.formerRealm, before.formerRealm);
        // 修为只从仗（n05、n24 的 rewards）与静室（n28）来；脚本不发修为（3.2 末条）。
        if (entry.node != 5 && entry.node != 24 && entry.node != 28) EXPECT_EQ(after.cultivation, before.cultivation);
        // 法术：n01 学五行环、n20 学蜂云合剑，别处一门不多。
        const std::size_t learned = (entry.node == 1 || entry.node == 20) ? 1u : 0u;
        EXPECT_EQ(after.learnedMagics.size(), before.learnedMagics.size() + learned);
        EXPECT_TRUE(after.party.empty()) << after.party.size();
        std::cout << "[ch13 ledger] node=" << entry.node << " day=" << before.day << "->" << after.day
                  << " low=" << before.itemCount(kLow) << "->" << after.itemCount(kLow)
                  << " wuji=" << before.itemCount(kWuji) << "->" << after.itemCount(kWuji)
                  << " liuji=" << before.itemCount(kLiuji) << "->" << after.itemCount(kLiuji)
                  << " cult=" << before.cultivation << "->" << after.cultivation
                  << " hp=" << before.hp << "->" << after.hp << " mp=" << before.mp << "->" << after.mp << '\n';
    }

private:
    bool fail(const std::string& why) { problem_ = why; if (app_) problem_ += "\n" + status(); return false; }
    void captureKeys() {
        keys_.insert(keys_.end(), app().spokenKeys().begin(), app().spokenKeys().end());
        app().clearSpokenKeys();
    }

    std::unique_ptr<Application> app_;
    game::WorldScene world_;
    GameState start_;
    std::vector<Fight> fights_;
    std::vector<Entry> ledger_;
    std::vector<std::string> keys_;
    std::vector<std::string> shops_;
    std::vector<rules::BreakthroughAttempt> attempts_;
    std::deque<int> choices_;
    std::string problem_;
    const Side* side_ = nullptr;
    int retreats_ = 0;
    int next_ = 1, current_ = 1;
};

// 意图玩家一整趟（每侧一份、同一进程里只走一次）：切片与平衡从这里取「真走到的那一刻」，不手搭起点。
// ctest 每条用例单开进程，缓存只省直接跑 fanren_tests.exe 时的重复；口径与逐条现走相同。
struct Walk {
    std::string problem;
    std::vector<Entry> ledger;   // ledger[n - 1].after ＝ 第 n 节点走完的那一刻
    std::vector<Fight> fights;
    int retreats = 0;
    GameState end;
};

inline const Walk& intentWalk(const Side& side) {
    static std::map<std::string, Walk> cache;
    if (const auto found = cache.find(side.file); found != cache.end()) return found->second;
    Walk walk;
    Driver driver;
    if (!driver.load(side) || !driver.runTo(34)) walk.problem = driver.problem();
    walk.ledger = driver.ledger();
    walk.fights = driver.fights();
    walk.retreats = driver.retreats();
    walk.end = driver.state();
    return cache.emplace(side.file, std::move(walk)).first->second;
}

// 第 node 节点走完的那一刻（node ≥ 1）；没走到就是空状态，调用方先验 walk.problem。
inline GameState reachedAfter(const Walk& walk, int node) {
    const auto index = static_cast<std::size_t>(node - 1);
    return index < walk.ledger.size() ? walk.ledger[index].after : GameState{};
}
} // namespace fanren::test::ch13
