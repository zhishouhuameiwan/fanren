#include <gtest/gtest.h>

#include <algorithm>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <SDL3/SDL.h>

#include "BattleHand.h"
#include "ChapterFixture.h"
#include "core/rules/Objectives.h"
#include "game/AlchemyScene.h"
#include "game/CultivationScene.h"
#include "game/FieldScene.h"
#include "game/ShopScene.h"
#include "game/SpriteAtlas.h"
#include "game/WorldScene.h"
#include "io/VisualLoader.h"

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
constexpr const char* kBattles[] = {"b08_guiling_shaozhu", "b08_lingkuang_shouzhen", "b08_zhongrudong",
                                  "b08_jinguyuan_juezhan", "b08_yuehuang", "b08_quhun"};
struct ReviewKey {
    const char* key;
    std::size_t step;
    std::size_t completedBattles;
};
// Controller F1-F7 revision: scope follows the scene prefix, not the text filename.
constexpr ReviewKey kReviewKeys[] = {
    {"ch08.tianxing_ru.height", 5, 0},
    {"ch08.qiyunxiao.f1", 8, 0},
    {"ch08.zhulinxin.f1", 47, 4},
    {"ch08.luanshi.aura", 52, 5},
    {"ch08.shandong.bones", 53, 5},
};

test::HandPolicy intentPolicy() {
    test::HandPolicy p;
    p.healAtPercent = 50;
    p.healWhenDoomed = false;
    p.fleeWhenSpent = true;
    p.pills = {"pill_yangjing_dan", "pill_jinchuang_yao"};
    return p;
}

class Ch08Walkthrough : public ::testing::Test {
protected:
    void SetUp() override { reset(test::kChapterSevenEndingFirst); }
    void TearDown() override { app_->shutdown(); }
    Application& app() { return *app_; }
    GameState& state() { return app().state(); }

