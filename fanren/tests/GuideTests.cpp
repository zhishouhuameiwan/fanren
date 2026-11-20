// 第一期地基包·路 G：指引（docs/ch07-foundation.md 第 2 节，验收判据 2.8）。
//
//   1. 目标两行式：第 7 章 50 步行动句至多 16 个字、要点至多 28 个字、长句都在 detail_key、要数数的都有进度；
//      进度三种读法（物品、按年份的物品、旗标）与「1/3」的写法。
//   2. 跨图指路：routeToward 与 nextPortalToward 同一个判据；第 7 章每一步都写得出「去哪、经哪」，或目标在本图。
//   3. 教学卡：数据形状（不少于 14 张、各至多三行、每行至多 30 个字、只在第 7 章）、只弹一次、关掉设置不弹但记作看过、
//      战斗里压在战斗上面、对白进行中不弹；缺省关着（无头测试与通关验收不受影响）。
//   4. 记事四页：翻得动；支线写出奖励（G-25，钱按 Wording 叫）；指南列得出看过的卡；见闻页写出妖兽的已知破绽与掉落。
//   5. 第 7 章以外的目标框不变：没写新字段的步骤，目标行与「前往」行与从前逐字相同。
//   6. 见闻：数据形状、解锁与「见闻：XXX」提示（头一回不报）。
#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../vendor/json.hpp"

#include "core/model/AttackCategory.h"
#include "core/model/Types.h"
#include "core/rules/Lore.h"
#include "core/rules/Objectives.h"
#include "core/rules/Tutorials.h"
#include "game/Application.h"
#include "game/BoardScene.h"
#include "game/DialogueScene.h"
#include "game/Guide.h"
#include "game/LoreNotices.h"
#include "game/TutorialScene.h"
#include "game/WorldScene.h"
#include "io/DataLoader.h"
#include "io/GuideLoader.h"
#include "io/SaveFile.h"

namespace fs = std::filesystem;
using fanren::core::GameData;
using fanren::core::GameState;
using fanren::core::Objective;
using fanren::core::ObjectiveProgress;
using fanren::game::Application;
using fanren::game::BoardScene;
using fanren::rules::PortalLink;

namespace {

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

// UTF-8 码点数（「至多 16 个字」数的是字，不是字节）。
std::size_t glyphs(const std::string& s) {
    std::size_t n = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) ++n;
    }
    return n;
}

const GameData& realData() {
    static const GameData data = [] {
        auto loaded = fanren::io::loadGameData((fs::path(assetRoot()) / "data").string());
        EXPECT_TRUE(loaded.ok) << loaded.error;
        return loaded.value;
    }();
    return data;
}

std::vector<const Objective*> chapterSteps(const GameData& data, int chapter) {
    std::vector<const Objective*> out;
    for (const Objective& step : data.objectives) {
        if (step.chapter == chapter) out.push_back(&step);
    }
    return out;
}

std::set<std::string> registeredFlags() {
    std::ifstream in(fs::path(assetRoot()) / "data" / "flags.json", std::ios::binary);
    const nlohmann::json j = nlohmann::json::parse(in);
    std::set<std::string> out;
    for (auto it = j.begin(); it != j.end(); ++it) out.insert(it.key());
    return out;
}

PortalLink door(std::string from, std::string name, std::string to, std::string flag = {}) {
    return PortalLink{std::move(from), std::move(name), fanren::core::Point{0, 0}, std::move(to), std::move(flag)};
}

void pushKey(SDL_Scancode scancode, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.down = down;
    event.key.repeat = false;
    SDL_PushEvent(&event);
}

}  // namespace

// ===========================================================================
// 1. 目标两行式
// ===========================================================================

TEST(GuideObjectives, ChapterSevenHasFiftyShortActionLinesWithTheLongOnesKeptAsDetail) {
    const GameData& data = realData();
    const std::vector<const Objective*> steps = chapterSteps(data, 7);
    ASSERT_EQ(steps.size(), 50u);
    for (const Objective* step : steps) {
        const std::string action = data.lookupText(step->textKey);
        ASSERT_NE(action, step->textKey) << step->id << " 的行动句没有文案";
        EXPECT_LE(glyphs(action), 16u) << step->id << "：" << action;
        ASSERT_FALSE(step->detailKey.empty()) << step->id << " 没挂详情";
        const std::string detail = data.lookupText(step->detailKey);
        EXPECT_NE(detail, step->detailKey) << step->id << " 的详情没有文案";
        // 原句本来就短的几步（n22、n40……）详情与行动句相同；长句的那些，详情必须比行动句长——长句没丢。
        EXPECT_GE(glyphs(detail), glyphs(action)) << step->id << " 的详情比行动句还短：原来的长句丢了";
        if (!step->hintKey.empty()) {
            const std::string hint = data.lookupText(step->hintKey);
            EXPECT_NE(hint, step->hintKey) << step->id << " 的要点没有文案";
            EXPECT_LE(glyphs(hint), 28u) << step->id << "：" << hint;
        }
    }
}

