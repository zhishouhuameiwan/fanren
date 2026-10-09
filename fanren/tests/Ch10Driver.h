#pragma once

#include <gtest/gtest.h>
#include <algorithm>
#include <deque>
#include <iostream>
#include <memory>
#include "Ch10Data.h"
#include "BattleHand.h"
#include "ChapterFixture.h"
#include "core/rules/Cultivation.h"
#include "game/AlchemyScene.h"
#include "game/CultivationScene.h"
#include "game/FieldScene.h"
#include "game/WorldScene.h"

namespace fanren::test::ch10 {
using core::GameState;
using core::MapObject;
using core::Point;
using core::battle::BattlePhase;
using game::Application;
using game::BattleScene;
using game::CultivationScene;
using game::FieldScene;
inline constexpr Point kDirections[]{{0,-1},{1,0},{0,1},{-1,0}};

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

struct Fight {
    std::string id;
    GameState before;
    GameState after;
    BattlePhase phase = BattlePhase::Ongoing;
    int rounds = 0;
    std::vector<std::string> allies;
    std::vector<std::string> log;
};

struct Entry {
    int node;
    GameState before;
    GameState after;
    std::vector<std::string> keys;
};

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
    int crafts() const { return crafts_; }
    int plants() const { return plants_; }
    int harvests() const { return harvests_; }
    int pours() const { return pours_; }
    int windowDays() const { return windowDays_; }
    int lessons = 3;
    bool loseNextFight = false;
    bool continueFourthYear = false;

    bool load(const char* file = "ch09-end-first.sav") {
        app_ = std::make_unique<Application>();
        const auto ready = app().init(root().string(), true);
        if (!ready.ok) return fail("Application init: " + ready.error);
        const auto loaded = io::loadGame(chapterFixturePath(root().string(), file).string());
        if (!loaded.ok) return fail("C9 fixture: " + loaded.error);
        start_ = loaded.value;
        if (start_.mapId != "ch10_gudao" || start_.position != Point{12,16})
            return fail("Waiting for C9 replay after qidong teleport; old endpoint " + start_.mapId);
        EXPECT_EQ(start_.flag("ch09.done"), 1);
        EXPECT_EQ(start_.realm, rules::Realm::QiRefining3);
        EXPECT_EQ(start_.realmCap, rules::Realm::QiRefining3);
        EXPECT_EQ(start_.formerRealm, rules::Realm::FoundationMid);
        EXPECT_EQ(start_.maxHp, 60);
        EXPECT_EQ(start_.maxMp, 30);
        EXPECT_EQ(start_.bottle.capacity, 6);
        EXPECT_EQ(start_.itemCount("pill_zhuji_dan"), 16);
        EXPECT_EQ(start_.itemCount("story_jin_kuloutou"), 1);
        EXPECT_EQ(start_.itemCount("pill_xuening_wuxing_dan"), 5);
        EXPECT_EQ(start_.itemCount("story_danuoyi_ling"), 1);
        EXPECT_EQ(start_.itemCount("story_zhenyuan_danfang"), 1);
        EXPECT_GE(start_.flag("ch09.xingchen"), 1);
        EXPECT_EQ(partyIds(start_), std::vector<std::string>{"kuilei_shou"});
        EXPECT_EQ(start_.learnedMagics.size(), 14u);
        EXPECT_EQ(std::set<std::string>(start_.learnedMagics.begin(), start_.learnedMagics.end()), kIncomingMagics);
        for (const char* id : {"story_qingning_jing","story_wulong_duo","story_baizhu_feidao"})
            EXPECT_EQ(start_.itemCount(id), 1) << id;
        const auto resumed = app().continueJourney(chapterFixturePath(root().string(), file).string());
        if (!resumed.ok) return fail("continueJourney: " + resumed.error);
        EXPECT_EQ(comparableSaveLines(state()), comparableSaveLines(start_));
        return true;
    }