    void reset(const char* file) {
        if (app_) app_->shutdown();
        app_ = std::make_unique<Application>();
        const auto ready = app().init(root_, true);
        ASSERT_TRUE(ready.ok) << ready.error;
        const auto loaded = io::loadGame(test::chapterFixturePath(root_, file).string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        start_ = loaded.value;
        ASSERT_EQ(start_.flag("ch07.done"), 1);
        ASSERT_EQ(start_.realm, rules::Realm::FoundationEarly);
        ASSERT_EQ(start_.realmCap, rules::Realm::FoundationEarly);
        ASSERT_EQ(start_.maxHp, 260);
        ASSERT_EQ(start_.maxMp, 180);
        ASSERT_EQ(start_.bottle.capacity, 6);
        ASSERT_EQ(start_.itemCount("pill_zhuji_dan"), 17);
        ASSERT_TRUE(start_.party.empty());
        const std::set<std::string> magics = {"magic_huodan_shu", "magic_yufeng_jue", "magic_tianyan_shu",
            "magic_liusha_shu", "magic_bingdong_shu", "magic_ji_jinfu", "magic_ji_qingjiao", "magic_ji_qingning"};
        ASSERT_EQ(std::set<std::string>(start_.learnedMagics.begin(), start_.learnedMagics.end()), magics);
        ASSERT_EQ(start_.learnedMagics.size(), 8u);
        state() = start_;
        ASSERT_TRUE(app().loadMap(state().mapId, "").ok);
        state().position = start_.position;
        state().facing = start_.facing;
        ASSERT_EQ(test::comparableSaveLines(state()), test::comparableSaveLines(start_));
        fights_.clear();
        crafts_ = 0;
        prepDays_ = 0;
        arrayPurse_ = 0;
        combatStonesSpent_ = 0;
        reviewKeys_.clear();
        stopped_.clear();
    }

    std::string balance() {
        std::ostringstream out;
        out << "step=" << current_ + 1 << " " << stopped_ << " map=" << state().mapId
            << " xy=" << state().position.x << ',' << state().position.y << " day=" << state().day
            << " hp=" << state().hp << '/' << state().maxHp << " mp=" << state().mp << '/' << state().maxMp;
        for (const char* item : {"material_lingshi", "material_lingshi_zhong", "pill_yangjing_dan",
             "pill_jinchuang_yao", "pill_huiqi_dan", "talisman_mojiao_chujiao", "talisman_xuelingzuan",
             "pill_lianqi_san", "material_lianqi_yao", "herb_zishen_cao"})
            out << ' ' << item << '=' << state().itemCount(item);
        out << " jingong=" << state().flag("ch08.jingong") << " huanggong=" << state().flag("ch08.huanggong")
            << " scriptError=" << app().scripts().lastError();
        const auto* next = rules::currentObjective(app().data().objectives, state());
        if (next) out << " next=" << next->id << ':' << next->targetMap << '/' << next->targetObject;
        return out.str();
    }

    bool pump() {
        for (int frame = 0; frame < 8000 && app().scripts().isRunning(); ++frame) {
            app().tick(1.0 / 60.0);
            if (auto* battle = dynamic_cast<BattleScene*>(app().topScene())) {
                if (battle->battle().phase() != BattlePhase::Ongoing) continue;
                const std::size_t index = fights_.size();
                if (index >= std::size(kBattles)) return false;
                const std::string id = kBattles[index];
                std::multiset<std::pair<std::string, int>> actual, expected;
                for (const auto& u : battle->battle().units()) if (!u.ally) actual.emplace(u.id, u.wave);
                const auto* setup = app().battleSetup(id);
                if (!setup) return false;
                for (const auto& u : setup->units) if (!u.ally) expected.emplace(u.roleId, u.wave);
                EXPECT_EQ(actual, expected) << id << " unexpected battle request";
                const GameState before = state();
                test::BattleHand hand(app(), intentPolicy());
                const auto phase = hand.play(*battle);
                const int stonesUsed = before.itemCount("material_lingshi_zhong") - state().itemCount("material_lingshi_zhong");
                EXPECT_GE(stonesUsed, 0);
                combatStonesSpent_ += stonesUsed;
                std::cout << "[ch08 battle] side=" << side_ << " id=" << id << " phase=" << static_cast<int>(phase)
                          << " policy=heal50,noDoomed,pills:yangjing-jinchuang"
                          << " round=" << battle->battle().round() << " before hp=" << before.hp << '/' << before.maxHp
                          << " mp=" << before.mp << '/' << before.maxMp
                          << " pills=" << before.itemCount("pill_yangjing_dan")
                          << " salves=" << before.itemCount("pill_jinchuang_yao")
                          << " midstones=" << before.itemCount("material_lingshi_zhong")
                          << " huiqi=" << before.itemCount("pill_huiqi_dan")
                          << " horn=" << before.itemCount("talisman_mojiao_chujiao")
                          << " drill=" << before.itemCount("talisman_xuelingzuan")
                          << " remaining " << balance() << '\n';
                for (const auto& u : battle->battle().units())
                    std::cout << "  " << u.id << " hp=" << u.hp << '/' << u.maxHp << " mp=" << u.mp
                              << " toughness=" << u.toughness << " wave=" << u.wave << '\n';
                if (test::environmentFlagSet("FANREN_CH08_BATTLE_LOG") || phase != BattlePhase::Won)
                    for (const auto& line : battle->battle().log()) std::cout << "  " << line << '\n';
                fights_.push_back(id);
                EXPECT_EQ(phase, BattlePhase::Won) << balance();
                if (phase != BattlePhase::Won) return false;
                // Perturb a reachable battle state, with only its one-shot item exhausted.
                if (index == 2 || index == 5) {
                    Application isolated;
                    const auto ready = isolated.init(root_, true);
                    EXPECT_TRUE(ready.ok) << ready.error;
                    if (!ready.ok) return false;
                    isolated.state() = before;
                    const char* item = index == 2 ? "talisman_mojiao_chujiao" : "talisman_xuelingzuan";
                    EXPECT_EQ(before.itemCount(item), 1) << "perturbation must exhaust an actually acquired item";
                    EXPECT_TRUE(isolated.state().removeItem(item, isolated.state().itemCount(item)));
                    BattleScene noItem(id);
                    noItem.onEnter(isolated);
                    test::BattleHand other(isolated, intentPolicy());
                    const auto without = other.play(noItem);
                    std::cout << "[ch08 exhausted] " << id << " " << item << '=' << isolated.state().itemCount(item)
                              << " phase=" << static_cast<int>(without) << " round=" << noItem.battle().round()
                              << " pills=" << isolated.state().itemCount("pill_yangjing_dan")
                              << " salves=" << isolated.state().itemCount("pill_jinchuang_yao") << '\n';
                    for (const auto& u : noItem.battle().units()) if (u.ally)
                        std::cout << "  " << u.id << " hp=" << u.hp << '/' << u.maxHp << " mp=" << u.mp << '\n';
                    if (without != BattlePhase::Won)
                        for (const auto& line : noItem.battle().log()) std::cout << "  " << line << '\n';
                    EXPECT_EQ(without, BattlePhase::Won) << id << " without " << item
                        << " rounds=" << noItem.battle().round() << " inherited hp=" << before.hp
                        << " pills=" << before.itemCount("pill_yangjing_dan");
                    isolated.shutdown();
                }
                continue;
            }
            if (!app().awaitingCommand()) continue;
            for (const auto& key : app().spokenKeys()) {
                for (const auto& expected : kReviewKeys) {
                    if (key != expected.key) continue;
                    const auto [it, inserted] = reviewKeys_.emplace(key, std::make_pair(current_, fights_.size()));
                    EXPECT_TRUE(inserted) << key << " played more than once";
                    std::cout << "[ch08 key] side=" << side_ << " scene=" << key.substr(5, key.find('.', 5) - 5)
                              << " key=" << key << " step=" << it->second.first + 1
                              << " completedBattles=" << it->second.second << " xy=" << state().position.x
                              << ',' << state().position.y << '\n';
                }
            }
            script::CommandResult result;
            result.ok = true;
            result.choiceIndex = app().spokenKeys().empty() ? (choiceOverride_ < 0 ? side_ : choiceOverride_) : 0;
            app().clearSpokenKeys();
            app().completeCommand(result);
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
    bool move(Point d) {
        const bool moved = world_.tryStep(app(), d.x, d.y);
        return pump() && moved;
    }
    bool walk(Point goal) {
        if (state().position == goal) return true;
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
        if (!previous.count(key(goal))) return false;
        std::vector<Point> path;
        for (Point p = goal; p != from; p = previous.at(key(p))) path.push_back(p);
        std::reverse(path.begin(), path.end());
        const auto mapId = state().mapId;
        for (Point p : path) {
            const Point at = state().position;
            if (!move({p.x - at.x, p.y - at.y})) return false;
            if (state().mapId != mapId) return p == goal;
        }
        return state().position == goal;
    }
    bool face(const MapObject& o) {
        for (const auto d : kDirections) {
            const Point stand{o.position.x - d.x, o.position.y - d.y};
            if (!passable(stand)) continue;
            if (!passable(o.position, true)) {
                if (!walk(stand)) continue;
                // A blocked movement still turns the player toward the wall-mounted hook.
                world_.tryStep(app(), d.x, d.y);
                if (pump()) return true;
                return false;
            }
            const Point approach{stand.x - d.x, stand.y - d.y};
            if (!passable(approach) || !walk(approach)) continue;
            if (move(d) && state().position == stand) return true;
        }
        return false;
    }
    bool press(const std::string& name) {
        const auto o = object(name);
        if (o.name.empty() || !face(o)) return false;
        if (name == "trigger_shanfeng" && test::environmentFlagSet("FANREN_CH08_WORLD_QA")) {
            const auto file = std::filesystem::path(root_) / "build-resume8test" /
                ("ch08-world-mouse-qa-" + std::to_string(side_) + ".sav");
            const auto saved = io::saveGame(state(), file.string());
            EXPECT_TRUE(saved.ok) << saved.error;
        }
        if (!world_.interact(app())) return false;
        return pump();
    }
    bool enter(const std::string& name) {
        const auto o = object(name);
        if (o.name.empty()) return false;
        if (state().position != o.position) return walk(o.position);
        // Script teleport can land exactly on the next enter hook; it still needs a real step.
        for (const auto d : kDirections) {
            const Point beside{o.position.x - d.x, o.position.y - d.y};
            if (passable(beside) && walk(beside)) return move(d);
        }
        return false;
    }
    bool travel(const std::string& target) {
        for (int hop = 0; hop < 12 && state().mapId != target; ++hop) {
            const auto* link = rules::nextPortalToward(app().portalLinks(), state().mapId, target, state());
            if (!link) return false;
            const auto o = object(link->portalName);
            const auto before = state().mapId;
            if (!walk(o.position) || state().mapId == before) return false;
        }
        return state().mapId == target;
    }
    bool open(const std::string& name) {
        const auto o = object(name);
        if (o.name.empty() || !face(o) || !world_.interact(app())) return false;
        app().tick(1.0 / 60.0);
        return true;
    }
    void close() { app().popScene(); app().tick(1.0 / 60.0); }
    bool sit(int days, const char* facility = "facility_lingquan") {
        if (!open(facility)) return false;
        auto* panel = dynamic_cast<game::CultivationScene*>(app().topScene());
        if (!panel) return false;
        const int before = state().day;
        panel->meditateFor(app(), days);
        prepDays_ += state().day - before;
        close();
        return state().day == before + days;
    }
    bool grow(const char* herb, int age) {
        if (!open("facility_yaoyuan")) return false;
        auto* field = dynamic_cast<game::FieldScene*>(app().topScene());
        if (!field) return false;
        const bool planted = field->plantAt(app(), 0, herb);
        close();
        if (!planted) return false;
        for (int tries = 0; tries < 80; ++tries) {
            const auto* f = state().findField("field_dongfu");
            if (!f || f->slots[0].seedId != herb) return false;
            if (f->slots[0].age >= age) break;
            if (!sit(5) || !open("facility_yaoyuan")) return false;
            field = dynamic_cast<game::FieldScene*>(app().topScene());
            if (!field) return false;
            // Newly planted herbs must ripen before a drop can be poured.
            field->matureAt(app(), 0);
            close();
        }
        if (!open("facility_yaoyuan")) return false;
        field = dynamic_cast<game::FieldScene*>(app().topScene());
        if (!field) return false;
        const bool harvested = field->harvestAt(app(), 0);
        close();
        return harvested && state().itemCountAtLeastAge(herb, age) > 0;
    }
    bool preparePills() {
        if (!travel("ch08_tianxing_fangshi") || !open("facility_yaopu")) return false;
        auto* shop = dynamic_cast<game::ShopScene*>(app().topScene());
        if (!shop) return false;
        const auto* stock = app().shop("ch08_tianxing_yaopu");
        if (!stock) return false;
        int row = -1;
        for (std::size_t i = 0; i < stock->entries.size(); ++i)
            if (stock->entries[i].itemId == "material_lianqi_yao") row = static_cast<int>(i);
        if (row < 0) return false;
        for (int i = 0; i < 3; ++i) if (!shop->buyAt(app(), row)) return false;
        shop->leave(app()); app().tick(1.0 / 60.0);
        if (!travel("ch08_dongfu")) return false;
        for (int i = 0; i < 12 && state().itemCount("pill_lianqi_san") < 2; ++i) {
            if (i < 3) {
                if (!grow("herb_zishen_cao", 100)) return false;
            } else {
                const int before = state().day;
                choiceOverride_ = 0;
                const bool worked = press("trigger_beiyao");
                choiceOverride_ = -1;
                if (!worked) return false;
                prepDays_ += state().day - before;
            }
            if (!open("facility_danfang")) return false;
            auto* furnace = dynamic_cast<game::AlchemyScene*>(app().topScene());
            if (!furnace) return false;
            const auto& ids = furnace->recipeIds();
            const auto it = std::find(ids.begin(), ids.end(), "recipe_lianqi_san");
            if (it == ids.end() || !furnace->craftAt(app(), static_cast<int>(it - ids.begin()))) return false;
            ++crafts_;
            furnace->leave(app()); app().tick(1.0 / 60.0);
        }
        return state().itemCount("pill_lianqi_san") >= 2;
    }
    bool formation(int group) {
        const std::vector<std::string> names = group == 1
            ? std::vector<std::string>{"trigger_zhenwei_jin", "trigger_zhenwei_mu", "trigger_zhenwei_shui", "trigger_zhenwei_huo"}
            : std::vector<std::string>{group == 2 ? "trigger_zhulin_1" : "trigger_milin_1",
                  group == 2 ? "trigger_zhulin_2" : "trigger_milin_2", group == 2 ? "trigger_zhulin_3" : "trigger_milin_3",
                  group == 2 ? "trigger_zhulin_4" : "trigger_milin_4"};
        for (const auto& name : names) if (!press(name)) return false;
        return state().flag("ch08.zhenqi" + std::to_string(group)) == 15;
    }
    bool chapter(int side, const char* fixture) {
        side_ = side;
        if (side == 1) reset(test::kChapterSevenEndingSecond);
        for (current_ = 0; current_ < test::kChapterEightSteps.size(); ++current_) {
            const auto& s = test::kChapterEightSteps[current_];
            stopped_ = s.object;
            SCOPED_TRACE(balance());
            if (current_ == 4 && !grow("herb_zigui_hua", 1000)) return false;
            if (current_ == 15 && !preparePills()) return false;
            if (!travel(s.map)) return false;
            if (current_ == 10) {
                arrayPurse_ = state().itemCount("material_lingshi_zhong");
                EXPECT_EQ(arrayPurse_, start_.itemCount("material_lingshi_zhong") + 3);
                if (!formation(1)) return false;
            }
            if (current_ == 43 && !formation(2)) return false;
            if (current_ == 55 && !formation(3)) return false;
            // A player can recover at the shipped meditation facilities, costing real days.
            if (current_ == 20 || current_ == 23 || current_ == 24 || current_ == 30 || current_ == 47 || current_ == 54) {
                if (state().hp < state().maxHp || state().mp < state().maxMp) {
                    if (!sit(10, "facility_yangxi")) return false;
                }
            }
            const int day = state().day;
            const int low = state().itemCount("material_lingshi");
            if (state().flag(s.done) == 0) {
                const bool fired = s.enter ? enter(s.object) : press(s.object);
                if (current_ == 46) {
                    std::cout << "[ch08 coldpalace] " << balance() << '\n';
                    EXPECT_EQ(state().flag("ch08.jingong"), 1);
                    EXPECT_EQ(state().flag("ch08.huanggong"), 1);
                    EXPECT_TRUE(app().scripts().lastError().empty());
                    EXPECT_EQ(state().mapId, "ch08_yuejing");
                    EXPECT_EQ(state().position, (Point{19, 12}));
                    const auto* next = rules::currentObjective(app().data().objectives, state());
                    EXPECT_NE(next, nullptr);
                    if (next) EXPECT_EQ(next->doneFlag, "ch08.yuehuang");
                }
                if (!fired && state().flag(s.done) == 0) return false;
            }
            if (state().flag(s.done) == 0) return false;
            EXPECT_EQ(state().day - day, s.days) << s.object << " design 3.3";
            const std::map<std::size_t, int> payments = {{1, -3}, {3, -12}, {12, 5}, {18, -40},
                                                        {25, 150}, {29, 100}, {56, -20}};
            if (payments.count(current_))
                EXPECT_EQ(state().itemCount("material_lingshi") - low, payments.at(current_)) << s.object;
            if (current_ < 26) EXPECT_EQ(state().realm, rules::Realm::FoundationEarly);
            if (current_ == 10) {
                // Controller M2, 2026-10-09: replaces design's fixed three with half the current purse.
                EXPECT_EQ(state().itemCount("material_lingshi_zhong"), arrayPurse_ - arrayPurse_ / 2);
            }
            if (current_ == 0) EXPECT_TRUE(state().knowsMagic("magic_qingyuan_jianmang"));
            if (current_ == 9) EXPECT_TRUE(state().knowsMagic("magic_ji_qinghuozhang"));
            if (current_ == 20) {
                EXPECT_TRUE(state().knowsMagic("magic_ji_wulongduo"));
                EXPECT_TRUE(state().knowsMagic("magic_ji_xiaodao"));
            }
            if (current_ == 22) EXPECT_TRUE(hasParty("kuilei_shou"));
            if (current_ == 25) {
                EXPECT_TRUE(state().knowsMagic("magic_ji_qingchi"));
                EXPECT_EQ(state().itemCount("talisman_qingchi_fubao"), 1);
            }
            if (current_ == 32) {
                EXPECT_EQ(state().itemCount("story_diandao_zhenqi_gai"), 1);
                EXPECT_EQ(state().itemCount("story_diandao_zhenqi"), 0);
            }
            if (current_ == 46) EXPECT_FALSE(hasParty("kuilei_shou"));
            if (current_ == 54) EXPECT_TRUE(hasParty("kuilei_shou"));
            if (current_ == 47) {
                EXPECT_FALSE(state().knowsMagic("magic_ji_qingchi"));
                EXPECT_EQ(state().itemCount("talisman_qingchi_fubao"), 0);
            }
        }
        EXPECT_EQ(fights_, (std::vector<std::string>(std::begin(kBattles), std::end(kBattles))));
        EXPECT_GT(crafts_, 0);
        EXPECT_EQ(state().realm, rules::Realm::FoundationMid);
        EXPECT_EQ(state().realmCap, rules::Realm::FoundationMid);
        EXPECT_GE(state().maxHp, 380);
        EXPECT_GE(state().maxMp, 250);
        auto magics = std::set<std::string>(start_.learnedMagics.begin(), start_.learnedMagics.end());
        for (const char* id : {"magic_qingyuan_jianmang", "magic_ji_qinghuozhang", "magic_ji_wulongduo", "magic_ji_xiaodao"})
            magics.insert(id);
        EXPECT_EQ(std::set<std::string>(state().learnedMagics.begin(), state().learnedMagics.end()), magics);
        EXPECT_EQ(state().learnedMagics.size(), 12u);
        EXPECT_EQ(state().itemCount("pill_zhuji_dan"), 16);
        EXPECT_EQ(state().itemCount("pill_dingyan_dan"), side == 0 ? 5 : 6);
        EXPECT_TRUE(hasParty("kuilei_shou"));
        EXPECT_FALSE(hasParty("qu_hun"));
        for (const char* item : {"story_lvhuang_jian", "story_danuoyi_ling", "story_xueyu_zhizhu",
             "story_diandao_zhenqi_gai", "story_yinhun_zhong"}) EXPECT_EQ(state().itemCount(item), 1) << item;
        for (const char* item : {"story_diandao_zhenqi", "story_yinse_shuye", "talisman_qingchi_fubao"})
            EXPECT_EQ(state().itemCount(item), 0) << item;
        EXPECT_NE(state().findField("field_dongfu"), nullptr);
        if (!state().findField("field_dongfu")) return false;
        EXPECT_EQ(state().findField("field_dongfu")->slots.size(), 4u);
        EXPECT_NE(state().findField("field_dongfu_nei"), nullptr);
        if (!state().findField("field_dongfu_nei")) return false;
        EXPECT_EQ(state().findField("field_dongfu_nei")->slots.size(), 2u);
        EXPECT_EQ(state().itemCount("material_lingshi") - start_.itemCount("material_lingshi"), 75);
        EXPECT_EQ(state().itemCount("material_lingshi_zhong"),
                  start_.itemCount("material_lingshi_zhong") + 12 - arrayPurse_ / 2 - combatStonesSpent_);
        const int scriptDays = std::accumulate(test::kChapterEightSteps.begin(), test::kChapterEightSteps.end(), 0,
            [](int days, const auto& step) { return days + step.days; });
        EXPECT_EQ(scriptDays, 2113);  // Design 3.3: 2210 is approximate and includes player gardening days.
        EXPECT_EQ(state().day, start_.day + scriptDays + prepDays_);
        for (const char* branch : {"ch08.yexi", "ch08.leiwen", "ch08.lvbo", "ch08.caihuan", "ch08.tianheju",
             "ch08.kuilei", "ch08.nangong", "ch08.fengwu", "ch08.huayuan", "ch08.junling", "ch08.lifu", "ch08.quhun"})
            EXPECT_EQ(state().flag(branch), side + 1) << branch;
        for (const auto& expected : kReviewKeys) {
            const auto found = reviewKeys_.find(expected.key);
            EXPECT_NE(found, reviewKeys_.end()) << expected.key << " never played on this real route";
            if (found == reviewKeys_.end()) continue;
            EXPECT_EQ(found->second.first, expected.step) << expected.key;
            EXPECT_EQ(found->second.second, expected.completedBattles) << expected.key << " dialogue ordering";
        }
        if (HasFailure()) return false;
        const test::TempDir temp("fanren_ch08_handover");
        const auto probe = temp.path() / "ending.sav";
        const auto wrote = io::saveGame(state(), probe.string());
        EXPECT_TRUE(wrote.ok) << wrote.error;
        if (!wrote.ok) return false;
        const auto saved = io::loadGame(probe.string());
        EXPECT_TRUE(saved.ok) << saved.error;
        if (!saved.ok) return false;
        EXPECT_EQ(test::comparableSaveLines(saved.value), test::comparableSaveLines(state()));
        Application resumed;
        const auto ready = resumed.init(root_, true);
        EXPECT_TRUE(ready.ok) << ready.error;
        if (!ready.ok) return false;
        const auto continued = resumed.continueJourney(probe.string());
        EXPECT_TRUE(continued.ok) << continued.error;
        if (continued.ok)
            EXPECT_EQ(test::comparableSaveLines(resumed.state()), test::comparableSaveLines(state()));
        resumed.shutdown();
        if (HasFailure()) return false;
        const auto verdict = test::settleAgainstFixture(state(), root_, fixture, test::kWriteChapterEightFixturesEnv, "Chapter 9");
        EXPECT_TRUE(verdict.problem.empty()) << verdict.problem;
        if (!verdict.problem.empty()) return false;
        const auto path = test::chapterFixturePath(root_, fixture);
        const auto handedOver = io::loadGame(path.string());
        EXPECT_TRUE(handedOver.ok) << handedOver.error;
        if (handedOver.ok)
            EXPECT_EQ(test::comparableSaveLines(handedOver.value), test::comparableSaveLines(state()));
        std::cout << "[ch08 ending] side=" << side_ << " wrote=" << verdict.wrote
                  << " file=" << path.string() << " " << balance() << '\n';
        return !HasFailure();
    }
    bool hasParty(const std::string& id) {
        return std::any_of(state().party.begin(), state().party.end(), [&id](const auto& p) { return p.roleId == id; });
    }

    std::string root_ = test::chapterEightAssetRoot();
    std::unique_ptr<Application> app_;
    WorldScene world_;
    GameState start_;
    int side_ = 0, crafts_ = 0, prepDays_ = 0, choiceOverride_ = -1;
    int arrayPurse_ = 0, combatStonesSpent_ = 0;
    std::size_t current_ = 0;
    std::string stopped_;
    std::vector<std::string> fights_;
    std::map<std::string, std::pair<std::size_t, std::size_t>> reviewKeys_;
};

TEST_F(Ch08Walkthrough, FirstSideWalksEveryGateAndHandsOverItsRealEnding) {
    EXPECT_TRUE(chapter(0, test::kChapterEightEndingFirst)) << balance();
}
TEST_F(Ch08Walkthrough, SecondSideTakesEveryOtherStoryChoiceAndHandsOverItsRealEnding) {
    EXPECT_TRUE(chapter(1, test::kChapterEightEndingSecond)) << balance();
}
TEST_F(Ch08Walkthrough, TheTwoGatesActuallyRejectThePlayerBeforeTheirStoryKeys) {
    ASSERT_TRUE(travel("ch06_huangfenggu"));
    const auto first = object("portal_to_dongfu");
    ASSERT_FALSE(first.name.empty());
    EXPECT_EQ(first.property("require_flag"), "ch08.dengji");
    walk(first.position);
    EXPECT_EQ(state().mapId, "ch06_huangfenggu");
    EXPECT_EQ(state().flag("ch08.dengji"), 0);
    ASSERT_NE(app().topScene(), nullptr);
    EXPECT_EQ(app().topScene()->name(), "Dialogue");
    close();
    ASSERT_TRUE(travel("ch06_baiyaoyuan"));
    ASSERT_TRUE(press("trigger_shixiong"));
    ASSERT_TRUE(travel("ch06_huangfenggu"));
    ASSERT_TRUE(press("trigger_dengji"));
    ASSERT_EQ(state().mapId, "ch08_dongfu");
    const auto second = object("portal_to_tianxing_fangshi");
    ASSERT_FALSE(second.name.empty());
    EXPECT_EQ(second.property("require_flag"), "ch08.qiannian");
    walk(second.position);
    EXPECT_EQ(state().mapId, "ch08_dongfu");
    EXPECT_EQ(state().flag("ch08.qiannian"), 0);
    ASSERT_NE(app().topScene(), nullptr);
    EXPECT_EQ(app().topScene()->name(), "Dialogue");
    close();
}

class Ch08WorldSprite : public ::testing::Test {
protected:
    void SetUp() override {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        const auto ready = engine_.init("ch08-world-sprite", false);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override {
        engine_.shutdown();
        SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
    }
    engine::Engine engine_;
};

TEST_F(Ch08WorldSprite, MouseResolvesThroughTheRealWorldSheetAndAllFourDirectionsLoad) {
    const auto root = std::filesystem::path(test::chapterEightAssetRoot());
    const auto index = io::loadSpriteIndex((root / "assets/art/sprites/index.json").string());
    ASSERT_TRUE(index.ok) << index.error;
    const game::ChapterDone done = [](int n) { return n <= 7; };
    const auto* mouse = game::sheetForRole(index.value, "shuangtong_shu", done);
    ASSERT_NE(mouse, nullptr) << "WorldView::drawCharacter only reads roles/sheets; enemies is insufficient";
    EXPECT_NE(mouse, game::sheetForRole(index.value, "hanli", done));
    const auto texture = engine_.loadTexture((root / "assets/art/sprites" / mouse->file).string(), engine::ScaleMode::Pixel);
    ASSERT_NE(texture, engine::kInvalidTexture);
    const auto size = engine_.textureSize(texture);
    ASSERT_GT(size.x, 0);
    ASSERT_GT(size.y, 0);
    ASSERT_GT(mouse->cols, 0);
    ASSERT_GT(mouse->frameW, 0);
    ASSERT_GT(mouse->frameH, 0);
    for (int facing = 0; facing < 4; ++facing) {
        SCOPED_TRACE(facing);
        ASSERT_GE(mouse->walk[static_cast<std::size_t>(facing)].size(), 3u);
        for (int frame : mouse->walk[static_cast<std::size_t>(facing)]) {
            EXPECT_GE(frame, 0);
            const auto rect = game::sheetFrameRect(*mouse, frame);
            EXPECT_GE(rect.x, 0);
            EXPECT_GE(rect.y, 0);
            EXPECT_LE(rect.x + rect.w, static_cast<float>(size.x));
            EXPECT_LE(rect.y + rect.h, static_cast<float>(size.y));
        }
        EXPECT_EQ(game::walkFrame(index.value, *mouse, facing, false, 0), mouse->walk[static_cast<std::size_t>(facing)][0]);
    }
    auto battleOnly = index.value;
    battleOnly.roles.erase("shuangtong_shu");
    battleOnly.variants.erase("shuangtong_shu");
    EXPECT_EQ(game::sheetForRole(battleOnly, "shuangtong_shu", done), nullptr);
}
}  // namespace