TEST(GuideObjectives, EveryStepThatCountsSomethingShowsItsProgress) {
    // 要凑数量的几步（脚本里按数量收的：药篓、藏室、园角、火灵石、盘点）都要有进度，数的东西与脚本同一个口径。
    const GameData& data = realData();
    const std::map<std::string, std::tuple<ObjectiveProgress::Kind, std::string, int, int>> expected{
        {"n02_jiaoyao", {ObjectiveProgress::Kind::AgedItem, "herb_huangjing_cao", 44, 3}},     // yaolou.lua
        {"n03_danbao", {ObjectiveProgress::Kind::AgedItem, "herb_zishen_cao", 100, 2}},        // yaolou.lua
        {"n05_cangshi", {ObjectiveProgress::Kind::Item, "material_lingshi", 0, 22}},           // cangshi.lua：1 + 1 + 20
        {"n11_qiannian", {ObjectiveProgress::Kind::AgedItem, "herb_huangjing_zhi", 1000, 2}},  // yuanjiao.lua
        {"n37c_huolingshi", {ObjectiveProgress::Kind::Item, "material_huolingshi", 0, 1}},     // huolingshi.lua
        {"n45_chengdan", {ObjectiveProgress::Kind::Item, "pill_zhuji_dan", 0, 4}},             // banian.lua
    };
    for (const Objective* step : chapterSteps(data, 7)) {
        const auto it = expected.find(step->id);
        if (it == expected.end()) {
            EXPECT_EQ(step->progress.kind, ObjectiveProgress::Kind::None) << step->id << " 多挂了进度";
            continue;
        }
        const auto& [kind, item, minAge, need] = it->second;
        EXPECT_EQ(step->progress.kind, kind) << step->id;
        EXPECT_EQ(step->progress.item, item) << step->id;
        EXPECT_EQ(step->progress.minAge, minAge) << step->id;
        EXPECT_EQ(step->progress.need, need) << step->id;
        EXPECT_NE(data.findItem(item), nullptr) << step->id << " 数的物品不存在：" << item;
    }
}

TEST(GuideObjectives, ProgressReadsTheBagAndTheFlagsAndClampsAtTheNeed) {
    Objective step;
    step.textKey = "t";
    GameState state;
    fanren::rules::ProgressCount count;
    EXPECT_FALSE(fanren::rules::objectiveProgress(step, state, count)) << "不数数的步骤不该有进度";

    step.progress.kind = ObjectiveProgress::Kind::AgedItem;
    step.progress.item = "herb";
    step.progress.minAge = 44;
    step.progress.need = 3;
    state.addItem("herb", 2, 44);
    state.addItem("herb", 5, 43);   // 差一年：不算
    ASSERT_TRUE(fanren::rules::objectiveProgress(step, state, count));
    EXPECT_EQ(fanren::rules::progressLabel(count), "2/3");
    state.addItem("herb", 4, 100);
    ASSERT_TRUE(fanren::rules::objectiveProgress(step, state, count));
    EXPECT_EQ(fanren::rules::progressLabel(count), "3/3") << "揣多了也只写到需要的数";

    step.progress.kind = ObjectiveProgress::Kind::Item;   // 各年份合计
    step.progress.need = 20;
    ASSERT_TRUE(fanren::rules::objectiveProgress(step, state, count));
    EXPECT_EQ(fanren::rules::progressLabel(count), "11/20");

    step.progress.kind = ObjectiveProgress::Kind::Flags;
    step.progress.flags = {"a", "b", "c"};
    step.progress.need = 3;
    state.setFlag("b");
    ASSERT_TRUE(fanren::rules::objectiveProgress(step, state, count));
    EXPECT_EQ(fanren::rules::progressLabel(count), "1/3");

    // 目标框那一行：行动句接「　1/3」。
    GameData data;
    data.text["t"] = "做一件事";
    EXPECT_EQ(fanren::game::guide::objectiveActionLine(data, state, step), "做一件事　1/3");
}

TEST(GuideObjectives, TheLoaderRejectsABrokenProgress) {
    const fs::path root = fs::temp_directory_path() / "fanren_guide_progress";
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root / "objectives");
    const auto write = [&root](const std::string& progress) {
        std::ofstream out(root / "objectives" / "ch01.json", std::ios::binary);
        out << R"({"id": "o", "chapter": 1, "steps": [{"id": "s", "text_key": "k", "done_flag": "f",
                  "target_map": "m_a", "target_object": "o")"
            << progress << "}]}";
    };
    write("");
    ASSERT_TRUE(fanren::io::loadGameData(root.string()).ok) << "没写进度的老格式照收";
    write(R"(, "progress": {"kind": "item", "item": "x", "need": 2})");
    EXPECT_TRUE(fanren::io::loadGameData(root.string()).ok);
    for (const char* bad : {R"(, "progress": {"kind": "item", "item": "x"})",
                            R"(, "progress": {"kind": "aged_item", "item": "x", "need": 2})",
                            R"(, "progress": {"kind": "flags", "flags": []})",
                            R"(, "progress": {"kind": "count", "item": "x", "need": 1})",
                            R"(, "hint_key": 3)"}) {
        write(bad);
        EXPECT_FALSE(fanren::io::loadGameData(root.string()).ok) << bad;
    }
    fs::remove_all(root, ec);
}