    bool resumeReached(const GameState& checkpoint, int node) {
        const TempDir temporary("fanren_ch10_reached_scene");
        const auto file=temporary.path()/"reached.sav";
        const auto saved=io::saveGame(checkpoint,file.string());
        if (!saved.ok) return fail(saved.error);
        if (app_) app_->shutdown();
        app_=std::make_unique<Application>();
        const auto ready=app().init(root().string(),true);
        if (!ready.ok) return fail(ready.error);
        const auto loaded=io::loadGame(file.string());
        if (!loaded.ok) return fail(loaded.error);
        app().state()=loaded.value;
        const auto map=app().loadMap(checkpoint.mapId,"");
        if (!map.ok) return fail(map.error);
        // Same exact-save hydration as the formal C8/C9 drivers, after map spawn setup.
        state().position=checkpoint.position;
        state().facing=checkpoint.facing;
        const auto differences=saveLineDifferences(comparableSaveLines(checkpoint),comparableSaveLines(state()));
        EXPECT_TRUE(differences.empty()) << joinLines(differences);
        current_=node;
        next_=node;
        keys_.clear();
        return true;
    }

    std::string status() {
        std::ostringstream out;
        out << "node=" << current_ << " map=" << state().mapId << " xy=" << state().position.x
            << ',' << state().position.y << " day=" << state().day << " realm=" << rules::toValue(state().realm)
            << " hp=" << state().hp << '/' << state().maxHp << " mp=" << state().mp << '/' << state().maxMp;
        for (const char* id : kAccounts) out << ' ' << id << '=' << state().itemCount(id);
        out << " scriptError=" << app().scripts().lastError();
        return out.str();
    }

    MapObject object(const std::string& name) {
        if (const auto* map = app().currentMap())
            for (const auto& o : map->objects) if (o.name == name) return o;
        return {};
    }

