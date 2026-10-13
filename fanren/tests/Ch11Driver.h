#pragma once

// 第 11 章通关驱动（测试路 B）：从真实的第 10 章终档起步，走真实入口——WorldScene 走图与
// interact、openFacility 开的 CultivationScene / ShopScene / AlchemyScene、BattleScene 真打
//（tests/BattleHand.h 那只手）。不改 state 注入结果；切片里的扰动只在用例里写明。
// 判据一律抄 docs/ch11-design.md：3.2 动作、3.3 日历、第 15 节验收第 2 条的字面量。
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include "Ch11Data.h"
#include "BattleHand.h"
#include "ChapterFixture.h"
#include "core/rules/Cultivation.h"
#include "core/rules/Realm.h"
#include "game/AlchemyScene.h"
#include "game/CultivationScene.h"
#include "game/ShopScene.h"
#include "game/WorldScene.h"

namespace fanren::test::ch11 {
using core::GameState;
using core::MapObject;
using core::Point;
using core::battle::BattlePhase;
using game::Application;
using game::BattleScene;
using game::CultivationScene;
inline constexpr Point kDirections[]{{0,-1},{1,0},{0,1},{-1,0}};

// 施工图 8.1 的意图玩家：与第 10 章同一只手（五成吃药、不算下一轮、先养精丹后金疮药）。
inline HandPolicy intentPolicy() {
    HandPolicy result;
    result.healAtPercent = 50;
    result.healWhenDoomed = false;
    result.pills = {"pill_yangjing_dan", "pill_jinchuang_yao"};
    return result;
}

inline std::vector<std::string> partyIds(const GameState& state) {
    std::vector<std::string> ids;
    for (const auto& member : state.party) ids.push_back(member.roleId);
    return ids;
}

// 两侧起点：施工图第 2 节实读、验收第 2 条的先验字面量。
struct Side {
    const char* file;
    const char* ending;
    int day;
    int low;
    int mid;
};
inline constexpr Side kFirst{"ch10-end-first.sav", "ch11-end-first.sav", 16003, 254, 117};
inline constexpr Side kSecond{"ch10-end-second.sav", "ch11-end-second.sav", 15978, 234, 116};

// 账上要逐节点核的物品（3.2 每一处 take / give 与第 9 节四本账）。
inline constexpr std::array<const char*, 22> kAccounts{
    "material_lingshi", "material_lingshi_zhong", "material_yaodan_wuji", "material_yaodan_liuji",
    "herb_nichang_cao", "pill_jiangchen_dan", "material_xuelingshui", "pill_dingyan_dan",
    "material_lianqi_yao", "material_tianleizhu", "material_jinleizhu", "material_lianjian_fuliao",
    "weapon_qingzhu_fengyun_jian", "story_jinzhi_lingpai", "story_kuilei_qun", "story_kuilei_can",
    "story_yaoshou_ziliao", "story_yaoshou_cailiao", "story_kuilei_yuan_qun", "story_miaoyin_yaopai",
    "story_bai_zhenzhu", "story_waihai_tu"};

// 3.3 日历：每个节点推进的日数（n29 另按「开店起恰 7200 日」核）。下标 = 节点号。
inline constexpr std::array<int, 32> kNodeDays{
    0, 0, 0, 0, 0, 0, 900, 0, 180, 0, 0, 0, 12, 3, 1800, 735, 21600,
    0, 0, 0, 0, 15, 0, 0, 30, 0, 720, 0, 1080, -1, 180, 0};

// 战斗里我方怎么出手。Intent = 那只手；Flee = 一开打就逃（意图玩家的 ②）；
// Defend = 只防（败的扰动）；AlliesOnly = 韩立与曲魂只防、写死的友军只放自己的那门法术（④ 判据）。
enum class FightMode { Intent, Flee, Defend, AlliesOnly };

struct Fight {
    std::string id;
    FightMode mode = FightMode::Intent;
    GameState before;
    GameState after;
    BattlePhase phase = BattlePhase::Ongoing;
    int rounds = 0;
    int enemyHpLeft = 0;
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
                                 std::to_string(fight.rounds), std::to_string(fight.enemyHpLeft)};
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
    const std::vector<Entry>& ledger() const { return ledger_; }
    const std::vector<std::string>& keys() const { return keys_; }
    const std::string& problem() const { return problem_; }
    // 下一场（或几场）仗换打法；空就按意图玩家：② 逃、其余那只手打。
    std::deque<FightMode> nextModes;
    // 每一场仗用的那只手（平衡扫描可以换取舍）。
    HandPolicy policy = intentPolicy();