// ===========================================================================
// 2. 跨图指路
// ===========================================================================

TEST(GuideRoute, TheWholeRouteStartsWithTheDoorThatLightsAndSkipsLockedDoors) {
    const std::vector<PortalLink> links{
        door("A", "a_to_b", "B", "f.key"), door("B", "b_to_c", "C"),
        door("A", "a_to_d", "D"), door("D", "d_to_e", "E"), door("E", "e_to_c", "C"),
    };
    GameState state;
    std::vector<const PortalLink*> route = fanren::rules::routeToward(links, "A", "C", state);
    ASSERT_EQ(route.size(), 3u) << "锁着的门不算路：只能绕 D、E";
    EXPECT_EQ(route.front(), fanren::rules::nextPortalToward(links, "A", "C", state)) << "第一道门与亮的门不是同一道";
    EXPECT_EQ(fanren::rules::routeWaypoints(route), (std::vector<std::string>{"D", "E"}));

    state.flags["f.key"] = 1;
    route = fanren::rules::routeToward(links, "A", "C", state);
    ASSERT_EQ(route.size(), 2u);
    EXPECT_EQ(route.front()->portalName, "a_to_b");
    EXPECT_EQ(fanren::rules::routeWaypoints(route), (std::vector<std::string>{"B"}));

    EXPECT_TRUE(fanren::rules::routeToward(links, "A", "A", state).empty()) << "目标在脚下";
    EXPECT_TRUE(fanren::rules::routeToward(links, "C", "A", state).empty()) << "走不到";
    // 直达：没有「经」。
    EXPECT_TRUE(fanren::rules::routeWaypoints(fanren::rules::routeToward(links, "A", "B", state)).empty());

    EXPECT_EQ(fanren::game::guide::viaClause({}), "");
    EXPECT_EQ(fanren::game::guide::viaClause({"黄枫谷"}), "（经黄枫谷）");
    EXPECT_EQ(fanren::game::guide::viaClause({"甲", "乙", "丙", "丁"}), "（经甲、乙、丙…）");
}

namespace {

// 一步的脚本把人送到哪张图（teleport 的最后一处）；不送人返回空串。
std::string landingOf(const std::string& mapId, const std::string& objectName) {
    std::ifstream in(fs::path(assetRoot()) / "maps" / (mapId + ".tmj"), std::ios::binary);
    const nlohmann::json map = nlohmann::json::parse(in);
    std::string script;
    for (const auto& layer : map["layers"]) {
        if (!layer.contains("objects")) continue;
        for (const auto& object : layer["objects"]) {
            if (object.value("name", "") != objectName || !object.contains("properties")) continue;
            for (const auto& p : object["properties"]) {
                if (p.value("name", "") == "script") script = p.value("value", "");
            }
        }
    }
    if (script.empty()) return {};
    std::ifstream lua(fs::path(assetRoot()) / "scripts" / script, std::ios::binary);
    std::stringstream text;
    text << lua.rdbuf();
    const std::string source = text.str();
    std::string landing;
    const std::regex teleport(R"(teleport\(\s*\"([a-z0-9_]+)\")");
    for (auto it = std::sregex_iterator(source.begin(), source.end(), teleport); it != std::sregex_iterator(); ++it) {
        landing = (*it)[1];
    }
    return landing;
}

}  // namespace

TEST(GuideRoute, EveryChapterSevenStepSaysWhereToGoAndWhichMapsToPass) {
    // 第 7 章章首存档起步，一步一步往下推：做完前面几步（置它们的完成旗标），人站在上一步把他留下的那张图上
    //（上一步的脚本 teleport 走了，就是 teleport 到的那张），问这一步：目标在本图，或走得到、「前往」行写着目标图。
    Application app;
    ASSERT_TRUE(app.init(assetRoot(), /*headless=*/true).ok);
    auto save = fanren::io::loadGame((fs::path(assetRoot()) / "saves" / "ch07-start.sav").string());
    ASSERT_TRUE(save.ok) << save.error;
    app.state() = save.value;
    const std::vector<const Objective*> steps = chapterSteps(app.data(), 7);
    ASSERT_EQ(steps.size(), 50u);

    std::string here = save.value.mapId;
    int withVia = 0;
    std::vector<std::string> unreachable;
    for (std::size_t i = 0; i < steps.size(); ++i) {
        const Objective& step = *steps[i];
        ASSERT_TRUE(app.loadMap(here, "").ok) << here;
        ASSERT_EQ(fanren::rules::currentObjective(app.data().objectives, app.state()), &step)
            << "先验：推到第 " << i + 1 << " 步";
        const std::string where = fanren::game::WorldScene::objectiveWhereText(app);
        if (step.targetMap == here) {
            EXPECT_TRUE(where.empty()) << step.id << "：目标在本图还报地名：" << where;
        } else {
            const auto route = fanren::rules::routeToward(app.portalLinks(), here, step.targetMap, app.state());
            const std::string goal = app.text(fanren::core::mapDisplayNameKey(step.targetMap));
            EXPECT_NE(where.find("前往　" + goal), std::string::npos) << step.id << "：" << where;
            if (route.empty()) {
                unreachable.push_back(step.id + "（" + here + " → " + step.targetMap + "）");
            } else {
                for (const std::string& via : fanren::rules::routeWaypoints(route)) {
                    EXPECT_NE(where.find(app.text(fanren::core::mapDisplayNameKey(via))), std::string::npos)
                        << step.id << " 没写途经的 " << via << "：" << where;
                    ++withVia;
                }
            }
        }
        app.state().setFlag(step.doneFlag);
        const std::string landing = landingOf(step.targetMap, step.targetObject);
        here = landing.empty() ? step.targetMap : landing;
    }
    EXPECT_TRUE(unreachable.empty()) << "这几步从上一步落脚处走不到：" << ::testing::PrintToString(unreachable);
    EXPECT_GT(withVia, 0) << "先验：第 7 章至少有一步要经过别的图（百药园 → 黄枫谷 → 岳麓殿）";
    app.shutdown();
}