    bool pump() {
        int pending = -1;
        for (int frame = 0; frame < 10000 && app().scripts().isRunning(); ++frame) {
            app().tick(1.0 / 60.0);
            auto* battle = dynamic_cast<BattleScene*>(app().topScene());
            if (pending >= 0 && !battle) {
                fights_[static_cast<std::size_t>(pending)].after = state();
                pending = -1;
            }
            if (battle) {
                if (battle->battle().phase() != BattlePhase::Ongoing) continue;
                const int index = current_ == 10 ? 0 : current_ == 28 ? 1 : current_ == 29 ? 2 : current_ == 32 ? 3 : -1;
                if (index < 0) return fail("Unexpected battle outside the four design nodes");
                Fight record;
                record.id = kBattles[static_cast<std::size_t>(index)];
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
                if (loseNextFight) {
                    for (int turn = 0; turn < 8000 && battle->battle().phase() == BattlePhase::Ongoing; ++turn) {
                        const int actor = battle->runToAllyTurn();
                        if (actor < 0) break;
                        core::battle::Action action;
                        action.kind = core::battle::ActionKind::Defend;
                        action.actorIndex = actor;
                        const auto issued = battle->issuePlayerAction(app(), action);
                        if (!issued.ok) return fail("Passive battle action refused: " + issued.error);
                    }
                    loseNextFight = false;
                } else {
                    BattleHand hand(app(), intentPolicy());
                    hand.play(*battle);
                }
                record.phase = battle->battle().phase();
                record.rounds = battle->battle().round();
                record.log = battle->battle().log();
                std::cout << "[ch10 battle] " << record.id << " phase=" << static_cast<int>(record.phase)
                          << " rounds=" << record.rounds << ' ' << status() << '\n';
                if (record.phase != BattlePhase::Won || environmentFlagSet("FANREN_CH10_BATTLE_LOG"))
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
        const auto* goalTrigger=allowEnter ? map.objectAt(goal,"trigger") : nullptr;
        const std::string completed=goalTrigger ? goalTrigger->property("set_flag") : "";
        const int wasCompleted=completed.empty() ? 0 : state().flag(completed);
        for (const Point p : route) {
            const Point at = state().position;
            if (!move({p.x-at.x,p.y-at.y})) return false;
            if (!completed.empty() && wasCompleted==0 && state().flag(completed)!=0) return true;
            if (state().mapId != mapId || state().position != p) return p == goal;
        }
        return state().position == goal;
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

    bool travel(const std::string& destination) {
        for (int hop=0; hop<8 && state().mapId != destination; ++hop) {
            const auto* link = rules::nextPortalToward(app().portalLinks(),state().mapId,destination,state());
            if (!link) return fail("No real portal to " + destination);
            const auto o = object(link->portalName);
            const std::string from = state().mapId;
            if (o.name.empty() || !walk(o.position,true) || state().mapId == from) return fail("Portal traversal failed");
        }
        return state().mapId == destination;
    }

    bool open(const std::string& name) {
        const auto o = object(name);
        if (o.name.empty() || !face(o) || !world_.interact(app())) return fail("Cannot open " + name);
        app().tick(1.0/60.0);
        return true;
    }
    void close() { app().popScene(); app().tick(1.0/60.0); }

    bool sit(int days) {
        if (!open("facility_jingshi")) return false;
        auto* panel = dynamic_cast<CultivationScene*>(app().topScene());
        if (!panel) return fail("Meditation facility did not open CultivationScene");
        const int day = state().day;
        panel->meditateFor(app(),days);
        windowDays_ += state().day-day;
        close();
        return state().day == day+days;
    }

    bool reachNine(int target = 9) {
        for (int attempt=0; attempt<200 && rules::toValue(state().realm)<target; ++attempt) {
            const int need = rules::cultivationNeeded(state().realm);
            if (state().cultivation < need && !sit(60)) return false;
            if (state().cultivation < need) continue;
            if (!open("facility_jingshi")) return false;
            auto* panel = dynamic_cast<CultivationScene*>(app().topScene());
            if (!panel) return fail("No real breakthrough panel");
            const auto rows = CultivationScene::buildMainItems(state());
            EXPECT_EQ(rows.at(1).detail.find("95%"),0u);
            EXPECT_FALSE(panel->reclimbLine(state()).empty());
            const auto result = panel->breakthrough(app());
            close();
            if (result.blocked != rules::BreakthroughBlock::None) return fail("Reclimb unexpectedly blocked");
        }
        return rules::toValue(state().realm) == target || fail("Could not reach requested realm through CultivationScene");
    }

    bool grow(const char* herb, int slot) {
        if (!open("facility_yaoyuan_xh")) return false;
        auto* field = dynamic_cast<FieldScene*>(app().topScene());
        if (!field || !field->plantAt(app(),slot,herb)) return fail("Real planting failed");
        ++plants_;
        close();
        for (int cycle=0; cycle<150; ++cycle) {
            const auto* planted = state().findField("field_xiaohuan");
            if (!planted || planted->slots.at(static_cast<std::size_t>(slot)).seedId != herb) return fail("Planted herb disappeared");
            if (planted->slots.at(static_cast<std::size_t>(slot)).age >= 60) break;
            if (!sit(5) || !open("facility_yaoyuan_xh")) return false;
            field = dynamic_cast<FieldScene*>(app().topScene());
            if (!field) return fail("Missing field scene while watering");
            if (state().findField("field_xiaohuan")->slots.at(static_cast<std::size_t>(slot)).ripe && state().bottle.drops > 0) {
                if (!field->matureAt(app(),slot)) {
                    const auto& plant=state().findField("field_xiaohuan")->slots.at(static_cast<std::size_t>(slot));
                    std::ostringstream reason;
                    reason << "FieldScene::matureAt refused: " << field->feedback() << " field=" << field->fieldId()
                           << " slot=" << slot << " herb=" << plant.seedId << " age=" << plant.age
                           << " plantedDay=" << plant.plantedDay << " ripe=" << plant.ripe << " maxAge=" << plant.maxAge
                           << " drops=" << state().bottle.drops << " lastCharge=" << state().bottle.lastChargeDay;
                    return fail(reason.str());
                }
                ++pours_;
            }
            close();
        }
        if (!open("facility_yaoyuan_xh")) return false;
        field = dynamic_cast<FieldScene*>(app().topScene());
        const int count = state().itemCountAtLeastAge(herb,60);
        if (!field || !field->harvestAt(app(),slot)) return fail("Real harvesting failed");
        ++harvests_;
        close();
        return state().itemCountAtLeastAge(herb,60) == count+1;
    }

    bool preparePills() {
        for (int batch=0; batch<20 && state().itemCount("pill_zhenyuan_dan")<2; ++batch) {
            for (const char* herb : {"herb_zishen_cao","herb_xuehong_zhi"}) {
                if (state().itemCountAtLeastAge(herb,60)>0) continue;
                if (state().itemCount(herb)==0) {
                    if (!press("trigger_sanzhuan") || state().flag("ch10.chuguan") != 0)
                        return fail("Failed to obtain seeds from the real shortage branch");
                }
                if (!grow(herb,herb == std::string("herb_zishen_cao") ? 0 : 1)) return false;
            }
            if (!open("facility_danfang_xh")) return false;
            auto* furnace = dynamic_cast<game::AlchemyScene*>(app().topScene());
            if (!furnace) return fail("Alchemy facility did not open AlchemyScene");
            const auto& ids = furnace->recipeIds();
            const auto found = std::find(ids.begin(),ids.end(),"recipe_zhenyuan_dan");
            if (found==ids.end() || !furnace->craftAt(app(),static_cast<int>(found-ids.begin())))
                return fail("Real furnace refused the recipe: " + furnace->feedback());
            ++crafts_;
            furnace->leave(app());
            app().tick(1.0/60.0);
        }
        return state().itemCount("pill_zhenyuan_dan")==2 || fail("Did not craft exactly two pills");
    }

    bool formation(bool sea, int count = 4) {
        const char* prefix = sea ? "trigger_bishui_" : "trigger_zhenwei_";
        const char* mask = sea ? "ch10.bishui" : "ch10.zhenqi";
        int index = 0;
        for (const char* direction : {"dong","nan","xi","bei"}) {
            if (index++ >= count) break;
            const int before = state().flag(mask);
            if (!press(std::string(prefix)+direction)) return false;
            EXPECT_EQ(state().flag(mask),before | (1 << (index-1)));
            if (!press(std::string(prefix)+direction)) return false;
            EXPECT_EQ(state().flag(mask),before | (1 << (index-1)));
        }
        return true;
    }

    void choose(std::initializer_list<int> picks) { choices_ = std::deque<int>(picks); }
    bool spoke(const std::string& key) const { return std::find(keys_.begin(),keys_.end(),key)!=keys_.end(); }
    void clearKeys() { keys_.clear(); app().clearSpokenKeys(); }

    bool runTo(int last) {
        for (; next_<=last; ++next_) {
            current_ = next_;
            const auto& hook = kStory.at(static_cast<std::size_t>(next_-1));
            if (next_==6) for (int lesson=0; lesson<lessons; ++lesson)
                if (!press("npc_wang_changqing")) return false;
            if (next_==15 && !formation(false)) return false;
            if (next_==17 && !reachNine()) return false;
            if (next_==19 && !preparePills()) return false;
            if (next_==24) {
                if (!travel("ch10_kuixing")) return false;
                const int low = state().itemCount("material_lingshi");
                if (!press("trigger_gujia")) return false;
                EXPECT_EQ(state().itemCount("material_lingshi"),low+120);
                EXPECT_EQ(state().flag("ch10.gujia"),1);
                if (!travel("ch10_tiandujie")) return false;
            }
            if (state().mapId != hook.map) return fail("Unexpected map before " + std::string(hook.object));
            choose({0});
            if (next_==12) choose({0,1,2});
            if (next_==27) {
                choose({0});
                if (!press(hook.object)) return false;
                EXPECT_EQ(state().flag(hook.done),0);
                choose({1});
            }
            if (next_==30 && continueFourthYear) choose({1});
            Entry entry{next_,state(),{}, {}};
            const std::size_t firstKey = keys_.size();
            const std::size_t firstFight = fights_.size();
            const bool fired = hook.enter ? enter(hook.object) : press(hook.object);
            if (!fired || state().flag(hook.done)==0) return fail("Node failed: " + std::string(hook.object));
            entry.after = state();
            entry.keys.assign(keys_.begin()+static_cast<std::ptrdiff_t>(firstKey),keys_.end());
            checkEntry(entry,firstFight);
            ledger_.push_back(std::move(entry));
            if (next_==27) {
                if (!formation(true)) return false;
                const int day = state().day;
                if (!press("trigger_bishui_yan")) return false;
                EXPECT_EQ(state().flag("ch10.bishui_cheng"),1);
                EXPECT_EQ(state().day,day+1);
            }
        }
        return true;
    }

    void checkEnding() {
        EXPECT_EQ(state().flag("ch10.done"),1);
        EXPECT_EQ(state().realm,rules::Realm::FoundationLate);
        EXPECT_EQ(state().realmCap,rules::Realm::FoundationLate);
        EXPECT_EQ(state().formerRealm,rules::Realm::FoundationMid);
        EXPECT_EQ(state().maxHp,rules::realmMaxHp(state().realm));
        EXPECT_EQ(state().maxMp,rules::realmMaxMp(state().realm));
        EXPECT_EQ(state().bottle.capacity,6);
        EXPECT_EQ(partyIds(state()),std::vector<std::string>{"qu_hun_shadan"});
        auto expected = kIncomingMagics;
        for (const char* id : kLostMagics) expected.erase(id);
        EXPECT_EQ(std::set<std::string>(state().learnedMagics.begin(),state().learnedMagics.end()),expected);
        EXPECT_EQ(state().learnedMagics.size(),11u);
        EXPECT_EQ(state().itemCount("pill_zhuji_dan"),13);
        EXPECT_EQ(state().itemCount("pill_dingyan_dan"),start_.itemCount("pill_dingyan_dan"));
        EXPECT_EQ(state().itemCount("pill_jiangchen_dan"),5);
        EXPECT_EQ(state().itemCount("material_xuelingshui"),2);
        for (const char* id : {"pill_zhenyuan_dan","material_tianhuoye","pill_xuening_wuxing_dan",
             "story_jin_kuloutou","story_hunyuan_bo","talisman_jinjian_fubao","story_qingning_jing",
             "story_wulong_duo","story_baizhu_feidao","story_lvse_yupai"}) EXPECT_EQ(state().itemCount(id),0) << id;
        for (const char* id : {"talisman_lanjiao_fu","story_tulijue","story_shijin_chong","story_lanse_yupei",
             "story_xiaohuan_yujian","story_haiyu_tu","story_luanxing_danfang","story_dandao_pingjian"})
            EXPECT_EQ(state().itemCount(id),1) << id;
        EXPECT_EQ(state().itemCount("material_yaodan_wuji"),start_.itemCount("material_yaodan_wuji")+1);
        int combatLow=0,combatMid=0;
        for (const auto& fight : fights_) {
            combatLow += fight.before.itemCount("material_lingshi")-fight.after.itemCount("material_lingshi");
            combatMid += fight.before.itemCount("material_lingshi_zhong")-fight.after.itemCount("material_lingshi_zhong");
        }
        EXPECT_EQ(state().itemCount("material_lingshi"),start_.itemCount("material_lingshi")+34-combatLow);
        EXPECT_EQ(state().itemCount("material_lingshi_zhong"),start_.itemCount("material_lingshi_zhong")+75-combatMid);
        EXPECT_EQ(state().day-start_.day,9212);
        EXPECT_EQ(state().flag("ch10.yuyan"),2);
        EXPECT_EQ(state().flag("ch10.shizi"),1);
        EXPECT_EQ(state().mapId,"ch10_xiaohuan");
        EXPECT_EQ(state().position,(Point{50,39}));
        const auto* field = state().findField("field_xiaohuan");
        EXPECT_NE(field,nullptr);
        if (field) EXPECT_EQ(field->slots.size(),4u);
        EXPECT_GT(crafts_,0);
        EXPECT_GT(plants_,0);
        EXPECT_GT(pours_,0);
        EXPECT_GT(harvests_,0);
        std::vector<std::string> ids;
        for (const auto& fight : fights_) ids.push_back(fight.id);
        EXPECT_EQ(ids,std::vector<std::string>(kBattles.begin(),kBattles.end()));
    }

    void setCurrentNode(int node) { current_ = node; }

private:
    bool fail(const std::string& why) { problem_ = why; if (app_) problem_ += "\n" + status(); return false; }
    void captureKeys() {
        keys_.insert(keys_.end(),app().spokenKeys().begin(),app().spokenKeys().end());
        app().clearSpokenKeys();
    }
    void checkEntry(const Entry& entry, std::size_t firstFight) {
        SCOPED_TRACE("ch10 node " + std::to_string(entry.node));
        const auto& before=entry.before;
        const auto& after=entry.after;
        int days=kStory.at(static_cast<std::size_t>(entry.node-1)).days;
        if (entry.node==6) days=3-lessons;
        if (entry.node==19) days=std::max(360,7560-(before.day-before.flag("ch10.kaifu_ri")));
        EXPECT_EQ(after.day-before.day,days);
        std::map<std::string,int> deltas;
        const auto add=[&](const char* id,int count=1) { deltas[id]+=count; };
        const auto take=[&](const char* id,int count) { deltas[id]-=std::min(before.itemCount(id),count); };
        switch(entry.node) {
        case 7: add("story_lvse_yupai"); break;
        case 12: take("story_lvse_yupai",1); add("story_lanse_yupei"); add("story_xiaohuan_yujian"); add("story_haiyu_tu"); take("material_lingshi",5); break;
        case 13: take("material_lingshi",6); break;
        case 16: add("herb_zishen_cao",2); add("herb_xuehong_zhi",2); break;
        case 17: take("pill_zhuji_dan",3); break;
        case 19: take("pill_zhenyuan_dan",2); take("material_lingshi",120); break;
        case 22: add("material_xuelingshui",2); add("material_tianhuoye",2); break;
        case 23: take("material_lingshi",35); add("story_luanxing_danfang"); add("story_dandao_pingjian"); break;
        case 29:
            for (const char* id : {"story_qingning_jing","story_wulong_duo","story_baizhu_feidao"}) take(id,1);
            add("story_hunyuan_bo"); add("material_lingshi_zhong",75); add("pill_jiangchen_dan",5);
            for (const char* id : {"talisman_jinjian_fubao","talisman_lanjiao_fu","story_tulijue","material_yaodan_wuji"}) add(id);
            break;
        case 30:
            take("material_tianhuoye",2); take("pill_xuening_wuxing_dan",5);
            for (const char* id : {"story_hunyuan_bo","talisman_jinjian_fubao","story_jin_kuloutou"}) take(id,1);
            break;
        case 31: add("story_shijin_chong"); break;
        case 32: add("material_lingshi",80); break;
        default: break;
        }
        for (const char* id : kAccounts) {
            int combat=0;
            for (std::size_t i=firstFight;i<fights_.size();++i)
                combat+=fights_[i].after.itemCount(id)-fights_[i].before.itemCount(id);
            EXPECT_EQ(after.itemCount(id)-before.itemCount(id),deltas[id]+combat) << id;
        }
        EXPECT_EQ(after.formerRealm,before.formerRealm);
        if (entry.node!=8 && entry.node!=17 && entry.node!=19) EXPECT_EQ(after.realm,before.realm);
        if (entry.node==10) EXPECT_EQ(partyIds(after),std::vector<std::string>{"kuilei_shou"});
        if (entry.node==19) EXPECT_EQ(partyIds(after),(std::vector<std::string>{"kuilei_shou","qu_hun_huashen"}));
        if (entry.node==29) {
            EXPECT_EQ(partyIds(after),std::vector<std::string>{"qu_hun_huashen"});
            for (const char* id : kLostMagics) EXPECT_FALSE(after.knowsMagic(id));
        }
        if (entry.node==30) EXPECT_EQ(partyIds(after),std::vector<std::string>{"qu_hun_shadan"});
        std::cout << "[ch10 ledger] node=" << entry.node << " day=" << before.day << "->" << after.day
                  << " low=" << before.itemCount("material_lingshi") << "->" << after.itemCount("material_lingshi")
                  << " mid=" << before.itemCount("material_lingshi_zhong") << "->" << after.itemCount("material_lingshi_zhong") << '\n';
    }

    std::unique_ptr<Application> app_;
    game::WorldScene world_;
    GameState start_;
    std::vector<Fight> fights_;
    std::vector<Entry> ledger_;
    std::vector<std::string> keys_;
    std::deque<int> choices_;
    std::string problem_;
    int next_=1,current_=1,crafts_=0,plants_=0,harvests_=0,pours_=0,windowDays_=0;
};
} // namespace fanren::test::ch10
