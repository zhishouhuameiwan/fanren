#pragma once

// 第 12 章通关驱动（测试路 B）：从真实的第 11 章终档（T1 重生成版，落在 ch12_tushan 的 andeng）起步，
// 走真实入口——WorldScene 走图与 interact、真门（portal_dixue / portal_tushan）、openFacility 开的
// CultivationScene、BattleScene 真打（tests/BattleHand.h 那只手）。不改 state 注入结果；切片里的扰动
// 只在用例里写明。判据一律抄 docs/ch12-design.md：3.2 动作、3.3 日历、第 15 节验收第 2 条的字面量，
// 意图玩家的选项照研究笔记 N.1 第 5 条（施工图 9 节第 8 条引用的那一串）。
// 本章 n05 曲魂离队之后队伍为空，四场的我方 ＝ 韩立 ＋ 编成里写死的友军（契约 E1）。
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
#include "Ch12Data.h"
#include "BattleHand.h"
#include "ChapterFixture.h"
#include "core/rules/Cultivation.h"
#include "core/rules/Objectives.h"
#include "core/rules/Realm.h"
#include "game/CultivationScene.h"
#include "game/WorldScene.h"
#include "script/Command.h"

namespace fanren::test::ch12 {
using core::GameState;
using core::MapObject;
using core::Point;
using core::battle::BattlePhase;
using game::Application;
using game::BattleScene;
using game::CultivationScene;
inline constexpr Point kDirections[]{{0,-1},{1,0},{0,1},{-1,0}};

// 施工图 8.1 的意图玩家：第 10、11 章那只手（五成门槛、不算下一轮），只差一处——**不吃药**。
// 起点背包里能回血的只有养精丹（55）与金疮药（15）：对 700 气血不值一手，第 11 章四场也一颗没吃；
// 照旧「五成就吃」的话，韩立一过半血就连吃十来回合、一下不打，量出来的是这条门槛的脾气、不是编成
//（平衡路实测，见 notes/ch12/balance.md 第 0 节）。施工图 8.1 末条的「最低合法起点」本就是无丹药。
// 神雷（talisman_bixie_shenlei）也不在这只手的物品单上：BattleHand 只认 pills / poisons，
// 单体 castMagic 的道具它当毒药估（实打 0），所以四场的胜负一律不靠神雷——比施工图 8.2 的意图更紧。
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

// 两侧起点：协调者备注用真档核过的值（验收第 2 条「day、hp / mp、两种灵石按真档写死」）。
struct Side {
    const char* file;
    const char* ending;
    int day;
    int low;
    int mid;
};
inline constexpr Side kFirst{"ch11-end-first.sav", "ch12-end-first.sav", 49378, 2339, 20};
inline constexpr Side kSecond{"ch11-end-second.sav", "ch12-end-second.sav", 49353, 2319, 20};

// 验收第 2 条：「learnedMagics 恰第 10 章那 11 门」（施工图 8.1 列名）。
inline const std::set<std::string>& elevenMagics() {
    static const std::set<std::string> kEleven{
        "magic_huodan_shu", "magic_liusha_shu", "magic_bingdong_shu", "magic_tianyan_shu", "magic_yufeng_jue",
        "magic_ji_jinfu", "magic_ji_qingjiao", "magic_qingyuan_jianmang", "magic_ji_qinghuozhang",
        "magic_ji_xiaodao", "magic_ji_hongxianzhen"};
    return kEleven;
}

// 验收第 2 条终点：「下列 23 种 story 各恰 1」（按条目原文的先后）。
inline constexpr std::array<const char*, 23> kEndStories{
    "story_hualan", "story_bihuo_baoyi", "story_jiuqu_lingshen", "story_xuanyin_huan", "story_baixi_pei",
    "story_hanbing_zhu", "story_tihun_shou", "story_minghun_zhu", "story_poluo_zhu", "story_qingming_zhen",
    "story_huanglin_jia", "story_xuese_pifeng", "story_wuse_tonghuan", "story_kuilei_canhai",
    "story_neidian_juanzhou", "story_yu_ruyi", "story_jiuqu_danfang", "story_xutian_ding", "story_qianlan_zhu",
    "story_xuangu_leigu", "story_jinlei_xiaojian", "story_wannian_lingru", "story_yanghun_mugen"};

// 3.2 每一处 give / take 与第 9 节的账；外加两种吃得到的药（只在战斗里动）。
inline constexpr const char* kThunder = "talisman_bixie_shenlei";
inline constexpr const char* kHerb = "herb_huangjing_zhi";   // 18.1 第 5 条 / 18.4 第 4 条：O-13 认定的灵草
inline constexpr std::array<const char*, 37> kAccounts{
    "material_lingshi", "material_lingshi_zhong", "story_bai_zhenzhu", "story_zhiyang_huazhou", "story_hualan",
    "story_bihuo_baoyi", "story_jiuqu_lingshen", "story_xuanyin_huan", "story_baixi_pei", "story_hanbing_zhu",
    "story_tihun_shou", "story_minghun_zhu", "material_lianjing", "story_poluo_zhu", "story_qingming_zhen",
    "story_huanglin_jia", "story_xuese_pifeng", "story_wuse_tonghuan", "story_kuilei_canhai",
    "story_neidian_juanzhou", "story_jinsi_can", "story_yu_ruyi", "story_jiuqu_danfang", "story_xutian_ding",
    "story_qianlan_zhu", "story_xuangu_leigu", "story_jinlei_xiaojian", "story_wannian_lingru",
    "story_yanghun_mugen", kHerb, "story_jinpa_ditu", "story_yinhun_zhong", "story_xueyu_zhizhu",
    "story_shijin_chong", "weapon_qingzhu_fengyun_jian", "pill_yangjing_dan", "pill_jinchuang_yao"};

// 3.3 日历：每个节点 advance_days 的合计（意图玩家：n20 两处罗烟步、n24 / n32 付得起低阶）。下标 = 节点号。
inline constexpr std::array<int, 34> kNodeDays{
    0, 62, 90, 0, 0, 0, 0, 4, 8, 0, 8, 3, 0, 0, 0, 1, 0,
    0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 3};

// 第 15 节验收第 2 条与 3.1：每一节点走完必置的旗标（n33 是 ch12.done）。
inline const char* doneFlagOf(int node) {
    static constexpr std::array<const char*, 34> kDone{
        "", "ch12.dengdao", "ch12.pozhen", "ch12.tuizhu", "ch12.qixialian", "ch12.duoshe", "ch12.canyuan",
        "ch12.lidao", "ch12.rudian", "ch12.jiumeng", "ch12.qunxiang", "ch12.chuansong", "ch12.tupo",
        "ch12.guiwang", "ch12.yuanyao", "ch12.xiaojie", "ch12.lingshen", "ch12.fanlian", "ch12.baishi",
        "ch12.xiagu", "ch12.rongyan", "ch12.tihun", "ch12.lianjing", "ch12.shidian", "ch12.quba",
        "ch12.huanjing", "ch12.neidian", "ch12.cengjie", "ch12.juanzhou", "ch12.ruyi", "ch12.butian",
        "ch12.xuangu", "ch12.canhun", "ch12.done"};
    return kDone.at(static_cast<std::size_t>(node));
}

// 意图玩家每一节点排进去的选项（choiceIndex 从 0 数；Lua 侧 pick = index + 1）。照 N.1 第 5 条：
// n01 应允；n02 一次选对（第三项「换」）；n03 藏拙；n04 喝住、展画轴；n05 敲钟；n06 搪塞；n09 应；n10 让；
// Z1 取衣（第 13 节点里、先于鬼王）；n13 喝醒（第二项）；n14 拒；n16 换旗；n18 拜极阴；n19 熔岩路；
// n20 两处罗烟步；n21 闯（不绕路）；n23 四门任一（取第一扇）；n24 先取披风；n25 两段一次选对；
// Z2 收（n27）；n29 讨要；n30 溜（被叫回）；n32 吞。
inline std::deque<int> intentPicks(int node) {
    switch (node) {
    case 1: case 3: case 5: case 9: case 10: case 14: case 18: case 23: case 24: case 27: case 29: case 30: case 32:
        return {0};
    case 2: return {2};
    case 4: return {0, 1};
    case 6: case 16: case 19: case 21: return {1};
    case 13: return {0, 1};          // Z1 取衣，再 n13 喝醒
    case 20: return {0, 0};
    case 25: return {1, 1};
    default: return {};
    }
}

// 战斗里我方怎么出手。Intent = 那只手；Defend = 人人只防（败的扰动）；
// AlliesOnly = 韩立只防、写死的友军照那只手打（③「两名友军打不死它」的判据）；
// Custom = 交给 Driver::customFight（切片在用例里一手一手经真菜单打，比如 ④ 放辟邪神雷）。
enum class FightMode { Intent, Defend, AlliesOnly, Custom };

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
        app_ = std::make_unique<Application>();
        const auto ready = app().init(root().string(), true);
        if (!ready.ok) return fail("Application init: " + ready.error);
        const auto path = chapterFixturePath(root().string(), side.file).string();
        const auto loaded = io::loadGame(path);
        if (!loaded.ok) return fail("C11 fixture: " + loaded.error);
        start_ = loaded.value;
        // 验收第 2 条 SetUp 先验：T1 重生成的那两份（C1 patch 之后落在 ch12_tushan 的 andeng）。
        if (start_.mapId != "ch12_tushan" || start_.position != Point{24,31})
            return fail("C11 fixture is not the regenerated T1 hand-over: " + start_.mapId);
        EXPECT_EQ(start_.day, side.day);
        EXPECT_EQ(start_.flag("ch11.done"), 1);
        EXPECT_EQ(rules::toValue(start_.realm), 31);
        EXPECT_EQ(rules::toValue(start_.realmCap), 31);
        EXPECT_EQ(rules::toValue(start_.formerRealm), 22);
        EXPECT_EQ(start_.hp, 700);
        EXPECT_EQ(start_.maxHp, 700);
        EXPECT_EQ(start_.mp, 470);
        EXPECT_EQ(start_.maxMp, 470);
        EXPECT_EQ(start_.itemCount("material_lingshi"), side.low);
        EXPECT_EQ(start_.itemCount("material_lingshi_zhong"), side.mid);
        EXPECT_EQ(partyIds(start_), std::vector<std::string>{"qu_hun_shadan"});
        EXPECT_EQ(std::set<std::string>(start_.learnedMagics.begin(), start_.learnedMagics.end()), elevenMagics());
        EXPECT_EQ(start_.learnedMagics.size(), 11u);
        for (const char* id : {"story_jinpa_ditu", "story_bai_zhenzhu", "weapon_qingzhu_fengyun_jian",
                               "story_xueyu_zhizhu", "story_shijin_chong", "story_yinhun_zhong"})
            EXPECT_EQ(start_.itemCount(id), 1) << id;
        EXPECT_EQ(start_.flag("ch12.dengdao"), 0);
        const auto resumed = app().continueJourney(path);
        if (!resumed.ok) return fail("continueJourney: " + resumed.error);
        EXPECT_EQ(comparableSaveLines(state()), comparableSaveLines(start_));
        return true;
    }

    // 从一份真实走到的存档接着走（存盘、重开 Application、读回、落图；同第 10、11 章驱动）。
    bool resumeReached(const GameState& checkpoint, int node) {
        const TempDir temporary("fanren_ch12_reached_scene");
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
        return true;
    }

    std::string status() {
        std::ostringstream out;
        out << "node=" << current_ << " map=" << state().mapId << " xy=" << state().position.x << ','
            << state().position.y << " day=" << state().day << " realm=" << rules::toValue(state().realm)
            << " hp=" << state().hp << '/' << state().maxHp << " mp=" << state().mp << '/' << state().maxMp
            << " low=" << state().itemCount("material_lingshi") << " mid=" << state().itemCount("material_lingshi_zhong")
            << " party=" << state().party.size() << " scriptError=" << app().scripts().lastError();
        return out.str();
    }

    MapObject object(const std::string& name) {
        if (const auto* map = app().currentMap())
            for (const auto& o : map->objects) if (o.name == name) return o;
        return {};
    }

    // 此刻打起来的应是哪一场（3.1：鬼雾 ①、冰火道 ②、寒骊台 ruyi 之前 ③、之后 ④）。
    std::string expectedBattle() {
        if (state().mapId == "ch12_guiwu") return kBattles[0];
        if (state().mapId == "ch12_binghuo") return kBattles[1];
        if (state().mapId == "ch12_hanlitai") return state().flag("ch12.ruyi") == 0 ? kBattles[2] : kBattles[3];
        return {};
    }

    bool fight(BattleScene& scene, const Fight& record) {
        BattleHand hand(app(), policy);
        if (record.mode == FightMode::Intent) {
            hand.play(scene);
            return true;
        }
        if (record.mode == FightMode::Custom)
            return (customFight && customFight(scene, hand)) || fail("Custom fight failed: " + customProblem);
        for (int turn = 0; turn < 8000 && scene.battle().phase() == BattlePhase::Ongoing; ++turn) {
            const int actor = scene.runToAllyTurn();
            if (actor < 0) break;
            const auto& me = scene.battle().units()[static_cast<std::size_t>(actor)];
            if (record.mode == FightMode::AlliesOnly && me.id != kHandHero) {
                if (!hand.act(scene, actor)) return fail("Ally could not act");
                continue;
            }
            core::battle::Action guard;
            guard.kind = core::battle::ActionKind::Defend;
            guard.actorIndex = actor;
            const auto issued = scene.issuePlayerAction(app(), guard);
            if (!issued.ok) return fail("Defend refused: " + issued.error);
        }
        return true;
    }

    bool pump() {
        int pending = -1;
        for (int frame = 0; frame < 20000 && app().scripts().isRunning(); ++frame) {
            app().tick(1.0 / 60.0);
            auto* battle = dynamic_cast<BattleScene*>(app().topScene());
            if (pending >= 0 && !battle) {
                fights_[static_cast<std::size_t>(pending)].after = state();
                pending = -1;
            }
            if (battle) {
                if (battle->battle().phase() != BattlePhase::Ongoing) continue;
                Fight record;
                record.id = expectedBattle();
                if (record.id.empty()) return fail("Unexpected battle outside the four design hooks");
                record.before = state();
                const auto* setup = app().battleSetup(record.id);
                if (!setup) return fail("Missing battle " + record.id);
                std::multiset<std::pair<std::string,int>> expected, actual;
                for (const auto& u : setup->units) if (!u.ally) expected.emplace(u.roleId,u.wave);
                for (const auto& u : battle->battle().units()) {
                    if (u.ally) record.allies.push_back(u.id);
                    else actual.emplace(u.id,u.wave);
                }
                EXPECT_EQ(actual, expected) << record.id;
                record.mode = FightMode::Intent;
                if (!nextModes.empty()) {
                    record.mode = nextModes.front();
                    nextModes.pop_front();
                }
                if (!fight(*battle, record)) return false;
                const auto& units = battle->battle().units();
                record.phase = battle->battle().phase();
                record.rounds = battle->battle().round();
                for (const auto& u : units) if (!u.ally) record.enemyHpLeft += std::max(0, u.hp);
                // 契约 E1：units[0] 恒是韩立（BattleScene 先摆韩立、再摆编成友军、再 addPartyUnits）。
                record.heroHpLeft = units.empty() || units.front().id != kHandHero ? -1 : units.front().hp;
                record.log = battle->battle().log();
                for (const auto& line : record.log)
                    if (line.find("蓄势被打断") != std::string::npos) record.interrupted = true;
                std::cout << "[ch12 battle] " << record.id << " mode=" << static_cast<int>(record.mode)
                          << " phase=" << static_cast<int>(record.phase) << " rounds=" << record.rounds
                          << " enemyHp=" << record.enemyHpLeft << " heroHp=" << record.before.hp << "->"
                          << record.heroHpLeft << " mp=" << record.before.mp << " interrupted="
                          << record.interrupted << ' ' << status() << '\n';
                if (environmentFlagSet("FANREN_CH12_BATTLE_LOG"))
                    for (const auto& line : record.log) std::cout << "  " << line << '\n';
                pending = static_cast<int>(fights_.size());
                fights_.push_back(std::move(record));
                continue;
            }
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

    // 走图：与第 10、11 章驱动同一套（BFS 只走玩家真走得上的格；门、facility、在场 NPC、
    // 一踩就响的 enter 触发器都不当路，只有终点例外）。
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

    // 本章唯一一对门（荒岛 ↔ 地洞）：真踩门格。
    bool travel(const std::string& destination) {
        for (int hop=0; hop<4 && state().mapId != destination; ++hop) {
            const auto* link = rules::nextPortalToward(app().portalLinks(),state().mapId,destination,state());
            if (!link) return fail("No real portal to " + destination);
            const auto o = object(link->portalName);
            const std::string from = state().mapId;
            if (o.name.empty() || !walk(o.position,true) || state().mapId == from) return fail("Portal traversal failed toward " + destination);
        }
        return state().mapId == destination || fail("Did not reach " + destination);
    }

    bool open(const std::string& name) {
        const auto o = object(name);
        if (o.name.empty() || !face(o) || !world_.interact(app())) return fail("Cannot open " + name);
        app().tick(1.0/60.0);
        return true;
    }
    void close() { app().popScene(); app().tick(1.0/60.0); }

    // 四处打坐设施之一真打坐（CultivationScene::meditateFor）：静养一日回一成（Application::advanceDays）。
    bool sit(const std::string& facility, int days) {
        if (!open(facility)) return false;
        auto* panel = dynamic_cast<CultivationScene*>(app().topScene());
        if (!panel) return fail("Meditation facility did not open CultivationScene: " + facility);
        const int day = state().day;
        panel->meditateFor(app(),days);
        close();
        return state().day == day+days || fail("Meditation did not advance the calendar");
    }

    // 章末落点：第 13 章给 lingquan.lua 末尾补了 teleport("ch13_nanming")（契约 interfaces-p3-ch13.md 5.3、
    // ch13 施工图 3.1 #1、16.1 第 1 条），落 default spawn haishang（maps/ch13_nanming.tmj 的 192,928 → 格 (6,29)，
    // facing up → 0）。teleport 之后不触发脚下 enter；第 13 章 n01 的横贯带在它北面一格，这里不迈步。
    static constexpr Point kEndCell{6,29};
    bool landedOnNanming() {
        if (state().mapId != "ch13_nanming" || state().position != kEndCell)
            return fail("lingquan.lua did not hand over to ch13_nanming haishang");
        return true;
    }

    bool step(int node) {
        const auto custom = picks.find(node);
        choices_ = custom != picks.end() ? custom->second : intentPicks(node);
        switch (node) {
        case 1: return enter("trigger_dengdao");
        case 2: return press("trigger_huangwu");
        case 3: return enter("trigger_guaizhao");
        case 4: return travel("ch12_dixue") && press("trigger_shuichi");
        case 5: return enter("trigger_shizhu");
        case 6: return press("npc_shaonian");
        case 7: return travel("ch12_tushan") && press("npc_wen_siyue");
        case 8: return enter("trigger_rudian");
        case 9: return press("npc_xuan_gu");
        case 10: return enter("trigger_qunxiang");
        case 11: return press("trigger_chuansongzhen");
        case 12: return enter("trigger_tupo");
        case 13: return press("trigger_yihai") && enter("trigger_guiwang");
        case 14: return enter("trigger_huayuan");
        case 15: return press("trigger_tingzi");
        case 16: return press("trigger_caiyao") && press("trigger_shishan");
        case 17: return enter("trigger_fanlian");
        case 18: return press("npc_jiyin");
        case 19: return enter("trigger_xiagu");
        case 20: return enter("trigger_guangbi");
        case 21: return enter("trigger_shamo");
        case 22: return press("trigger_jingzhu");
        case 23: return enter("trigger_shidian");
        case 24: return enter("trigger_yuqiao");
        case 25: return enter("trigger_changlang");
        case 26: return enter("trigger_taqian");
        case 27: return press("trigger_tamen");
        case 28: return press("trigger_qingshi");
        case 29: return enter("trigger_taijie");
        case 30: return enter("trigger_jitan");
        case 31: return press("trigger_dongkou");
        case 32: return press("trigger_xiaojian");
        case 33: return press("npc_yuan_yao_lq") && landedOnNanming();
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
            if (audit) checkEntry(entry, firstFight);
            ledger_.push_back(std::move(entry));
        }
        return true;
    }

    bool spoke(const std::string& key) const { return std::find(keys_.begin(),keys_.end(),key) != keys_.end(); }
    void clearKeys() { keys_.clear(); app().clearSpokenKeys(); }
    void setCurrentNode(int node) { current_ = node; }

    // 章末：验收第 2 条「终点先验」逐项照抄（字面量，不从驱动推导）。
    void checkEnding(const Side& side) {
        SCOPED_TRACE(std::string("ch12 ending from ") + side.file);
        EXPECT_EQ(state().flag("ch12.done"), 1);
        EXPECT_EQ(rules::toValue(state().realm), 31);
        EXPECT_EQ(rules::toValue(state().realmCap), 31);
        EXPECT_EQ(rules::toValue(state().formerRealm), 22);
        EXPECT_EQ(state().maxHp, 700);
        EXPECT_EQ(state().maxMp, 470);
        EXPECT_TRUE(state().party.empty()) << state().party.size();
        std::set<std::string> twelve = elevenMagics();
        twelve.insert("magic_shijin_chongqun");
        EXPECT_EQ(std::set<std::string>(state().learnedMagics.begin(), state().learnedMagics.end()), twelve);
        EXPECT_EQ(state().learnedMagics.size(), 12u);
        for (const char* id : {"story_bai_zhenzhu", "story_zhiyang_huazhou", kThunder})
            EXPECT_EQ(state().itemCount(id), 0) << id;
        EXPECT_EQ(state().itemCount("material_lianjing"), 1);
        EXPECT_EQ(state().itemCount("story_jinsi_can"), 3);
        for (const char* id : kEndStories) EXPECT_EQ(state().itemCount(id), 1) << id;
        for (const char* id : {"story_jinpa_ditu", "story_yinhun_zhong", "story_xueyu_zhizhu", "story_shijin_chong"})
            EXPECT_EQ(state().itemCount(id), start_.itemCount(id)) << id;
        EXPECT_EQ(state().itemCount(kHerb), start_.itemCount(kHerb) + 2);
        EXPECT_EQ(state().itemCount("material_lingshi"), side.low - 8);
        EXPECT_EQ(state().itemCount("material_lingshi_zhong"), side.mid);
        const std::map<std::string, int> flags{{"ch12.huanqi", 2}, {"ch12.quba_xian", 1}, {"ch12.xinjing", 3},
            {"ch12.jincan", 1}, {"ch12.z1", 1}, {"ch12.z2", 1}, {"ch12.caiyao", 1}};
        for (const auto& [flag, value] : flags) EXPECT_EQ(state().flag(flag), value) << flag;
        EXPECT_EQ(state().day - side.day, 182);
        // 第 13 章契约 5.3：lingquan.lua 末行 teleport("ch13_nanming")，落 haishang (6,29) 朝上。
        EXPECT_EQ(state().mapId, "ch13_nanming");
        EXPECT_EQ(state().position, kEndCell);
        EXPECT_EQ(state().facing, 0);
        // 意图玩家一路付得起、一路有东西可扣：兜底与「不足」的旁白一句也不该出现。
        for (const char* key : {"ch12.dengdao.pearl_none", "ch12.duoshe.scroll_gone", "ch12.duoshe.not_in_party",
             "ch12.guairen.learn_fail", "ch12.lidao.no_map", "ch12.quba.paid_zhong", "ch12.quba.paid_none",
             "ch12.canhun.paid_zhong", "ch12.canhun.paid_none", "ch12.guiwang.keep", "ch12.xuangu.keep"})
            EXPECT_FALSE(spoke(key)) << key;
        std::vector<std::string> ids;
        std::vector<BattlePhase> phases;
        for (const auto& fight : fights_) {
            ids.push_back(fight.id);
            phases.push_back(fight.phase);
        }
        // 验收第 14 条（通关那一半）：两侧各记实际打到的编成，恰是这 4 个、各一次、都胜。
        EXPECT_EQ(ids, (std::vector<std::string>{kBattles[0], kBattles[1], kBattles[2], kBattles[3]}));
        EXPECT_EQ(phases, std::vector<BattlePhase>(4, BattlePhase::Won));
    }

    // 逐节点账：3.2 的 take / give（「有几扣几」或走退路）＋ 这一节点里各场仗的实收实付；3.3 的日子。
    // firstFight / endFight：这一节点里打的那几场在 fights() 里的下标区间（endFight 缺省 ＝ 到末尾）。
    void checkEntry(const Entry& entry, std::size_t firstFight, std::size_t endFight = static_cast<std::size_t>(-1)) {
        SCOPED_TRACE("ch12 node " + std::to_string(entry.node));
        endFight = std::min(endFight, fights_.size());
        const auto& before = entry.before;
        const auto& after = entry.after;
        const int low = before.itemCount("material_lingshi");
        const int mid = before.itemCount("material_lingshi_zhong");
        // n24 / n32 灵石三档：低阶 4 → 中阶 1 → 推演半日（advance_days(1)）。
        const bool stonesShort = low < 4 && mid < 1;
        int days = kNodeDays.at(static_cast<std::size_t>(entry.node));
        if ((entry.node == 24 || entry.node == 32) && stonesShort) days += 1;
        EXPECT_EQ(after.day - before.day, days);
        std::map<std::string, int> deltas;
        const auto add = [&](const char* id, int count = 1) { deltas[id] += count; };
        const auto take = [&](const char* id, int count) { deltas[id] -= std::min(before.itemCount(id), count); };
        const auto stones = [&] {
            if (low >= 4) add("material_lingshi", -4);
            else if (mid >= 1) add("material_lingshi_zhong", -1);
        };
        switch (entry.node) {
        case 1: take("story_bai_zhenzhu", 1); break;
        case 3: add("story_hualan"); add("story_zhiyang_huazhou"); break;
        case 5: take("story_zhiyang_huazhou", 1); break;
        case 13: add("story_bihuo_baoyi"); break;
        case 16: add(kHerb, 2); add("story_jiuqu_lingshen"); break;
        case 18: add("story_xuanyin_huan"); break;
        case 19: add("story_baixi_pei"); add("story_hanbing_zhu"); break;
        case 21: add("story_tihun_shou"); add("story_minghun_zhu"); break;
        case 22: add("material_lianjing"); break;
        case 23: add("story_poluo_zhu"); add("story_qingming_zhen"); add("story_huanglin_jia"); break;
        case 24: stones(); add("story_xuese_pifeng"); add("story_wuse_tonghuan"); break;
        case 27: add("story_kuilei_canhai"); break;
        case 28: add("story_neidian_juanzhou"); break;
        case 29: add("story_jinsi_can", 3); add("story_yu_ruyi"); break;
        case 31:
            add("story_jiuqu_danfang"); add("story_xutian_ding"); add("story_qianlan_zhu");
            add("story_xuangu_leigu"); add("story_jinlei_xiaojian");
            break;
        case 32: stones(); break;
        case 33: add("story_wannian_lingru"); add("story_yanghun_mugen"); break;
        default: break;
        }
        // 战斗收支直接断 0（测试审查 T-6）：本章四场 drops []、spirit_stones 0，意图手不吃药、神雷不在 kAccounts——
        // 仗里多出或少了账上任何一样，都是「战斗偷偷给了 / 拿了东西」，不许当作合法收支抵进节点账。
        for (const char* id : kAccounts) {
            for (std::size_t i = firstFight; i < endFight; ++i)
                EXPECT_EQ(fights_[i].after.itemCount(id) - fights_[i].before.itemCount(id), 0)
                    << id << " changed inside " << fights_[i].id;
            EXPECT_EQ(after.itemCount(id) - before.itemCount(id), deltas[id]) << id;
        }
        // 神雷：n13、n31 战前各给 2、战后有几收几（拍板 19）——每个节点走完都是 0。
        EXPECT_EQ(after.itemCount(kThunder), 0) << kThunder;
        if (entry.node == 13 || entry.node == 31) {
            EXPECT_EQ(endFight, firstFight + 1);
            if (endFight == firstFight + 1) EXPECT_EQ(fights_[firstFight].before.itemCount(kThunder), 2);
        }
        EXPECT_EQ(after.realm, before.realm);
        EXPECT_EQ(after.realmCap, before.realmCap);
        EXPECT_EQ(after.formerRealm, before.formerRealm);
        const std::vector<std::string> party = entry.node < 5 ? std::vector<std::string>{"qu_hun_shadan"}
                                                              : std::vector<std::string>{};
        EXPECT_EQ(partyIds(after), party);
        std::cout << "[ch12 ledger] node=" << entry.node << " day=" << before.day << "->" << after.day
                  << " low=" << low << "->" << after.itemCount("material_lingshi")
                  << " mid=" << mid << "->" << after.itemCount("material_lingshi_zhong")
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
    std::deque<int> choices_;
    std::string problem_;
    int next_ = 1, current_ = 1;
};

// 意图玩家一整趟（每侧一份、同一进程里只走一次）：切片与平衡从这里取「真走到的那一刻」，不手搭起点。
// ctest 每条用例单开进程，缓存只省直接跑 fanren_tests.exe 时的重复；口径与逐条现走相同。
struct Walk {
    std::string problem;
    std::vector<Entry> ledger;   // ledger[n - 1].after ＝ 第 n 节点走完的那一刻
    std::vector<Fight> fights;
    GameState end;
};

inline const Walk& intentWalk(const Side& side) {
    static std::map<std::string, Walk> cache;
    if (const auto found = cache.find(side.file); found != cache.end()) return found->second;
    Walk walk;
    Driver driver;
    if (!driver.load(side) || !driver.runTo(33)) walk.problem = driver.problem();
    walk.ledger = driver.ledger();
    walk.fights = driver.fights();
    walk.end = driver.state();
    return cache.emplace(side.file, std::move(walk)).first->second;
}

// 第 node 节点走完的那一刻（node ≥ 1）；没走到就是空状态，调用方先验 walk.problem。
inline GameState reachedAfter(const Walk& walk, int node) {
    const auto index = static_cast<std::size_t>(node - 1);
    return index < walk.ledger.size() ? walk.ledger[index].after : GameState{};
}
} // namespace fanren::test::ch12