TEST(GuideRoute, OtherChaptersKeepTheirOldWhereLine) {
    // 判据 5：没照新版式写的步骤（第 7 章以外），「前往」行照旧只报下一站「先到」。
    GameData data = realData();
    for (const Objective& step : data.objectives) {
        if (step.chapter == 7) continue;
        EXPECT_FALSE(fanren::game::guide::upgradedObjective(step)) << step.id << " 不是第 7 章却挂了新字段";
    }
}

// ===========================================================================
// 3. 教学卡
// ===========================================================================

namespace {

// Application 发得出的引擎事件（Application.cpp、BattleScene::openMenu、WorldScene::noteGuideEvents）。
const std::set<std::string>& knownEvents() {
    static const std::set<std::string> events{
        "battle.command", "battle.round2", "battle.round3", "battle.item_cast", "foe.near", "foe.alert",
        "loot.near",      "gate.near",     "world.idle",    "world.hurt",       "roster.joined", "bond.gained",
        "facility.field", "facility.meditate", "facility.alchemy",
    };
    return events;
}

}  // namespace

TEST(GuideTutorials, ChapterSevenHasAtLeastFourteenShortCardsEachWiredToAnEventThatFires) {
    const GameData& data = realData();
    const std::set<std::string> flags = registeredFlags();
    std::set<std::string> objectiveEvents;
    for (const Objective& step : data.objectives) objectiveEvents.insert("objective." + step.id);
    int chapterSeven = 0;
    for (const fanren::core::TutorialCard& card : data.tutorials) {
        EXPECT_EQ(card.minChapter, 7) << card.id;
        EXPECT_EQ(card.maxChapter, 7) << card.id;
        // 「演完到第 n 章」：第 n 章及以前都演完了。
        const auto upTo = [](int n) { return [n](int chapter) { return chapter <= n; }; };
        if (fanren::rules::tutorialInChapter(card, upTo(6))) ++chapterSeven;
        EXPECT_FALSE(fanren::rules::tutorialInChapter(card, upTo(5))) << card.id << " 会在第 6 章弹";
        EXPECT_FALSE(fanren::rules::tutorialInChapter(card, upTo(7))) << card.id << " 会在第 8 章弹";
        const std::string title = data.lookupText(card.titleKey);
        EXPECT_NE(title, card.titleKey) << card.id;
        EXPECT_LE(glyphs(title), 10u) << card.id;
        ASSERT_LE(card.bodyKeys.size(), 3u) << card.id;
        for (const std::string& key : card.bodyKeys) {
            const std::string line = data.lookupText(key);
            EXPECT_NE(line, key) << card.id;
            EXPECT_LE(glyphs(line), 30u) << card.id << "：" << line;
        }
        EXPECT_TRUE(knownEvents().count(card.trigger) != 0 || objectiveEvents.count(card.trigger) != 0)
            << card.id << " 挂的事件没人发：" << card.trigger;
        EXPECT_EQ(flags.count(fanren::rules::tutorialSeenFlag(card.id)), 1u) << card.id << " 的旗标没登记";
    }
    EXPECT_GE(chapterSeven, 14);
}

TEST(GuideTutorials, TheRuleNumbersOnTheCardsMatchTheEngine) {
    // 卡上的数字逐条对着引擎的常量与数据核过（docs/ch07-foundation-g.md 第 3 节那张表）。这里钉住几条
    // 最容易随平衡改动走散的：改了引擎忘了改卡，这一条会红。
    const GameData& data = realData();
    const auto body = [&data](const std::string& id) {
        const fanren::core::TutorialCard* card = fanren::rules::findTutorial(data.tutorials, id);
        std::string all;
        if (card != nullptr) {
            for (const std::string& key : card->bodyKeys) all += data.lookupText(key);
        }
        return all;
    };
    const fanren::core::Item* tulao = data.findItem("talisman_tulao_fu");
    ASSERT_NE(tulao, nullptr);
    const fanren::core::Magic* stagger = data.findMagic(tulao->castMagic);
    ASSERT_NE(stagger, nullptr);
    EXPECT_NE(body("item_cast").find("三点架势"), std::string::npos);
    const fanren::core::Item* jinchuang = data.findItem("pill_jinchuang_yao");
    if (jinchuang != nullptr) {
        EXPECT_EQ(jinchuang->restoreHp, 15);
        EXPECT_NE(body("field_item").find("十五"), std::string::npos);
    }
    EXPECT_EQ(fanren::game::bottleChargeDays(fanren::rules::Realm::QiRefining1), 7);
    EXPECT_NE(body("bottle").find("七天"), std::string::npos);
    EXPECT_NE(body("boost").find("五点"), std::string::npos);
    EXPECT_NE(body("boost").find("三点"), std::string::npos);
}