    bool load(const Side& side) {
        app_ = std::make_unique<Application>();
        const auto ready = app().init(root().string(), true);
        if (!ready.ok) return fail("Application init: " + ready.error);
        const auto path = chapterFixturePath(root().string(), side.file).string();
        const auto loaded = io::loadGame(path);
        if (!loaded.ok) return fail("C10 fixture: " + loaded.error);
        start_ = loaded.value;
        // 验收第 2 条 SetUp 先验：E1 patch 之后重生成的那两份，字面量照抄。
        if (start_.mapId != "ch11_fangshi" || start_.position != Point{28,38})
            return fail("C10 fixture is not the regenerated E1 hand-over: " + start_.mapId);
        EXPECT_EQ(start_.day, side.day);
        EXPECT_EQ(start_.flag("ch10.done"), 1);
        EXPECT_EQ(rules::toValue(start_.realm), 23);
        EXPECT_EQ(rules::toValue(start_.realmCap), 23);
        EXPECT_EQ(rules::toValue(start_.formerRealm), 22);
        EXPECT_EQ(start_.hp, 480);
        EXPECT_EQ(start_.maxHp, 480);
        EXPECT_EQ(start_.mp, 320);
        EXPECT_EQ(start_.maxMp, 320);
        EXPECT_EQ(start_.itemCount("material_lingshi_zhong"), side.mid);
        EXPECT_EQ(start_.itemCount("material_lingshi"), side.low);
        EXPECT_EQ(start_.itemCount("pill_jiangchen_dan"), 5);
        EXPECT_EQ(start_.itemCount("material_xuelingshui"), 2);
        EXPECT_EQ(start_.itemCount("pill_zhuji_dan"), 13);
        EXPECT_EQ(start_.itemCount("material_yaodan_wuji"), 1);
        EXPECT_EQ(start_.learnedMagics.size(), 11u);
        EXPECT_EQ(partyIds(start_), std::vector<std::string>{"qu_hun_shadan"});
        EXPECT_EQ(start_.bottle.capacity, 6);
        for (const char* id : {"story_shijin_chong", "story_xueyu_zhizhu", "story_juhun_bo"})
            EXPECT_EQ(start_.itemCount(id), 1) << id;
        EXPECT_EQ(start_.itemCount("story_hunyuan_bo"), 0);
        const auto resumed = app().continueJourney(path);
        if (!resumed.ok) return fail("continueJourney: " + resumed.error);
        EXPECT_EQ(comparableSaveLines(state()), comparableSaveLines(start_));
        return true;
    }

    // 从一份真实走到的存档接着走（同第 10 章驱动的口径：存盘、重开 Application、读回、落图）。
    bool resumeReached(const GameState& checkpoint, int node) {
        const TempDir temporary("fanren_ch11_reached_scene");
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
        return true;
    }

    std::string status() {
        std::ostringstream out;
        out << "node=" << current_ << " map=" << state().mapId << " xy=" << state().position.x << ','
            << state().position.y << " day=" << state().day << " realm=" << rules::toValue(state().realm)
            << " hp=" << state().hp << '/' << state().maxHp << " mp=" << state().mp << '/' << state().maxMp
            << " low=" << state().itemCount("material_lingshi") << " mid=" << state().itemCount("material_lingshi_zhong")
            << " herb=" << state().itemCount("herb_nichang_cao") << " drops=" << state().bottle.drops
            << " scriptError=" << app().scripts().lastError();
        return out.str();
    }

