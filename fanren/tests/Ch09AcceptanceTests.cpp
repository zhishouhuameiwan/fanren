#include <gtest/gtest.h>
#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "../vendor/json.hpp"
#include "BattleHand.h"
#include "ChapterFixture.h"
#include "core/rules/Objectives.h"
#include "game/WorldScene.h"

namespace {
using namespace fanren;
using core::GameState;
using core::MapObject;
using core::Point;
using core::battle::BattlePhase;
using game::Application;
using game::BattleScene;
using game::WorldScene;
constexpr Point kDirections[] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
constexpr const char* kBattles[] = {"b09_tuwei", "b09_fujia", "b09_dong_xuaner"};

test::HandPolicy intentPolicy() {
    test::HandPolicy p;
    p.healAtPercent = 50;
    p.healWhenDoomed = false;
    p.fleeWhenSpent = true;
    p.pills = {"pill_yangjing_dan", "pill_jinchuang_yao"};
    return p;
}

class Ch09Walkthrough : public ::testing::Test {
protected:
    void SetUp() override { start(test::kChapterEightEndingFirst); }
    void TearDown() override { app_->shutdown(); }
    Application& app() { return *app_; }
    GameState& state() { return app().state(); }

    void start(const char* file) {
        if (app_) app_->shutdown();
        app_ = std::make_unique<Application>();
        const auto ready = app().init(root_, true);
        ASSERT_TRUE(ready.ok) << ready.error;
        const auto loaded = io::loadGame(test::chapterFixturePath(root_, file).string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        initial_ = loaded.value;
        ASSERT_EQ(initial_.flag("ch08.done"), 1);
        ASSERT_EQ(initial_.realm, rules::Realm::FoundationMid);
        ASSERT_EQ(initial_.realmCap, rules::Realm::FoundationMid);
        ASSERT_EQ(initial_.formerRealm, rules::Realm::Mortal);
        ASSERT_EQ(initial_.maxHp, 380);
        ASSERT_EQ(initial_.maxMp, 250);
        ASSERT_EQ(initial_.bottle.capacity, 6);
        ASSERT_EQ(initial_.itemCount("pill_zhuji_dan"), 16);
        ASSERT_EQ(initial_.learnedMagics.size(), 12u);
        ASSERT_EQ(initial_.party.size(), 1u);
        ASSERT_EQ(initial_.party.front().roleId, "kuilei_shou");
        ASSERT_TRUE(initial_.party.front().active);
        ASSERT_EQ(initial_.itemCount("story_danuoyi_ling"), 1);
        ASSERT_EQ(initial_.itemCount("story_diandao_zhenqi_gai"), 1);
        for (const auto& s : test::kChapterNineSteps) ASSERT_EQ(initial_.flag(s.done), 0) << s.done;
        state() = initial_;
        ASSERT_TRUE(app().loadMap(state().mapId, "").ok);
        state().position = initial_.position;
        state().facing = initial_.facing;
        ASSERT_EQ(test::comparableSaveLines(state()), test::comparableSaveLines(initial_));
        fights_.clear();
        demotions_ = 0;
        spentStones_ = 0;
        choiceNumber_ = 0;
        current_ = 0;
    }

    std::string diagnostic() {
        std::ostringstream out;
        out << "side=" << side_ << " step=" << current_ + 1 << " map=" << state().mapId
            << " xy=" << state().position.x << ',' << state().position.y << " day=" << state().day
            << " realm=" << rules::toValue(state().realm) << '/' << rules::toValue(state().realmCap)
            << " former=" << rules::toValue(state().formerRealm) << " hp=" << state().hp << '/' << state().maxHp
            << " mp=" << state().mp << '/' << state().maxMp;
        for (const char* item : {"material_lingshi", "material_lingshi_zhong", "pill_yangjing_dan",
             "pill_jinchuang_yao", "pill_dingyan_dan", "material_heyuanyu"}) out << ' ' << item << '=' << state().itemCount(item);
        out << " scriptError=" << app().scripts().lastError();
        const auto* next = rules::currentObjective(app().data().objectives, state());
        if (next) out << " next=" << next->id << '/' << next->targetObject;
        return out.str();
    }

    void roundTrip(const GameState& s, const char* label) {
        const test::TempDir temp("fanren_ch09_roundtrip");
        const auto file = temp.path() / "state.sav";
        ASSERT_TRUE(io::saveGame(s, file.string()).ok) << label;
        std::ifstream in(file);
        nlohmann::json json;
        in >> json;
        ASSERT_EQ(json.at("save_version").get<int>(), 9) << label;
        const auto read = io::loadGame(file.string());
        ASSERT_TRUE(read.ok) << read.error;
        EXPECT_EQ(test::comparableSaveLines(read.value), test::comparableSaveLines(s)) << label;
    }
    void observeDemotion(const GameState& before) {
        if (before.realm == state().realm) return;
        ++demotions_;
        EXPECT_EQ(current_, 18u);
        EXPECT_EQ(before.realm, rules::Realm::FoundationMid);
        GameState expected = before;
        expected.realm = expected.realmCap = rules::Realm::QiRefining3;
        expected.formerRealm = rules::fromValue(std::max(rules::toValue(before.formerRealm), rules::toValue(before.realm)));
        expected.maxHp = rules::realmMaxHp(expected.realm);
        expected.hp = std::min(before.hp, expected.maxHp);
        expected.maxMp = rules::realmMaxMp(expected.realm);
        expected.mp = expected.cultivation = 0;
        expected.cultivationRemainder = 0;
        EXPECT_EQ(test::comparableSaveLines(state()), test::comparableSaveLines(expected))
            << "E1: actual RealmDemote transition must preserve every other saved field";
        roundTrip(before, "before actual demote");
        roundTrip(state(), "after actual demote");
        std::cout << "[ch09 demote] " << diagnostic() << '\n';
    }

    void dumpFight(const char* label, const std::string& id, BattleScene& fight, BattlePhase phase, const GameState& before) {
        std::cout << "[ch09 " << label << "] side=" << side_ << " id=" << id << " phase=" << static_cast<int>(phase)
                  << " round=" << fight.battle().round() << " policy=heal50,noDoomed,pills:yangjing-jinchuang"
                  << " before hp=" << before.hp << '/' << before.maxHp << " mp=" << before.mp << '/' << before.maxMp
                  << " low=" << before.itemCount("material_lingshi") << " mid=" << before.itemCount("material_lingshi_zhong")
                  << " pills=" << before.itemCount("pill_yangjing_dan") << " salves=" << before.itemCount("pill_jinchuang_yao") << '\n';
        for (const auto& u : fight.battle().units())
            std::cout << "  " << u.id << " hp=" << u.hp << '/' << u.maxHp << " mp=" << u.mp << " toughness=" << u.toughness << '\n';
        if (phase != BattlePhase::Won || test::environmentFlagSet("FANREN_CH09_BATTLE_LOG"))
            for (const auto& line : fight.battle().log()) std::cout << "  " << line << '\n';
    }

    bool pump() {
        for (int frame = 0; frame < 8000 && app().scripts().isRunning(); ++frame) {
            const GameState beforeTick = state();
            app().tick(1.0 / 60.0);
            observeDemotion(beforeTick);
            if (auto* fight = dynamic_cast<BattleScene*>(app().topScene())) {
                if (fight->battle().phase() != BattlePhase::Ongoing) continue;
                if (fights_.size() >= std::size(kBattles)) return false;
                const std::string id = kBattles[fights_.size()];
                EXPECT_LT(current_, 18u) << "no mandatory combat after demotion";
                EXPECT_EQ(state().realm, rules::Realm::FoundationMid);
                EXPECT_TRUE(state().partyHas("kuilei_shou"));
                EXPECT_FALSE(state().partyHas("qu_hun"));
                const auto* setup = app().battleSetup(id);
                if (!setup) return false;
                std::multiset<std::string> actual, expected;
                for (const auto& u : fight->battle().units()) if (!u.ally) actual.insert(u.id);
                for (const auto& u : setup->units) if (!u.ally) expected.insert(u.roleId);
                EXPECT_EQ(actual, expected) << id;
                const GameState before = state();
                test::BattleHand hand(app(), intentPolicy());
                const auto phase = hand.play(*fight);
                dumpFight("battle", id, *fight, phase, before);
                EXPECT_EQ(phase, BattlePhase::Won) << diagnostic();
                if (id == "b09_fujia") EXPECT_LE(fight->battle().round(), 2) << "design 8.2: a short ambush";
                spentStones_ += before.itemCount("material_lingshi_zhong") - state().itemCount("material_lingshi_zhong");
                fights_.push_back(id);
                if (phase != BattlePhase::Won) return false;
                if (id == "b09_dong_xuaner") {
                    Application isolated;
                    const auto ready = isolated.init(root_, true);
                    EXPECT_TRUE(ready.ok) << ready.error;
                    if (!ready.ok) return false;
                    isolated.state() = before;
                    for (const auto& [item, magic] : std::vector<std::pair<const char*, const char*>>{
                         {"story_hongxian_zhen", "magic_ji_hongxianzhen"}, {"story_baizhu_feidao", "magic_ji_baizhudao"}}) {
                        EXPECT_EQ(before.itemCount(item), 1);
                        EXPECT_TRUE(before.knowsMagic(magic));
                        EXPECT_TRUE(isolated.state().removeItem(item, isolated.state().itemCount(item)));
                        EXPECT_TRUE(isolated.state().forgetMagic(magic));
                    }
                    const GameState bareBefore = isolated.state();
                    BattleScene noNew(id);
                    noNew.onEnter(isolated);
                    test::BattleHand other(isolated, intentPolicy());
                    const auto without = other.play(noNew);
                    dumpFight("no-new-artifacts", id, noNew, without, bareBefore);
                    EXPECT_EQ(without, BattlePhase::Won) << "same real pre-battle state, only two new artifacts removed";
                    isolated.shutdown();
                }
                continue;
            }
            if (!app().awaitingCommand()) continue;
            script::CommandResult answer;
            answer.ok = true;
            answer.choiceIndex = 0;
            if (app().spokenKeys().empty()) {
                if (current_ == 2) answer.choiceIndex = 1; // Ripe herbs may be left behind, never conjured or silently harvested.
                if (current_ == 12) answer.choiceIndex = std::min(choiceNumber_++, 4);
            }
            app().clearSpokenKeys();
            app().completeCommand(answer);
            app().popScene();
        }
        app().tick(1.0 / 60.0);
        return !app().scripts().isRunning() && !app().quitRequested();
    }

    MapObject object(const std::string& name) {
        for (const auto& o : app().currentMap()->objects) if (o.name == name) return o;
        return {};
    }
    bool passable(Point p, bool goal = false) {
        const auto& map = *app().currentMap();
        if (!map.walkable(p) || WorldScene::visibleNpcAt(state(), map, p) || map.objectAt(p, "facility")) return false;
        if (!goal && map.objectAt(p, "portal")) return false;
        const auto* t = map.objectAt(p, "trigger");
        return goal || !t || t->property("mode") != "enter" || !WorldScene::triggerReady(state(), *t);
    }
    std::vector<Point> pathTo(Point goal) {
        const auto& map = *app().currentMap();
        const int width = map.width;
        const auto key = [width](Point p) { return p.y * width + p.x; };
        const Point from = state().position;
        std::map<int, Point> previous{{key(from), from}};
        std::deque<Point> queue{from};
        while (!queue.empty() && !previous.count(key(goal))) {
            const Point p = queue.front(); queue.pop_front();
            for (const auto d : kDirections) {
                const Point n{p.x + d.x, p.y + d.y};
                if (n.x < 0 || n.y < 0 || n.x >= map.width || n.y >= map.height || previous.count(key(n))) continue;
                if (!passable(n, n == goal)) continue;
                previous[key(n)] = p;
                queue.push_back(n);
            }
        }
        if (!previous.count(key(goal))) return {};
        std::vector<Point> path;
        for (Point p = goal; p != from; p = previous.at(key(p))) path.push_back(p);
        std::reverse(path.begin(), path.end());
        return path;
    }
    bool move(Point d) {
        const bool moved = world_.tryStep(app(), d.x, d.y);
        return pump() && moved;
    }
    bool walk(Point goal) {
        if (state().position == goal) return true;
        const auto path = pathTo(goal);
        if (path.empty()) return false;
        const auto mapId = state().mapId;
        for (Point p : path) {
            const Point at = state().position;
            if (!move({p.x - at.x, p.y - at.y})) return false;
            if (state().mapId != mapId || state().position != p) return p == goal;
        }
        return state().position == goal;
    }
    bool face(const MapObject& o) {
        for (const auto d : kDirections) {
            const Point stand{o.position.x - d.x, o.position.y - d.y};
            if (!passable(stand)) continue;
            if (!passable(o.position, true)) {
                if (!walk(stand)) continue;
                world_.tryStep(app(), d.x, d.y);
                return pump();
            }
            const Point approach{stand.x - d.x, stand.y - d.y};
            if (passable(approach) && walk(approach) && move(d) && state().position == stand) return true;
        }
        return false;
    }
    bool fire(const test::ChapterNineStep& step) {
        const auto o = object(step.object);
        if (o.name.empty()) return false;
        if (!step.enter) return face(o) && world_.interact(app()) && pump();
        if (state().position != o.position) return walk(o.position);
        for (const auto d : kDirections) {
            const Point beside{o.position.x - d.x, o.position.y - d.y};
            if (passable(beside) && walk(beside)) return move(d);
        }
        return false;
    }
    bool formation() {
        for (const char* suffix : {"jin", "mu", "shui", "huo"}) {
            const std::string name = std::string("trigger_zhenwei_jiu_") + suffix;
            const test::ChapterNineStep step{"ch08_lingkuang", name.c_str(), "", 0, ""};
            if (!fire(step)) return false;
        }
        return state().flag("ch09.zhenqi") == 15;
    }
    bool tryCaveExit() {
        const auto wall = object("trigger_suidao_hui");
        EXPECT_FALSE(wall.name.empty());
        if (wall.name.empty()) return false;
        for (int attempt = 0; attempt < 2; ++attempt) {
            const GameState before = state();
            if (!walk(wall.position)) return false;
            EXPECT_EQ(state().mapId, "ch08_lingkuang");
            EXPECT_EQ(state().position, (Point{39, 34})); // Design 18: the reusable cave retreat landing.
            GameState expected = before;
            expected.position = {39, 34};
            expected.facing = state().facing;
            EXPECT_EQ(test::comparableSaveLines(state()), test::comparableSaveLines(expected));
            EXPECT_TRUE(WorldScene::triggerReady(state(), wall));
            std::cout << "[ch09 cave-exit] attempt=" << attempt + 1 << ' ' << diagnostic() << '\n';
        }
        return !HasFailure();
    }

    bool play(int side, const char* fixture) {
        side_ = side;
        if (side == 1) start(test::kChapterEightEndingSecond);
        for (current_ = 0; current_ < test::kChapterNineSteps.size(); ++current_) {
            const auto& s = test::kChapterNineSteps[current_];
            SCOPED_TRACE(diagnostic());
            EXPECT_EQ(state().mapId, s.map) << s.object;
            if (state().mapId != s.map) return false;
            if (current_ == 14 && !formation()) return false;
            const int day = state().day;
            const int low = state().itemCount("material_lingshi");
            const int mid = state().itemCount("material_lingshi_zhong");
            const int pills = state().itemCount("pill_dingyan_dan");
            const bool fired = fire(s);
            if (!fired && state().flag(s.done) == 0) return false;
            if (state().flag(s.done) == 0) return false;
            EXPECT_EQ(state().mapId, s.destination);
            EXPECT_EQ(state().day - day, s.days) << s.object;
            EXPECT_EQ(state().itemCount("pill_zhuji_dan"), 16);
            EXPECT_TRUE(state().partyHas("kuilei_shou"));
            EXPECT_FALSE(state().partyHas("qu_hun"));
            if (current_ == 6) {
                const auto portal = object("portal_to_dongfu");
                EXPECT_FALSE(portal.name.empty());
                if (portal.name.empty()) return false;
                EXPECT_TRUE(pathTo(portal.position).empty()) << "a real NPC wall must close the return to the destroyed cave";
            }
            if (current_ == 13 && !tryCaveExit()) return false;
            if (current_ < 18) EXPECT_EQ(state().realm, rules::Realm::FoundationMid);
            if (current_ == 5) EXPECT_EQ(state().itemCount("material_lingshi") - low, 60);
            if (current_ == 8) {
                EXPECT_EQ(state().itemCount("material_lingshi"), low - 300);
                EXPECT_EQ(state().itemCount("pill_dingyan_dan"), pills - 2);
            }
            if (current_ == 13) EXPECT_EQ(state().itemCount("material_lingshi"), low - 60);
            if (current_ == 14) EXPECT_EQ(state().itemCount("material_lingshi_zhong"), mid - std::min(mid, 2));
            if (current_ == 18) EXPECT_EQ(state().itemCount("material_lingshi_zhong"), mid + 30);
            if (current_ == 21) {
                EXPECT_EQ(state().itemCount("material_lingshi"), low - 6);
                EXPECT_EQ(state().itemCount("material_lingshi_zhong"), mid - 6);
            }
        }
        EXPECT_EQ(fights_, (std::vector<std::string>(std::begin(kBattles), std::end(kBattles))));
        EXPECT_EQ(demotions_, 1);
        EXPECT_EQ(choiceNumber_, 5);
        EXPECT_EQ(state().realm, rules::Realm::QiRefining3);
        EXPECT_EQ(state().realmCap, rules::Realm::QiRefining3);
        EXPECT_EQ(state().formerRealm, rules::Realm::FoundationMid);
        EXPECT_EQ(state().maxHp, rules::realmMaxHp(rules::Realm::QiRefining3));
        EXPECT_EQ(state().maxMp, rules::realmMaxMp(rules::Realm::QiRefining3));
        EXPECT_EQ(state().cultivation, 0);
        EXPECT_EQ(state().cultivationRemainder, 0);
        EXPECT_EQ(state().bottle.capacity, 6);
        auto magics = std::set<std::string>(initial_.learnedMagics.begin(), initial_.learnedMagics.end());
        magics.insert("magic_ji_hongxianzhen"); magics.insert("magic_ji_baizhudao");
        EXPECT_EQ(std::set<std::string>(state().learnedMagics.begin(), state().learnedMagics.end()), magics);
        EXPECT_EQ(state().learnedMagics.size(), 14u);
        for (const char* item : {"story_hongxian_zhen", "story_baizhu_feidao", "story_zhenyuan_danfang",
             "story_tongqian", "story_xiufu_yujian", "story_xin_zhenqi", "story_danuoyi_ling"})
            EXPECT_EQ(state().itemCount(item), 1) << item;
        for (const char* item : {"story_diandao_zhenqi_gai", "story_yinhui_jian", "material_heyuanyu", "material_xiuzhen_liao"})
            EXPECT_EQ(state().itemCount(item), 0) << item;
        EXPECT_EQ(state().itemCount("material_lingshi"), initial_.itemCount("material_lingshi") - 306);
        EXPECT_EQ(state().itemCount("pill_dingyan_dan"), initial_.itemCount("pill_dingyan_dan") - 2);
        EXPECT_GE(state().itemCount("material_lingshi_zhong"), 22);
        EXPECT_EQ(state().itemCount("material_lingshi_zhong"), initial_.itemCount("material_lingshi_zhong") + 22 - spentStones_);
        const int storyDays = std::accumulate(test::kChapterNineSteps.begin(), test::kChapterNineSteps.end(), 0,
            [](int sum, const auto& step) { return sum + step.days; });
        EXPECT_EQ(storyDays, 59);
        EXPECT_EQ(state().day, initial_.day + storyDays);
        EXPECT_EQ(state().mapId, "ch10_gudao");
        EXPECT_EQ(state().position, (Point{12, 16})); // Chapter 10 contract: qidong's final, real teleport.
        EXPECT_EQ(state().flag("ch09.done"), 1);
        if (HasFailure()) return false;
        roundTrip(state(), "real chapter 9 ending");
        const test::TempDir temp("fanren_ch09_continue");
        const auto probe = temp.path() / "ending.sav";
        EXPECT_TRUE(io::saveGame(state(), probe.string()).ok);
        Application continued;
        const auto ready = continued.init(root_, true);
        EXPECT_TRUE(ready.ok) << ready.error;
        if (!ready.ok) return false;
        const auto loaded = continued.continueJourney(probe.string());
        EXPECT_TRUE(loaded.ok) << loaded.error;
        if (loaded.ok) EXPECT_EQ(test::comparableSaveLines(continued.state()), test::comparableSaveLines(state()));
        continued.shutdown();
        if (HasFailure()) return false;
        const auto verdict = test::settleAgainstFixture(state(), root_, fixture, test::kWriteChapterNineFixturesEnv, "Chapter 10");
        EXPECT_TRUE(verdict.problem.empty()) << verdict.problem;
        std::cout << "[ch09 ending] wrote=" << verdict.wrote << " file=" << fixture << ' ' << diagnostic() << '\n';
        return !HasFailure();
    }

    std::string root_ = test::chapterNineAssetRoot();
    std::unique_ptr<Application> app_;
    WorldScene world_;
    GameState initial_;
    std::vector<std::string> fights_;
    std::size_t current_ = 0;
    int side_ = 0, demotions_ = 0, spentStones_ = 0, choiceNumber_ = 0;
};

TEST_F(Ch09Walkthrough, FirstSidePlaysAllThreeFightsThenDemotesAndHandsOverTheRealEnding) {
    EXPECT_TRUE(play(0, test::kChapterNineEndingFirst)) << diagnostic();
}
TEST_F(Ch09Walkthrough, SecondSideUsesTheSameOriginalChoicesAndHandsOverItsRealEnding) {
    EXPECT_TRUE(play(1, test::kChapterNineEndingSecond)) << diagnostic();
}
}  // namespace