namespace {

class TutorialFlow : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(app_.init(assetRoot(), /*headless=*/true).ok);
        auto save = fanren::io::loadGame((fs::path(assetRoot()) / "saves" / "ch07-start.sav").string());
        ASSERT_TRUE(save.ok) << save.error;
        app_.state() = save.value;
        ASSERT_TRUE(app_.chapterTable().chapterDone(app_.state(), 6)) << "先验：章首存档第 6 章演完了";
        ASSERT_FALSE(app_.chapterTable().chapterDone(app_.state(), 7)) << "先验：第 7 章还没演完";
        ASSERT_TRUE(app_.loadMap(app_.state().mapId, "").ok);
        app_.pushScene(std::make_unique<fanren::game::WorldScene>());
        tick();
    }
    void TearDown() override { app_.shutdown(); }
    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }
    void press(SDL_Scancode scancode) {
        pushKey(scancode, true);
        tick();
        pushKey(scancode, false);
        tick();
    }
    std::string top() {
        const fanren::game::Scene* scene = app_.topScene();
        return scene == nullptr ? std::string{} : scene->name();
    }
    // 一张接一张地关掉此刻排着的卡，直到顶上不是卡片。
    void drainCards() {
        for (int guard = 0; guard < 40 && (top() == "Tutorial" || !app_.pendingTutorials().empty()); ++guard) {
            if (top() == "Tutorial") press(SDL_SCANCODE_RETURN);
            else tick();
        }
    }
    Application app_;
};

}  // namespace

TEST_F(TutorialFlow, OffByDefaultSoHeadlessRunsNeverSeeACard) {
    EXPECT_FALSE(app_.tutorialsEnabled());
    app_.tutorialEvent("facility.field");
    EXPECT_EQ(app_.requestTutorial("bottle"), "skipped");
    tick(3);
    EXPECT_TRUE(app_.pendingTutorials().empty());
    EXPECT_TRUE(app_.tutorialLog().empty());
    EXPECT_EQ(top(), "World");
    EXPECT_EQ(app_.state().flag("tutorial.seen.bottle"), 0) << "关着时不许多出旗标（章末状态逐字段一致）";
}

TEST_F(TutorialFlow, ACardPopsOnceOverTheWorldAndTheConfirmKeyClosesIt) {
    app_.setTutorialsEnabled(true);
    tick(2);
    // 章首站在百药园：「记事与地图」挂在走图上，第一张就是它。
    ASSERT_EQ(top(), "Tutorial");
    const auto* card = dynamic_cast<const fanren::game::TutorialScene*>(app_.topScene());
    ASSERT_NE(card, nullptr);
    EXPECT_EQ(card->cardId(), "keys");
    EXPECT_EQ(app_.state().flag("tutorial.seen.keys"), 1);
    EXPECT_FALSE(card->lines().empty());
    press(SDL_SCANCODE_RETURN);
    const auto* after = dynamic_cast<const fanren::game::TutorialScene*>(app_.topScene());
    EXPECT_TRUE(after == nullptr || after->cardId() != "keys") << "确认键没关掉卡片";
    drainCards();

    // 同一个事件再来：看过了，不再弹。
    const std::size_t shown = app_.tutorialLog().size();
    app_.tutorialEvent("world.idle");
    tick(2);
    EXPECT_EQ(std::count(app_.tutorialLog().begin(), app_.tutorialLog().end(), "keys"), 1);
    EXPECT_GE(app_.tutorialLog().size(), shown);
}

TEST_F(TutorialFlow, SwitchedOffInSettingsTheCardIsMarkedSeenButNeverShown) {
    fanren::io::Settings settings = app_.settings();
    settings.tutorialCards = false;
    app_.setSettings(settings);
    app_.setTutorialsEnabled(true);
    app_.tutorialEvent("facility.field");
    tick(3);
    EXPECT_EQ(top(), "World") << "关了教学提示还弹";
    EXPECT_EQ(app_.state().flag("tutorial.seen.bottle"), 1) << "关了也照样记作看过";
    EXPECT_EQ(app_.state().flag("tutorial.seen.keys"), 1);
    // 指南页照样列得出来。
    const auto seen = fanren::rules::seenTutorials(app_.data().tutorials, app_.state());
    EXPECT_TRUE(std::any_of(seen.begin(), seen.end(), [](const auto* c) { return c->id == "bottle"; }));
}