    MapObject object(const std::string& name) {
        if (const auto* map = app().currentMap())
            for (const auto& o : map->objects) if (o.name == name) return o;
        return {};
    }

    // 此刻打起来的应是哪一场（3.2：红瑚岛 lieyao == 4 ∧ jiguan == 0 走 ②，否则 ①；礁岛 guiju 前 ③、后 ④）。
    std::string expectedBattle() {
        if (state().mapId == "ch11_honghu")
            return state().flag("ch11.lieyao") == 4 && state().flag("ch11.jiguan") == 0 ? kBattles[1] : kBattles[0];
        if (state().mapId == "ch11_yinsha_dao") return state().flag("ch11.guiju") == 0 ? kBattles[2] : kBattles[3];
        return {};
    }

    bool fight(BattleScene& scene, Fight& record) {
        if (record.mode == FightMode::Intent || record.mode == FightMode::Flee) {
            BattleHand hand(app(), policy);
            hand.fleeAtOnce(record.mode == FightMode::Flee);
            hand.play(scene);
            return true;
        }
        BattleHand menu(app(), policy);
        for (int turn = 0; turn < 8000 && scene.battle().phase() == BattlePhase::Ongoing; ++turn) {
            const int actor = scene.runToAllyTurn();
            if (actor < 0) break;
            core::battle::Action action;
            action.kind = core::battle::ActionKind::Defend;
            action.actorIndex = actor;
            const auto& me = scene.battle().units()[static_cast<std::size_t>(actor)];
            if (record.mode == FightMode::AlliesOnly && me.id != kHandHero && me.id != "qu_hun_shadan" && !me.magics.empty()) {
                for (std::size_t i = 0; i < scene.battle().units().size(); ++i) {
                    const auto& foe = scene.battle().units()[i];
                    if (foe.ally || !foe.alive()) continue;
                    core::battle::Action cast;
                    cast.kind = core::battle::ActionKind::Cast;
                    cast.actorIndex = actor;
                    cast.targetIndex = static_cast<int>(i);
                    cast.magicId = me.magics.front();
                    if (game::refusePlayerAction(state(), scene.battle(), cast).empty()) action = cast;
                    break;
                }
            }
            if (action.kind == core::battle::ActionKind::Cast && menu.issue(scene, action)) continue;
            action.kind = core::battle::ActionKind::Defend;
            const auto issued = scene.issuePlayerAction(app(), action);
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
                if (!nextModes.empty()) {
                    record.mode = nextModes.front();
                    nextModes.pop_front();
                } else {
                    record.mode = record.id == kBattles[1] ? FightMode::Flee : FightMode::Intent;
                }
                if (!fight(*battle, record)) return false;
                record.phase = battle->battle().phase();
                record.rounds = battle->battle().round();
                for (const auto& u : battle->battle().units()) if (!u.ally) record.enemyHpLeft += std::max(0, u.hp);
                record.log = battle->battle().log();
                std::cout << "[ch11 battle] " << record.id << " mode=" << static_cast<int>(record.mode)
                          << " phase=" << static_cast<int>(record.phase) << " rounds=" << record.rounds
                          << " enemyHp=" << record.enemyHpLeft << ' ' << status() << '\n';
                if (environmentFlagSet("FANREN_CH11_BATTLE_LOG"))
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

    // 走图：与第 10 章驱动同一套（BFS 只走玩家真走得上的格；门、facility、在场 NPC、
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

    bool travel(const std::string& destination) {
        for (int hop=0; hop<8 && state().mapId != destination; ++hop) {
            const auto* link = rules::nextPortalToward(app().portalLinks(),state().mapId,destination,state());
            if (!link) return fail("No real portal to " + destination);
            const auto o = object(link->portalName);
            const std::string from = state().mapId;
            if (o.name.empty() || !walk(o.position,true) || state().mapId == from) return fail("Portal traversal failed toward " + destination);
        }
        return state().mapId == destination || fail("Did not reach " + destination);
    }

    // 茶馆雅间是坊市图里同图的一对门（18.1 第 3 条）：进门、出门都得真踩门格。
    bool throughDoor(const char* portal, Point landing) {
        const auto o = object(portal);
        if (o.name.empty() || !walk(o.position,true) || state().position != landing)
            return fail(std::string("Same-map door failed: ") + portal);
        return true;
    }
    bool enterTeahouse() { return throughDoor("portal_chaguan", Point{44,8}); }
    bool leaveTeahouse() { return throughDoor("portal_chaguan_chu", Point{40,10}); }

    bool open(const std::string& name) {
        const auto o = object(name);
        if (o.name.empty() || !face(o) || !world_.interact(app())) return fail("Cannot open " + name);
        app().tick(1.0/60.0);
        return true;
    }
    void close() { app().popScene(); app().tick(1.0/60.0); }

    // 三十九层静室真打坐（CultivationScene::meditateFor），切片拨日子用。
    bool sit(int days) {
        if (!open("facility_jingshi_39")) return false;
        auto* panel = dynamic_cast<CultivationScene*>(app().topScene());
        if (!panel) return fail("Meditation facility did not open CultivationScene");
        const int day = state().day;
        panel->meditateFor(app(),days);
        close();
        return state().day == day+days || fail("Meditation did not advance the calendar");
    }

    // 坊市杂货真买霓裳草（ShopScene::buyAt）。意图玩家买 2 株而不是施工图写的 1 株：
    // youyao.lua 开头「草 ≥ 1」对 ② 那一次同样要判，四胜扣完四株之后第五次按得要还剩一株，
    // ② 逃又不扣草，所以章末剩 1 株（验收第 2 条原写「买 1、章末 0」；施工图 18.5 整改记录 T-3 改按买 2、章末 1）。
    static constexpr int kHerbsBought = 2;
    bool buyHerb(int count = kHerbsBought) {
        if (!travel("ch11_fangshi") || !open("facility_zahuo")) return false;
        auto* shop = dynamic_cast<game::ShopScene*>(app().topScene());
        const auto* stock = app().shop("ch11_fangshi_zahuo");
        if (!shop || !stock) return fail("The herb shop did not open ShopScene");
        int row = -1;
        for (std::size_t i = 0; i < stock->entries.size(); ++i)
            if (stock->entries[i].itemId == "herb_nichang_cao") row = static_cast<int>(i);
        bool bought = row >= 0;
        for (int i = 0; bought && i < count; ++i) bought = shop->buyAt(app(), row);
        const std::string why = shop->feedback();
        shop->leave(app());
        app().tick(1.0/60.0);
        return bought || fail("Could not buy a herb: " + why);
    }

    // 炼器台真炼一次（openFacility → AlchemyScene(Forge, 3) → craftAt）。
    bool forge() {
        if (!open("facility_lianqi_39")) return false;
        auto* furnace = dynamic_cast<game::AlchemyScene*>(app().topScene());
        if (!furnace) return fail("The forge did not open AlchemyScene");
        EXPECT_EQ(furnace->kind(), rules::CraftKind::Forge);
        EXPECT_EQ(furnace->toolGrade(), 3);
        const auto& ids = furnace->recipeIds();
        const auto found = std::find(ids.begin(), ids.end(), "recipe_qingzhu_fengyun_jian");
        const bool crafted = found != ids.end() && furnace->craftAt(app(), static_cast<int>(found-ids.begin()));
        const std::string why = furnace->feedback();
        furnace->leave(app());
        app().tick(1.0/60.0);
        return crafted || fail("The forge refused the sword: " + why);
    }

    // 剑炼出来之前，密室门口 (10,15) 的 trigger_chujian 踩上去只空跑一次（18.1 第 4 条）；
    // 它又是进密室的唯一一格，所以要真踩过去，并核它什么也没做。
    bool stepIntoSecretRoom() {
        const int day = state().day;
        if (!walk(Point{11,15}) || !move(Point{-1,0}) || state().position != Point{10,15})
            return fail("Cannot step into the secret room");
        EXPECT_EQ(state().flag("ch11.lianjian"), 0);
        EXPECT_EQ(state().day, day);
        return true;
    }

    bool peizhu() { return travel("ch11_dongfu") && press("trigger_peizhu"); }

    // n28 回访：坐到青竹小轩柜台后（trigger_jiesuan）。
    bool visitShop() {
        return travel("ch11_xiaoxuan") && press("trigger_jiesuan");
    }

    // 洞府门外（禁制格 (16,21) 以南那片空地）、金青 (14,23) 正南那一格：n31 从这里面朝北搭话，
    // 章末就站在这里（施工图 18.5 写死 (14,24)，n31 脚本不挪人）。取朝北这一格是因为 continueJourney 读档后朝向回到 spawn men 的朝北（引擎不恢复
    // facing，已报整改）；站别的格，读档前后朝向就对不上。
    static constexpr Point kEndCell{14,24};

    bool step(int node) {
        switch (node) {
        case 1: return enter("trigger_chudao");
        case 2: return press("npc_baiyi_xiushi");
        case 3: choose({0}); return press("npc_lu_er");
        case 4: return press("npc_xinggong_zhishi");
        case 5: return enter("trigger_anju");
        case 6: return press("trigger_kuilei");
        case 7: return travel("ch11_fangshi") && press("npc_baimen_zhanggui");
        case 8: return travel("ch11_dongfu") && press("trigger_nichang") && buyHerb();
        case 9: return travel("ch11_xingkong") && press("npc_xingkong_xiushi");
        case 10:
            choose({0});
            if (!enter("trigger_ningcui")) return false;
            choose({0});
            return press("npc_fan_xiaozi");
        case 11: return travel("ch11_honghu") && enter("trigger_buzhen");
        case 12:
            for (int round = 1; round <= 4; ++round) {
                if (!press("trigger_youyao")) return false;
                if (state().flag("ch11.lieyao") != round) return fail("Lure round did not count");
            }
            return true;
        case 13:
            return press("trigger_youyao") && travel("ch11_ningcui_jie") && press("npc_fan_xiaozi") &&
                   travel("ch11_honghu");
        case 14: choose({0}); return press("trigger_huicheng");
        case 15: return press("trigger_liandan");
        case 16: return press("trigger_biguan");
        case 17: return enter("trigger_chuguan");
        case 18:
            if (!enter("trigger_zhaolan")) return false;
            choose({0});
            return press("npc_jiedan_linju_b");
        case 19: return travel("ch11_fangshi") && press("npc_lian_er");
        case 20: return enterTeahouse() && press("npc_fan_furen") && leaveTeahouse();
        case 21: return travel("ch11_dongfu") && press("trigger_zhuzhan");
        case 22: return enter("trigger_guiju");
        case 23: return enter("trigger_wuchou");
        case 24: return enter("trigger_daoge");
        case 25: return travel("ch11_fangshi") && enterTeahouse() && press("npc_zi_ling") && leaveTeahouse();
        case 26: return peizhu() && press("trigger_peizhu") && press("trigger_peizhu");
        case 27: return travel("ch11_fangshi") && press("npc_li_dianzhu");
        case 28: return peizhu() && visitShop();
        case 29: return peizhu() && press("trigger_peizhu") && press("trigger_peizhu") && visitShop() && peizhu();
        case 30: return stepIntoSecretRoom() && enter("trigger_lianjian") && forge() && enter("trigger_chujian");
        case 31: return pressFrom("npc_jin_qing", kEndCell);
        default: return fail("No such node");
        }
    }

    // 每个节点走完之后必须置上的旗标（3.1 的 set_flag；n12 / n28 是脚本累加的整数旗标）。
    static const char* doneFlag(int node) {
        static constexpr std::array<const char*, 32> kDone{
            "", "ch11.chudao", "ch11.zhihuan", "ch11.luer", "ch11.zufu", "ch11.anju", "ch11.kuilei",
            "ch11.ziliao", "ch11.nichang", "ch11.chuansong", "ch11.ningcui", "ch11.buzhen", "ch11.lieyao",
            "ch11.jiguan", "ch11.huicheng", "ch11.sangong", "ch11.jiedan", "ch11.chuguan", "ch11.zhaolan",
            "ch11.lianer", "ch11.chaguan", "ch11.zhuzhan", "ch11.guiju", "ch11.jiyin", "ch11.yaoshi",
            "ch11.zhanglao", "ch11.jinlei", "ch11.kaidian", "ch11.jiesuan", "ch11.liugen", "ch11.lianjian",
            "ch11.done"};
        return kDone.at(static_cast<std::size_t>(node));
    }

    bool runTo(int last) {
        for (; next_ <= last; ++next_) {
            current_ = next_;
            Entry entry{next_, state(), {}};
            const std::size_t firstFight = fights_.size();
            if (!step(next_) || state().flag(doneFlag(next_)) == 0)
                return fail("Node failed: " + std::to_string(next_) + " " + doneFlag(next_));
            // 排进去的选项必须在本节点用完：剩下的会被下一处选择错拿去（测试审查 T-6）。
            if (!choices_.empty())
                return fail("Choice queue not drained at node " + std::to_string(next_));
            entry.after = state();
            checkEntry(entry, firstFight);
            ledger_.push_back(std::move(entry));
        }
        return true;
    }

    void choose(std::initializer_list<int> picks) { choices_ = std::deque<int>(picks); }
    bool spoke(const std::string& key) const { return std::find(keys_.begin(),keys_.end(),key) != keys_.end(); }
    void clearKeys() { keys_.clear(); app().clearSpokenKeys(); }
    void setCurrentNode(int node) { current_ = node; }

    // 章末：验收第 2 条「终点先验」逐项照抄（字面量，不从驱动推导）。
    void checkEnding(const Side& side) {
        SCOPED_TRACE(std::string("ch11 ending from ") + side.file);
        EXPECT_EQ(state().flag("ch11.done"), 1);
        EXPECT_EQ(rules::toValue(state().realm), 31);
        EXPECT_EQ(rules::toValue(state().realmCap), 31);
        EXPECT_EQ(rules::toValue(state().formerRealm), 22);
        EXPECT_EQ(state().maxHp, rules::realmMaxHp(rules::Realm::CoreEarly));
        EXPECT_EQ(state().maxHp, 700);
        EXPECT_EQ(state().maxMp, rules::realmMaxMp(rules::Realm::CoreEarly));
        EXPECT_EQ(state().bottle.capacity, 9);
        EXPECT_EQ(state().learnedMagics.size(), 11u);
        EXPECT_EQ(std::set<std::string>(state().learnedMagics.begin(), state().learnedMagics.end()),
                  std::set<std::string>(start_.learnedMagics.begin(), start_.learnedMagics.end()));
        EXPECT_EQ(partyIds(state()), std::vector<std::string>{"qu_hun_shadan"});
        for (const char* id : {"pill_jiangchen_dan", "material_xuelingshui", "material_yaodan_wuji",
             "material_yaodan_liuji", "material_tianleizhu", "material_jinleizhu", "material_lianjian_fuliao",
             "story_kuilei_qun", "story_yaoshou_cailiao"})
            EXPECT_EQ(state().itemCount(id), 0) << id;
        for (const char* id : {"weapon_qingzhu_fengyun_jian", "story_jinzhi_lingpai", "story_kuilei_can",
             "story_yaoshou_ziliao", "story_kuilei_yuan_qun", "story_miaoyin_yaopai", "story_bai_zhenzhu",
             "story_shijin_chong", "story_xueyu_zhizhu", "story_waihai_tu"})
            EXPECT_EQ(state().itemCount(id), 1) << id;
        const auto* field = state().findField("field_tianxing_39");
        ASSERT_NE(field, nullptr);
        EXPECT_EQ(field->slots.size(), 6u);
        const std::map<std::string, int> flags{{"ch11.lieyao", 4}, {"ch11.jiguan", 1}, {"ch11.zhu", 3},
            {"ch11.zhu_gen", 6}, {"ch11.jiesuan", 2}, {"ch11.lianjian_qian", 1}, {"ch11.zhihuan", 1},
            {"ch11.luer", 1}, {"ch11.z1", 2}, {"ch11.z2", 1}, {"ch11.dan_fu", 3}};
        for (const auto& [flag, value] : flags) EXPECT_EQ(state().flag(flag), value) << flag;
        EXPECT_EQ(state().itemCount("material_lingshi_zhong"), 20);
        // 草：给 3、买 2、胜扣 4 → 1（见 buyHerb 的注释）。
        // 低阶：−5（鲁二）−20（两株草）−50（盘店）＋ 两次回访（开店后第 1080 日 36 个月、第 4320 日 108 个月，
        // 每满 30 日 15，3.2 n28：540 ＋ 1620）= ＋2085；两侧差因此恰是起点差 20（验收第 2 条）。
        EXPECT_EQ(state().itemCount("material_lingshi"), side.low + 2085);
        EXPECT_EQ(state().itemCount("herb_nichang_cao"), 1);
        EXPECT_EQ(state().itemCount("pill_dingyan_dan"), start_.itemCount("pill_dingyan_dan") - 1);
        EXPECT_EQ(state().itemCount("material_lianqi_yao"), start_.itemCount("material_lianqi_yao") + 5);
        EXPECT_EQ(state().day - start_.day, 33375);
        EXPECT_EQ(state().mapId, "ch11_dongfu");
        EXPECT_EQ(state().position, kEndCell);
        for (const char* key : {"ch11.youyao.no_drop", "ch11.peizhu.wait", "ch11.nichang.wait", "ch11.youyao.no_herb"})
            EXPECT_FALSE(spoke(key)) << key;
        std::vector<std::string> ids;
        std::vector<BattlePhase> phases;
        for (const auto& fight : fights_) {
            ids.push_back(fight.id);
            phases.push_back(fight.phase);
        }
        EXPECT_EQ(ids, (std::vector<std::string>{kBattles[0], kBattles[0], kBattles[0], kBattles[0],
                                                 kBattles[1], kBattles[2], kBattles[3]}));
        EXPECT_EQ(phases, (std::vector<BattlePhase>{BattlePhase::Won, BattlePhase::Won, BattlePhase::Won,
                                                    BattlePhase::Won, BattlePhase::Escaped, BattlePhase::Won,
                                                    BattlePhase::Won}));
    }

    // 逐节点账：3.2 的 take / give（不足时「有几扣几」或走退路）＋ 这一节点里各场仗的实收实付。
    void checkEntry(const Entry& entry, std::size_t firstFight) {
        SCOPED_TRACE("ch11 node " + std::to_string(entry.node));
        const auto& before = entry.before;
        const auto& after = entry.after;
        if (entry.node == 29)   // 3.2 n29：补足到开店起 7200 日、至少 360
            EXPECT_EQ(after.day - after.flag("ch11.kaidian_ri"),
                      std::max(7200, before.day + 4 * 1080 - before.flag("ch11.kaidian_ri") + 360));
        else EXPECT_EQ(after.day - before.day, kNodeDays.at(static_cast<std::size_t>(entry.node)));
        std::map<std::string, int> deltas;
        const auto add = [&](const char* id, int count = 1) { deltas[id] += count; };
        const auto take = [&](const char* id, int count) { deltas[id] -= std::min(before.itemCount(id), count); };
        const auto all = [&](const char* id) { deltas[id] -= before.itemCount(id); };
        const int mid = before.itemCount("material_lingshi_zhong");
        const int low = before.itemCount("material_lingshi");
        const auto payout = [&](int days) {
            return 15 * std::min(240, (before.day + days - before.flag("ch11.jiesuan_ri")) / 30);
        };
        switch (entry.node) {
        case 2: if (mid >= 8) add("material_lingshi_zhong", -8); else if (low >= 3) add("material_lingshi", -3); break;
        case 3: take("material_lingshi", 5); break;
        case 4: take("material_lingshi_zhong", 27); add("story_jinzhi_lingpai"); break;
        case 6: add("story_kuilei_qun"); break;
        case 7:
            if (mid >= 1) add("material_lingshi_zhong", -1); else if (low >= 100) add("material_lingshi", -100);
            add("story_yaoshou_ziliao");
            break;
        case 8: add("herb_nichang_cao", 3 + kHerbsBought); add("material_lingshi", -10 * kHerbsBought); break;
        case 9: if (mid >= 2) add("material_lingshi_zhong", -2); break;
        case 10: if (mid >= 1) { add("material_lingshi_zhong", -1); add("story_waihai_tu"); } break;
        case 12: add("herb_nichang_cao", -4); break;
        case 13: add("material_yaodan_wuji", -3); add("material_lingshi_zhong", 10); add("material_lianqi_yao", 2); break;
        case 14:
            take("story_kuilei_qun", 1); add("story_kuilei_can"); add("material_yaodan_wuji", 8);
            add("story_yaoshou_cailiao");
            break;
        case 15: add("material_lingshi_zhong", 30); break;
        case 16:
            if (before.itemCount("pill_jiangchen_dan") >= 5) add("pill_jiangchen_dan", -5);
            if (before.itemCount("material_xuelingshui") >= 2) add("material_xuelingshui", -2);
            break;
        case 18: all("material_lingshi_zhong"); add("story_kuilei_yuan_qun"); add("pill_dingyan_dan", -1); add("material_lianqi_yao", 3); break;
        case 20: take("story_yaoshou_cailiao", 1); add("material_lingshi_zhong", 40); break;
        case 25: add("material_tianleizhu"); add("story_miaoyin_yaopai"); break;
        case 26:
            take("material_tianleizhu", 1); add("material_jinleizhu"); all("material_lingshi_zhong");
            all("material_yaodan_wuji"); add("material_lianjian_fuliao");
            break;
        case 27: take("material_lingshi", 50); break;
        case 28: add("material_jinleizhu"); add("material_lingshi", payout(1080)); break;
        case 29: add("material_jinleizhu", 4); add("material_lingshi", payout(3 * 1080)); add("material_lingshi_zhong", 20); break;
        case 30: add("material_jinleizhu", -6); add("material_lianjian_fuliao", -1); add("weapon_qingzhu_fengyun_jian"); break;
        case 31: add("story_bai_zhenzhu"); break;
        default: break;
        }
        for (const char* id : kAccounts) {
            int combat = 0;
            for (std::size_t i = firstFight; i < fights_.size(); ++i)
                combat += fights_[i].after.itemCount(id) - fights_[i].before.itemCount(id);
            EXPECT_EQ(after.itemCount(id) - before.itemCount(id), deltas[id] + combat) << id;
        }
        EXPECT_EQ(after.formerRealm, before.formerRealm);
        if (entry.node != 16) EXPECT_EQ(after.realm, before.realm);
        EXPECT_EQ(partyIds(after), std::vector<std::string>{"qu_hun_shadan"});
        std::cout << "[ch11 ledger] node=" << entry.node << " day=" << before.day << "->" << after.day
                  << " low=" << low << "->" << after.itemCount("material_lingshi")
                  << " mid=" << mid << "->" << after.itemCount("material_lingshi_zhong") << '\n';
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
} // namespace fanren::test::ch11