TEST_F(TutorialFlow, NoCardWhileADialogueIsUpItWaitsForTheSceneToEnd) {
    app_.setTutorialsEnabled(true);
    tick(2);
    drainCards();   // 先把「记事与地图」收掉
    ASSERT_EQ(top(), "World");
    app_.pushScene(std::make_unique<fanren::game::DialogueScene>(std::string{}, "一句话"));
    tick();
    ASSERT_EQ(top(), "Dialogue");
    app_.tutorialEvent("facility.meditate");
    tick(3);
    EXPECT_EQ(top(), "Dialogue") << "对白进行中弹了卡";
    EXPECT_EQ(app_.state().flag("tutorial.seen.meditate"), 0);
    app_.popScene();
    tick(2);
    EXPECT_EQ(top(), "Tutorial") << "对白收了、回到走图该弹了";
    EXPECT_EQ(app_.state().flag("tutorial.seen.meditate"), 1);
}

TEST_F(TutorialFlow, TheScriptCommandQueuesByIdAndReportsUnknownCards) {
    app_.setTutorialsEnabled(true);
    EXPECT_EQ(app_.requestTutorial("no_such_card"), "no_card");
    EXPECT_EQ(app_.requestTutorial("alchemy"), "");
    EXPECT_EQ(app_.pendingTutorials().back(), "alchemy");
    // 不在这一章的不弹：第 7 章演完了（章节表的 done_flag），卡就不弹了。
    const fanren::game::ChapterEntry* seven = app_.chapterTable().find(7);
    ASSERT_NE(seven, nullptr);
    app_.state().setFlag(seven->doneFlag);
    EXPECT_EQ(app_.requestTutorial("gift"), "skipped");
}

TEST_F(TutorialFlow, InBattleTheCardSitsOnTopOfTheBattleSoItsInputPauses) {
    app_.setTutorialsEnabled(true);
    tick(2);
    drainCards();
    // 一场打不完的仗不必真开：压一层不动的场景代替战斗不合适——flush 认的就是 BattleScene。
    // 这里只验「轮到我方开菜单发事件 → 卡片压在战斗上」的那一半：直接发事件、看卡片排在 battle 之上。
    app_.tutorialEvent("battle.command");
    ASSERT_FALSE(app_.pendingTutorials().empty());
    EXPECT_EQ(app_.pendingTutorials().back(), "weakness");
}

// ===========================================================================
// 4. 记事四页
// ===========================================================================

namespace {

class BoardPages : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(app_.init(assetRoot(), /*headless=*/true).ok);
        auto save = fanren::io::loadGame((fs::path(assetRoot()) / "saves" / "ch07-start.sav").string());
        ASSERT_TRUE(save.ok) << save.error;
        app_.state() = save.value;
        ASSERT_TRUE(app_.loadMap(app_.state().mapId, "").ok);
    }
    void TearDown() override { app_.shutdown(); }
    Application app_;
};

bool anyContains(const std::vector<std::string>& lines, const std::string& part) {
    return std::any_of(lines.begin(), lines.end(), [&part](const std::string& l) { return l.find(part) != std::string::npos; });
}

}  // namespace

TEST_F(BoardPages, FourPagesTurnWithLeftAndRightAndWrapAround) {
    BoardScene board;
    board.onEnter(app_);
    EXPECT_EQ(board.page(), BoardScene::Page::Main);
    const std::vector<std::string> main = board.paneLines(app_);
    ASSERT_FALSE(main.empty());
    // 主线页上半截：行动句（第 7 章第 1 步）、详情（原来的长句）、地点。
    EXPECT_TRUE(anyContains(main, app_.text("objective.ch07.n01_chaibao")));
    EXPECT_TRUE(anyContains(main, app_.text("objective.ch07.n01_chaibao.detail")));
    EXPECT_TRUE(anyContains(main, "地点"));
    for (BoardScene::Page expected : {BoardScene::Page::Side, BoardScene::Page::Guide, BoardScene::Page::Lore,
                                      BoardScene::Page::Main}) {
        board.turnPage(app_, 1);
        EXPECT_EQ(board.page(), expected);
    }
    board.turnPage(app_, -1);
    EXPECT_EQ(board.page(), BoardScene::Page::Lore);
}

TEST_F(BoardPages, TheSidePageWritesTheRewardsAndCallsMoneyByTheRightName) {
    // 造一条接了、写着奖励的支线：钱与一样东西。
    fanren::core::Quest quest;
    quest.id = "zz_probe";
    quest.kind = fanren::core::QuestKind::Side;
    quest.chapter = 7;
    quest.titleKey = "zz.title";
    quest.summaryKey = "zz.summary";
    quest.accept = {{fanren::core::QuestCondition::Op::FlagAtLeast, "zz.accept", 1}};
    quest.complete = {{fanren::core::QuestCondition::Op::FlagAtLeast, "zz.done", 1}};
    quest.rewards = {{"material_lingshi", 60, 0}, {"pill_zhuji_dan", 1, 0}};
    app_.data().text["zz.title"] = "探针";
    app_.data().text["zz.summary"] = "一件小事";
    app_.data().quests.push_back(quest);
    app_.state().setFlag("zz.accept");

    const std::string line = BoardScene::rewardLine(app_.data(), app_.state(), app_.data().quests.back());
    EXPECT_NE(line.find("奖励"), std::string::npos) << line;
    EXPECT_NE(line.find("灵石 × 60"), std::string::npos) << "第 7 章修仙阶段，钱叫灵石：" << line;
    EXPECT_NE(line.find(app_.data().findItem("pill_zhuji_dan")->name + " × 1"), std::string::npos) << line;
    // 凡人阶段（第 1–5 章）不许上屏「灵石」（G-25）。
    GameState mortal = app_.state();
    mortal.flags.erase("story.xiuxian_known");
    const std::string early = BoardScene::rewardLine(app_.data(), mortal, app_.data().quests.back());
    EXPECT_EQ(early.find("灵石"), std::string::npos) << early;
    EXPECT_NE(early.find("碎银 × 60"), std::string::npos) << early;

    BoardScene board(BoardScene::Page::Side);
    board.onEnter(app_);
    ASSERT_GT(board.list().count(), 0);
    // 光标停在探针那一条上（新接的进行中排在最前）。
    const std::vector<std::string> pane = board.paneLines(app_);
    EXPECT_TRUE(anyContains(pane, "灵石 × 60")) << ::testing::PrintToString(pane);
}

TEST_F(BoardPages, TheGuidePageListsTheCardsSeenAndTheLorePageShowsWeaknessesAndDrops) {
    app_.state().setFlag("tutorial.seen.boost");
    BoardScene guide(BoardScene::Page::Guide);
    guide.onEnter(app_);
    ASSERT_EQ(guide.list().count(), 1);
    EXPECT_EQ(guide.list().items()[0].label, app_.text("tutorial.boost.title"));
    EXPECT_TRUE(anyContains(guide.paneLines(app_), app_.text("tutorial.boost.l1")));

    // 见闻：进了禁地（ch07.pojin）寒冰蟾解锁；打过一回、看破了火，已知破绽写「火」，掉落写寒蟾酥。
    BoardScene lore(BoardScene::Page::Lore);
    lore.onEnter(app_);
    const int before = lore.list().count();
    app_.state().setFlag("ch07.pojin");
    app_.state().learnWeaknesses("hanbing_chan", fanren::core::kCategoryFire);
    lore.onEnter(app_);
    ASSERT_GT(lore.list().count(), before);
    std::vector<std::string> pane = lore.paneLines(app_);
    EXPECT_TRUE(anyContains(pane, "已知破绽　火")) << ::testing::PrintToString(pane);
    const fanren::core::Item* su = app_.data().findItem("material_hanchan_su");
    ASSERT_NE(su, nullptr);
    EXPECT_TRUE(anyContains(pane, "掉落　") && anyContains(pane, su->name)) << ::testing::PrintToString(pane);
}

// ===========================================================================
// 5. 见闻
// ===========================================================================

TEST(GuideLore, TheBeastEntriesAreWellFormedAndPointAtRealRolesAndFlags) {
    const GameData& data = realData();
    const std::set<std::string> flags = registeredFlags();
    int beasts = 0;
    for (const fanren::core::LoreEntry& entry : data.lore) {
        const auto& cats = fanren::io::loreCategories();
        EXPECT_NE(std::find(cats.begin(), cats.end(), entry.category), cats.end()) << entry.id;
        const std::string title = data.lookupText(entry.titleKey);
        const std::string body = data.lookupText(entry.bodyKey);
        EXPECT_NE(title, entry.titleKey) << entry.id;
        EXPECT_NE(body, entry.bodyKey) << entry.id;
        EXPECT_LE(glyphs(title), 10u) << entry.id;
        EXPECT_LE(glyphs(body), 120u) << entry.id;
        if (!entry.unlockFlag.empty()) EXPECT_EQ(flags.count(entry.unlockFlag), 1u) << entry.id;
        if (entry.category == "yaoshou") {
            ++beasts;
            EXPECT_NE(data.findRole(entry.roleId), nullptr) << entry.id << " 的 role_id 不存在";
        }
    }
    EXPECT_GE(beasts, 3);
}

TEST(GuideLore, NoticesQueueOneAfterAnotherAndFadeOut) {
    fanren::game::LoreNotices notices;
    notices.push("见闻：甲");
    notices.push("见闻：乙");
    EXPECT_EQ(notices.current(), "见闻：甲");
    notices.advance(fanren::game::LoreNotices::kFadeIn + 0.01f);
    EXPECT_FLOAT_EQ(notices.alpha(), 1.f);
    notices.advance(fanren::game::LoreNotices::kHold + fanren::game::LoreNotices::kFadeOut);
    EXPECT_EQ(notices.current(), "见闻：乙");
    notices.advance(10.f);
    EXPECT_FALSE(notices.active());
}

TEST(GuideLore, ANewlyUnlockedEntryIsAnnouncedButNotTheOnesKnownAtLoad) {
    Application app;
    ASSERT_TRUE(app.init(assetRoot(), /*headless=*/true).ok);
    auto save = fanren::io::loadGame((fs::path(assetRoot()) / "saves" / "ch07-start.sav").string());
    ASSERT_TRUE(save.ok);
    app.state() = save.value;
    app.state().setFlag("ch07.pojin");   // 读进来时就解锁了的：不报
    ASSERT_TRUE(app.loadMap(app.state().mapId, "").ok);
    app.pushScene(std::make_unique<fanren::game::WorldScene>());
    app.tick(1.0 / 60.0);
    EXPECT_FALSE(app.loreNotices().active()) << "读档时就有的见闻不该当成新的再报";
    app.state().setFlag("ch07.yixiantian");
    app.tick(1.0 / 60.0);
    ASSERT_TRUE(app.loreNotices().active());
    EXPECT_EQ(app.loreNotices().current(), "见闻：" + app.text("lore.ch07.fei_she.title"));
    app.shutdown();
}

// ===========================================================================
// 6. 头顶标记（2.6）与教学卡的「在不在这一章」
// ===========================================================================

TEST(GuideHeads, ShopKeepersAndSideQuestTargetsWearAMark) {
    Application app;
    ASSERT_TRUE(app.init(assetRoot(), /*headless=*/true).ok);
    auto save = fanren::io::loadGame((fs::path(assetRoot()) / "saves" / "ch07-start.sav").string());
    ASSERT_TRUE(save.ok);
    app.state() = save.value;

    // 坊市：药摊（kind=shop 的设施）旁边的摊主浮「商」；别人不浮。
    ASSERT_TRUE(app.loadMap("ch07_fangshi", "").ok);
    fanren::game::WorldScene world;
    const fanren::game::WorldFrame& market = world.describeFrame(app);
    int shopMarks = 0;
    for (const auto& npc : market.npcs) {
        if (npc.mark == app.text("ui.guide.mark.shop")) {
            ++shopMarks;
            EXPECT_FALSE(npc.markQuest);
        }
    }
    EXPECT_GE(shopMarks, 1) << "坊市药摊旁没有人浮「商」";

    // 环形山：接了「玉简」那条支线（Z2），两处触发区都浮「支」。
    app.state().setFlag("ch07.yujian_qiu");
    ASSERT_TRUE(app.loadMap("ch07_huanxingshan", "").ok);
    const fanren::game::WorldFrame& hill = world.describeFrame(app);
    std::set<std::string> marked;
    for (const auto* object : hill.questTargets) marked.insert(object->name);
    EXPECT_EQ(marked.count("trigger_yujian_dongpo"), 1u) << "当前那一步的触发区没浮「支」";
    EXPECT_EQ(hill.questMark, app.text("ui.guide.mark.quest"));
    // 做完第一处：标记挪到第二处。
    app.state().setFlag("ch07.yujian_a");
    const fanren::game::WorldFrame& later = world.describeFrame(app);
    marked.clear();
    for (const auto* object : later.questTargets) marked.insert(object->name);
    EXPECT_EQ(marked.count("trigger_yujian_dongpo"), 0u);
    EXPECT_EQ(marked.count("trigger_yujian_hantan"), 1u);
    app.shutdown();
}

TEST(GuideTutorials, ACardLivesBetweenTheEndOfTheChapterBeforeAndTheEndOfItsLastChapter) {
    fanren::core::TutorialCard card;
    card.minChapter = 7;
    card.maxChapter = 7;
    const auto upTo = [](int n) { return fanren::rules::ChapterDoneFn([n](int chapter) { return chapter <= n; }); };
    EXPECT_FALSE(fanren::rules::tutorialInChapter(card, upTo(5)));
    EXPECT_TRUE(fanren::rules::tutorialInChapter(card, upTo(6)));
    EXPECT_FALSE(fanren::rules::tutorialInChapter(card, upTo(7)));
    EXPECT_FALSE(fanren::rules::tutorialInChapter(card, nullptr)) << "没有章节表：当作哪一章都没演完，第 7 章的卡不弹";
    card.minChapter = 0;
    card.maxChapter = 0;
    EXPECT_TRUE(fanren::rules::tutorialInChapter(card, nullptr)) << "两头都不设限";
}

TEST(GuideBoard, ChapterSevenSideQuestsSayWhatTheyGive) {
    // G-25：两条第 7 章支线的回报是脚本按情形发的（不进 rewards），支线页照样写得出「奖励」那一行。
    const GameData& data = realData();
    GameState state;
    state.setFlag("story.xiuxian_known");
    for (const char* id : {"q07_baigong", "q07_yujian"}) {
        const auto it = std::find_if(data.quests.begin(), data.quests.end(),
                                     [id](const fanren::core::Quest& q) { return q.id == id; });
        ASSERT_NE(it, data.quests.end()) << id;
        EXPECT_TRUE(it->rewards.empty()) << id << "：回报照旧不进 rewards（Ch07SliceTests 钉着）";
        const std::string line = BoardScene::rewardLine(data, state, *it);
        EXPECT_NE(line.find("奖励"), std::string::npos) << id;
        EXPECT_EQ(line.find(it->rewardKey), std::string::npos) << id << " 的奖励文案没进 data/text：" << line;
    }
}
