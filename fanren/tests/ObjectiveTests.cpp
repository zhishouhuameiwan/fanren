// 目标链：选型规则、加载器、以及与真地图真脚本的对账。
//
// 这一批要解决的抱怨是「没有明确任务提示和任务地点提示」。做出来的东西错的方式
// 只有两种，而两种在画面上都表现为「目标系统等于没做」：
//
//   · **指到一个不存在的对象上** —— 屏幕上就是没有标记；
//   · **完成旗标没人会置** —— 那一步永远完不成，目标行从此卡死，后面的步骤
//     再也不会出现。
//
// 第二种尤其隐蔽：它在第一次游玩时看起来一切正常，直到那一步该翻过去而没翻。
// 所以下面既测纯函数（不开窗口就能跑），也拿**真的 data/ 与真的 maps/** 对账。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Objectives.h"
#include "game/Application.h"
#include "game/BoardScene.h"
#include "game/DialogueScene.h"
#include "game/WorldScene.h"
#include "io/DataLoader.h"
#include "ui/Widgets.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameData;
using fanren::core::GameState;
using fanren::core::Objective;
using fanren::core::mapDisplayNameKey;
using fanren::game::Application;
using fanren::game::WorldScene;
using fanren::rules::PortalLink;
using fanren::rules::completedObjectives;
using fanren::rules::currentObjective;
using fanren::rules::nextPortalToward;
using fanren::test::TempDir;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "data" / "objectives" / "ch01.json") &&
            fs::exists(root / "maps" / "ch01_hanjiacun.tmj")) {
            return candidate;
        }
    }
    return ".";
}

Objective step(std::string id, std::string flag) {
    Objective objective;
    objective.id = std::move(id);
    objective.textKey = "objective.test." + objective.id;
    objective.doneFlag = std::move(flag);
    objective.targetMap = "ch01_hanjiacun";
    objective.targetObject = "npc_han_sanshu";
    return objective;
}

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open()) << "无法创建夹具文件: " << path.string();
    out << content;
}

// ---------------------------------------------------------------------------
// 选型规则
// ---------------------------------------------------------------------------

TEST(Objectives, ProgressAdvancesOneStepAtATime) {
    const std::vector<Objective> chain{step("a", "f.a"), step("b", "f.b"), step("c", "f.c")};
    GameState state;

    ASSERT_NE(currentObjective(chain, state), nullptr);
    EXPECT_EQ(currentObjective(chain, state)->id, "a");

    state.flags["f.a"] = 1;
    ASSERT_NE(currentObjective(chain, state), nullptr);
    EXPECT_EQ(currentObjective(chain, state)->id, "b");

    state.flags["f.b"] = 1;
    ASSERT_NE(currentObjective(chain, state), nullptr);
    EXPECT_EQ(currentObjective(chain, state)->id, "c");

    state.flags["f.c"] = 1;
    EXPECT_EQ(currentObjective(chain, state), nullptr) << "三步都置位了，不该还有下一步";
}

TEST(Objectives, ACheckpointSaveResumesAtTheFurthestStepNotTheFirstGap) {
    // 检查点存档只带章末与本章前几节点的旗标，中间那些一个都没有
    //（tools/mksave 就是这么生成的，见 saves/README.md）。
    //
    // 按「第一条没做完的」算，读第 4 章检查点、人站在演武场上，目标行会写着
    // 第 1 章那句「在家中听三叔把话说完」并指着韩家村。这条测试钉的就是
    // 那个观感不许再出现。
    const std::vector<Objective> chain{step("a", "f.a"), step("b", "f.b"), step("c", "f.c"),
                                       step("d", "f.d")};
    GameState state;
    state.flags["f.a"] = 1;
    // b 这一步的旗标存档里没有——跳过去了。
    state.flags["f.c"] = 1;

    ASSERT_NE(currentObjective(chain, state), nullptr);
    EXPECT_EQ(currentObjective(chain, state)->id, "d") << "该接着走得最远的那一步往下";

    // 来路照实列：走过的是 a 与 c，中间 b 是个缺口，不替玩家编出来。
    const std::vector<const Objective*> done = completedObjectives(chain, state);
    ASSERT_EQ(done.size(), 2u);
    EXPECT_EQ(done[0]->id, "a");
    EXPECT_EQ(done[1]->id, "c");
}

TEST(Objectives, AStepWithNoDoneFlagNeverCountsAsFinished) {
    // 空 doneFlag 是数据写漏了（门禁那边不许，这里兜的是「数据绕过了门禁」）。
    // 它**永远不算做完**：算做完的话，一步没有任何完成判据的目标会把进度推
    // 过去，而玩家什么也没干。
    const std::vector<Objective> chain{step("a", ""), step("b", "f.b")};
    GameState state;
    ASSERT_NE(currentObjective(chain, state), nullptr);
    EXPECT_EQ(currentObjective(chain, state)->id, "a") << "它自己没做完，进度就停在这儿";
    EXPECT_TRUE(completedObjectives(chain, state).empty());

    state.flags["f.b"] = 1;
    // b 做完了，进度走到 b 之后；但 a 无论如何不会被列进「做过的事」。
    EXPECT_EQ(currentObjective(chain, state), nullptr);
    const std::vector<const Objective*> done = completedObjectives(chain, state);
    ASSERT_EQ(done.size(), 1u);
    EXPECT_EQ(done[0]->id, "b");
}

TEST(Objectives, CompletedNeverRunsAheadOfTheCurrentStep) {
    const std::vector<Objective> chain{step("a", "f.a"), step("b", "f.b"), step("c", "f.c")};
    GameState state;
    EXPECT_TRUE(completedObjectives(chain, state).empty());

    state.flags["f.a"] = 1;
    const std::vector<const Objective*> done = completedObjectives(chain, state);
    ASSERT_EQ(done.size(), 1u);
    EXPECT_EQ(done[0]->id, "a");
    // 当前这一步（b）不算「做过的事」：告示板上半截写它，下半截不该再列一遍。
    EXPECT_EQ(currentObjective(chain, state)->id, "b");
}

TEST(Objectives, AnEmptyChainHasNoCurrentStep) {
    const std::vector<Objective> chain;
    GameState state;
    EXPECT_EQ(currentObjective(chain, state), nullptr);
    EXPECT_TRUE(completedObjectives(chain, state).empty());
}

// ---------------------------------------------------------------------------
// 地图名 key 的推导（tools/validate.py 规则 21 守着它成立）
// ---------------------------------------------------------------------------

TEST(Objectives, MapNameKeyIsDerivedFromTheMapId) {
    EXPECT_EQ(mapDisplayNameKey("ch02_yaopu"), "ch02.map.yaopu.name");
    EXPECT_EQ(mapDisplayNameKey("ch01_liangu_ya"), "ch01.map.liangu_ya.name");
    // 推不出来的 id 回空串，让调用方原样显示 id——屏幕上看见 ch02_wairentang，
    // 比看见一个凭空拼出来、查不到的 key 更容易查。
    EXPECT_EQ(mapDisplayNameKey("nosuchmap"), "");
    EXPECT_EQ(mapDisplayNameKey(""), "");
    EXPECT_EQ(mapDisplayNameKey("ch01_"), "");
}

// ---------------------------------------------------------------------------
// 加载器
// ---------------------------------------------------------------------------

TEST(Objectives, TheLoaderRefusesAStepThatCannotPointAnywhere) {
    TempDir tmp{"fanren_objective_test"};
    const fs::path dataRoot = tmp.path() / "data";
    // target_object 漏了。收下它等于收下一步「画不出标记」的目标，
    // 而那在画面上与「目标系统没做」一模一样。
    writeFile(dataRoot / "objectives" / "ch01.json",
              R"({"id":"objectives_ch01","name":"测试链","chapter":1,"steps":[
                 {"id":"n1","text_key":"t","done_flag":"f","target_map":"m"}]})");

    const fanren::core::Result<GameData> result = fanren::io::loadGameData(dataRoot.string());
    EXPECT_FALSE(result.ok) << "缺 target_object 的目标链不该被收下";
    EXPECT_NE(result.error.find("target_object"), std::string::npos) << result.error;
}

TEST(Objectives, TheLoaderKeepsChapterOrderAndStepOrder) {
    TempDir tmp{"fanren_objective_test"};
    const fs::path dataRoot = tmp.path() / "data";
    // 故意让文件名的字典序与章号相反，确认排的是章号而不是文件名。
    writeFile(dataRoot / "objectives" / "zz_second.json",
              R"({"id":"o2","name":"第二章","chapter":2,"steps":[
                 {"id":"b1","text_key":"t","done_flag":"f","target_map":"m","target_object":"o"}]})");
    writeFile(dataRoot / "objectives" / "aa_first.json",
              R"({"id":"o1","name":"第一章","chapter":1,"steps":[
                 {"id":"a1","text_key":"t","done_flag":"f","target_map":"m","target_object":"o"},
                 {"id":"a2","text_key":"t","done_flag":"f","target_map":"m","target_object":"o"}]})");

    const fanren::core::Result<GameData> result = fanren::io::loadGameData(dataRoot.string());
    ASSERT_TRUE(result.ok) << result.error;
    ASSERT_EQ(result.value.objectives.size(), 3u);
    EXPECT_EQ(result.value.objectives[0].id, "a1");
    EXPECT_EQ(result.value.objectives[1].id, "a2");
    EXPECT_EQ(result.value.objectives[2].id, "b1");
    EXPECT_EQ(result.value.objectives[2].chapter, 2);
}

// ---------------------------------------------------------------------------
// 两块新面板的摆位
// ---------------------------------------------------------------------------

TEST(Objectives, ThePanelsDoNotHideRowsBehindThePageSize) {
    const fanren::ui::Theme theme;

    // 告示板要一屏装得下一章的来路：第 2 章十四步是眼下最长的一章。
    // 装不下不算错（可以翻页），但少于十行就说明面板高度或页大小算错了——
    // 而那种错的表现是「做过的事只列出前几条」，看起来像数据丢了。
    EXPECT_GE(fanren::game::BoardScene::listPageRows(theme), 10);

    // 回看同理：一屏十行以下的话，「回看」就变成了「翻页」。
    const int logRows = fanren::game::DialogueScene::logPageRows(theme);
    EXPECT_GE(logRows, 10);

    // 页大小不许超过这块区域真画得下的行数。超了就是「列表以为自己能画 N 行、
    // 实际只画得下 M 行」，多出来的那几行玩家得按方向键才找得到——而他多半
    // 不知道下面还有。这正是 P2 对话框漏显示选项的那个形状。
    const int contentH = fanren::game::DialogueScene::logPanelArea().h -
                         fanren::ui::panelTitleBandHeight(/*hasTitle=*/true, theme);
    EXPECT_LE(fanren::ui::listAreaHeight(logRows, theme), contentH);
}

// ---------------------------------------------------------------------------
// 与真数据对账
// ---------------------------------------------------------------------------

class ShippedObjectives : public ::testing::Test {
protected:
    void SetUp() override {
        auto loaded = fanren::io::loadGameData((fs::path(assetRoot()) / "data").string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        data_ = std::move(loaded.value);
        ASSERT_FALSE(data_.objectives.empty()) << "一条目标都没加载到，下面的断言全是空的";
    }

    GameData data_;
};

TEST_F(ShippedObjectives, EveryStepPointsAtSomethingThatExists) {
    // 与 tools/validate.py 规则 22 查的是同一件事，但走的是**引擎自己的加载器**：
    // 那边证明数据文件写得对，这边证明加载器真的把它读进来了。第 4 章那张
    // 「一个模块可用需要三样齐备」的表说的就是这个——规则层、加载器、游戏层入口
    // 缺一样，模块都是死的。
    for (const Objective& step : data_.objectives) {
        const fs::path mapPath =
            fs::path(assetRoot()) / "maps" / (step.targetMap + ".tmj");
        ASSERT_TRUE(fs::exists(mapPath)) << step.id << " 指向不存在的地图 " << step.targetMap;

        auto map = fanren::io::loadTileMap(mapPath.string());
        ASSERT_TRUE(map.ok) << mapPath.string() << "：" << map.error;
        const bool found = std::any_of(map.value.objects.begin(), map.value.objects.end(),
                                       [&step](const fanren::core::MapObject& object) {
                                           return object.name == step.targetObject;
                                       });
        EXPECT_TRUE(found) << step.id << " 指向 " << step.targetMap << " 上不存在的对象 "
                           << step.targetObject;

        // 文案缺了的话，屏幕上显示的是 key 本身——一行 objective.ch03.n7_beidu
        // 摆在「当前目标」那儿，比没有目标行更难看也更难查。
        EXPECT_NE(data_.lookupText(step.textKey), step.textKey)
            << step.id << " 的文案 " << step.textKey << " 不存在";
    }
}

TEST_F(ShippedObjectives, ChaptersRunInOrderAndCoverTheFinishedChapters) {
    int previous = 0;
    for (const Objective& step : data_.objectives) {
        EXPECT_GE(step.chapter, previous) << "章号倒退了：" << step.id;
        previous = std::max(previous, step.chapter);
    }
    // 已完成的四章每章都得有链。少一章，那一章的玩家就完全没有目标提示，
    // 而这条测试是唯一会喊出来的地方。
    for (int chapter = 1; chapter <= 4; ++chapter) {
        const bool has = std::any_of(data_.objectives.begin(), data_.objectives.end(),
                                     [chapter](const Objective& step) {
                                         return step.chapter == chapter;
                                     });
        EXPECT_TRUE(has) << "第 " << chapter << " 章没有任何目标";
    }
}

TEST_F(ShippedObjectives, WalkingTheRealChainAdvancesOneStepAtATime) {
    // 按真链逐个置位，每置一个，当前目标必须正好前进一步。
    //
    // 这一条钉的是「目标链与剧情推进是同一件事」：任何一步的 doneFlag 写成了
    // 别的旗标（比如写成同一个脚本里的选择旗标），这里就会一次跳过两步，或者
    // 卡住不动。而在游戏里，那种错要玩到那一章才看得见。
    GameState state;
    for (std::size_t i = 0; i < data_.objectives.size(); ++i) {
        const Objective* now = currentObjective(data_.objectives, state);
        ASSERT_NE(now, nullptr) << "第 " << i << " 步时目标链提前走完了";
        EXPECT_EQ(now->id, data_.objectives[i].id);
        EXPECT_EQ(completedObjectives(data_.objectives, state).size(), i);
        state.flags[data_.objectives[i].doneFlag] = 1;
    }
    EXPECT_EQ(currentObjective(data_.objectives, state), nullptr) << "全部置位后不该还有下一步";
}

// ---------------------------------------------------------------------------
// 跨图指路（第 4 章复验 N-7）
// ---------------------------------------------------------------------------
// 从前只高亮「直接通到目标图」的那一道门。第 3 章终局站在谷外，第 4 章第一步的
// 目标图是七玄门各堂，中间七张图一道门都不亮。现在按门的连通关系找下一道门。

PortalLink door(std::string from, std::string name, std::string to, std::string flag = "") {
    return PortalLink{std::move(from), std::move(name), fanren::core::Point{0, 0}, std::move(to),
                      std::move(flag)};
}

TEST(ObjectiveRouting, TheNextDoorIsTheFirstStepOfTheShortestRoute) {
    // A → B → C 两步；A → D → E → C 三步。该亮的是 A 上通往 B 的那一道。
    const std::vector<PortalLink> links{
        door("A", "a_to_d", "D"), door("D", "d_to_e", "E"), door("E", "e_to_c", "C"),
        door("A", "a_to_b", "B"), door("B", "b_to_c", "C"),
    };
    const GameState state;
    const PortalLink* hop = nextPortalToward(links, "A", "C", state);
    ASSERT_NE(hop, nullptr);
    EXPECT_EQ(hop->portalName, "a_to_b") << "取了一条更长的路";
    EXPECT_EQ(hop->fromMap, "A");

    // 同一张图、门的次序倒过来，答案不许变：最短就是最短。只按一种次序验的话，
    // 一份深度优先的实现恰好碰对次序也能绿（本批的变异 MRoute3 就是这么漏过去的）。
    const std::vector<PortalLink> reversed(links.rbegin(), links.rend());
    const PortalLink* again = nextPortalToward(reversed, "A", "C", state);
    ASSERT_NE(again, nullptr);
    EXPECT_EQ(again->portalName, "a_to_b") << "换了门的次序就换了路：那不是最短路";
}

// 真正的平局：两条一样长的路。契约（rules::nextPortalToward 的注释）写的是
// 「同样短的几条取链表里靠前的那一条」——同一份地图、同一份存档永远指同一道门。
// 上一条「次序倒过来答案不许变」管的是最短路唯一的情形；这一条管平手时由谁定。
TEST(ObjectiveRouting, OnATieTheDoorListedFirstWins) {
    // A → B → D 与 A → C → D，都是两步。
    std::vector<PortalLink> links{
        door("A", "a_to_b", "B"), door("A", "a_to_c", "C"),
        door("B", "b_to_d", "D"), door("C", "c_to_d", "D"),
    };
    const GameState state;
    const PortalLink* first = nextPortalToward(links, "A", "D", state);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->portalName, "a_to_b") << "平手时该取链表里靠前的那一支";

    // 把 A→B 与 A→C 在链表里对调，胜出的一支必须跟着换。只验上面那一半的话，
    // 一份按目标图 id 排序展开的实现（B 永远排在 C 前）也能绿。
    std::swap(links[0], links[1]);
    const PortalLink* swapped = nextPortalToward(links, "A", "D", state);
    ASSERT_NE(swapped, nullptr);
    EXPECT_EQ(swapped->portalName, "a_to_c") << "链表次序换了，平手的胜者却没跟着换";
}

// 读门的那一步不许把进程弄崩（C++ 审查 MEDIUM-1）。目录读不了就整个停用指路、留一句话；
// 一张坏图只跳过它自己。
TEST(ObjectiveRouting, AMapsFolderThatCannotBeReadTurnsGuidanceOffInsteadOfThrowing) {
    const fs::path missing = fanren::test::uniqueTempPath("fanren_no_maps_here");
    ASSERT_FALSE(fs::exists(missing)) << "先验：这个目录确实不存在";
    std::vector<std::string> problems;
    std::vector<PortalLink> links;
    EXPECT_NO_THROW(links = fanren::game::loadPortalLinks(missing.string(), problems))
        << "目录读不了就抛异常：世界层一画门，整个游戏就崩了";
    EXPECT_TRUE(links.empty()) << "目录读不了就不该指路";
    EXPECT_FALSE(problems.empty()) << "停用了指路却一句话也没留";

    // 对照组：真地图读得出门，也没有一句抱怨。少了这一半，一份永远返回空表的实现
    // 能让上面三条全绿。
    problems.clear();
    links = fanren::game::loadPortalLinks((fs::path(assetRoot()) / "maps").string(), problems);
    EXPECT_GT(links.size(), 10u);
    EXPECT_TRUE(problems.empty()) << (problems.empty() ? std::string{} : problems.front());
}

TEST(ObjectiveRouting, OneBrokenMapIsSkippedAndTheRestStillGuide) {
    TempDir tmp{"fanren_portal_maps"};
    const fs::path maps = tmp.path() / "maps";
    fs::create_directories(maps);
    std::error_code ec;
    fs::copy_file(fs::path(assetRoot()) / "maps" / "ch03_guwai.tmj", maps / "ch03_guwai.tmj", ec);
    ASSERT_FALSE(ec) << "复制夹具地图失败：" << ec.message();
    // 加载器要求外部 tileset 真的在（TileMapLoader 的规则），一并带过去。
    fs::create_directories(maps / "tilesets");
    fs::copy_file(fs::path(assetRoot()) / "maps" / "tilesets" / "terrain_guwai.tsj",
                  maps / "tilesets" / "terrain_guwai.tsj", ec);
    ASSERT_FALSE(ec) << "复制夹具 tileset 失败：" << ec.message();
    writeFile(maps / "zz_broken.tmj", "{ this is not json");

    std::vector<std::string> problems;
    const std::vector<PortalLink> links = fanren::game::loadPortalLinks(maps.string(), problems);
    ASSERT_EQ(problems.size(), 1u) << "坏图该留且只留一句";
    const bool kept = std::any_of(links.begin(), links.end(), [](const PortalLink& one) {
        return one.fromMap == "ch03_guwai" && one.portalName == "portal_to_shenshougu";
    });
    EXPECT_TRUE(kept) << "一张坏图把好图的门也一起弄丢了";
}

TEST(ObjectiveRouting, ALockedDoorIsNotARoute) {
    // 短的那条要穿过一道锁着的门：此刻不许指它，指了就是把人领到墙前面。
    const std::vector<PortalLink> links{
        door("A", "a_to_b", "B", "f.key"), door("B", "b_to_c", "C"),
        door("A", "a_to_d", "D"), door("D", "d_to_e", "E"), door("E", "e_to_c", "C"),
    };
    GameState state;
    const PortalLink* locked = nextPortalToward(links, "A", "C", state);
    ASSERT_NE(locked, nullptr);
    EXPECT_EQ(locked->portalName, "a_to_d") << "锁着的门被当成了路";

    // 对照组：钥匙到手之后，短的那条就该被指出来。少了这一半，一份「永远走远路」
    // 的实现也能让上面那条绿。
    state.flags["f.key"] = 1;
    const PortalLink* open = nextPortalToward(links, "A", "C", state);
    ASSERT_NE(open, nullptr);
    EXPECT_EQ(open->portalName, "a_to_b");
}

TEST(ObjectiveRouting, NoDoorLightsOnTheGoalMapOrWhenThereIsNoWay) {
    const std::vector<PortalLink> links{door("A", "a_to_b", "B"), door("C", "c_to_a", "A")};
    const GameState state;
    EXPECT_EQ(nextPortalToward(links, "A", "A", state), nullptr) << "目标就在脚下，不该指门";
    EXPECT_EQ(nextPortalToward(links, "A", "C", state), nullptr) << "走不到的地方不该瞎指";
    EXPECT_EQ(nextPortalToward(links, "", "B", state), nullptr);
    // 对照组：走得到的时候确实指得出来。
    EXPECT_NE(nextPortalToward(links, "A", "B", state), nullptr);
}

// ---- 真地图：从第 3 章终局出发，第 4 章第一步亮的是哪道门 ----

class ObjectiveRoute : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    // 第 3 章终局。位置取第 3 章通关测试的终局：谷外（docs/ch04-reverify.md 判据 1 那张表，
    // `ch03_guwai (19,25)`）；目标链走到第 3 章最后一步 n12c_ligu 的 ch03.done。
    void standAtChapterThreeEnding() {
        GameState& s = app_.state();
        s.setFlag("ch01.done");
        s.setFlag("ch01.muqin_bie");
        s.setFlag("ch02.done");
        s.setFlag("ch03.done");
        auto loaded = app_.loadMap("ch03_guwai", "");
        ASSERT_TRUE(loaded.ok) << loaded.error;
        s.position = fanren::core::Point{19, 25};
    }

    // 当前这张图上被描成目标色的那几道门（WorldScene::drawObjects 问的就是
    // portalLeadsToGoal）。
    std::vector<std::string> litPortals() {
        std::vector<std::string> lit;
        for (const fanren::core::MapObject& object : app_.currentMap()->objects) {
            if (object.type == "portal" && WorldScene::portalLeadsToGoal(app_, object)) {
                lit.push_back(object.name);
            }
        }
        return lit;
    }

    Application app_;
};

TEST_F(ObjectiveRoute, FromTheChapterThreeEndingTheValleyDoorLightsFirst) {
    standAtChapterThreeEnding();
    const Objective* step = currentObjective(app_.data().objectives, app_.state());
    ASSERT_NE(step, nullptr);
    ASSERT_EQ(step->id, "n1_liuxia") << "先验：第 3 章终局之后，当前目标是第 4 章第一步";
    ASSERT_EQ(step->targetMap, "ch04_getang") << "先验：那一步的目标图是七玄门各堂";
    ASSERT_NE(step->targetMap, app_.currentMap()->id) << "先验：目标不在脚下这张图上";

    // 修之前这里一道门也不亮：谷外两道门一道通神手谷、一道通暗道，都不直达各堂。
    const std::vector<std::string> lit = litPortals();
    ASSERT_EQ(lit.size(), 1u) << "谷外该亮且只亮一道门";
    EXPECT_EQ(lit.front(), "portal_to_shenshougu") << "从谷外去七玄门各堂，第一道门是回神手谷那一道";

    const fanren::rules::PortalLink* hop = WorldScene::guidePortal(app_);
    ASSERT_NE(hop, nullptr);
    EXPECT_EQ(hop->targetMap, "ch01_shenshougu");

    // 左上角那一行也要说出先去哪：目标文字（ch04_objectives）不提韩家村，
    // 那是文案那一路的事；引擎这一路至少把下一站写出来。
    const std::string where = WorldScene::objectiveWhereText(app_);
    const std::string nextName = app_.text(mapDisplayNameKey("ch01_shenshougu"));
    ASSERT_NE(nextName, mapDisplayNameKey("ch01_shenshougu")) << "先验：神手谷的地名有文案";
    EXPECT_NE(where.find("先到"), std::string::npos) << "目标行没说先去哪：" << where;
    EXPECT_NE(where.find(nextName), std::string::npos) << "目标行说的下一站不是神手谷：" << where;
}

TEST_F(ObjectiveRoute, FollowingTheLitDoorsLeadsAllTheWayToTheSects) {
    // 一路跟着亮着的门走，每张图都正好亮一道，最后落在七玄门各堂。
    // 路线是地理事实：谷外 → 神手谷 → 炼骨崖 → 七玄门 → 彩霞山 → 青牛镇 → 韩家村
    // → 山下镇 → 各堂（复验 N-7 说的「谷外到山下镇之间六张图」加上两头）。
    standAtChapterThreeEnding();
    const std::vector<std::string> expected{
        "ch01_shenshougu", "ch01_liangu_ya", "ch01_qixuanmen",   "ch01_caixiashan",
        "ch01_qingniuzhen", "ch01_hanjiacun", "ch04_shanxiazhen", "ch04_getang",
    };

    std::vector<std::string> walked;
    for (std::size_t guard = 0; guard < expected.size() + 2; ++guard) {
        if (app_.currentMap()->id == "ch04_getang") break;
        const std::string here = app_.currentMap()->id;
        ASSERT_EQ(litPortals().size(), 1u) << here << " 上该亮且只亮一道门";
        const fanren::rules::PortalLink* hop = WorldScene::guidePortal(app_);
        ASSERT_NE(hop, nullptr) << here << " 上指不出下一道门";
        ASSERT_EQ(hop->fromMap, here);
        const std::string next = hop->targetMap;
        walked.push_back(next);
        auto loaded = app_.loadMap(next, "");
        ASSERT_TRUE(loaded.ok) << loaded.error;
    }
    EXPECT_EQ(walked, expected);

    // 到了各堂，目标就在脚下：不再亮门，左上角也不再报地名。
    EXPECT_TRUE(litPortals().empty()) << "到了目标图还在亮门";
    EXPECT_TRUE(WorldScene::objectiveWhereText(app_).empty());
}

TEST_F(ObjectiveRoute, OtherOpenDoorsDoNotChangeTheRoute) {
    // 真的第 3 章终局比上面那几个旗标多得多（复验判据 1 那张表：23 个 ch02/ch03 旗标），
    // 多开的门会不会让「最短路」拐到别处去？把前三章地图上**每一道门的钥匙都给他**，
    // 从谷外出发仍该是同一道门。
    standAtChapterThreeEnding();
    // 只给第 1-3 章的钥匙：第 4 章的门（ch04.liuxia、ch04.yufeng_xue）要本章演过才开，
    // 第 3 章终局手上不可能有；给了还会顺手把第 4 章前几步标成已完成。
    // **按白名单放行，不按「不是第 4 章」排除**：从前写成排除 ch04.，第 5 章落地后
    // ch05.zhuishao / ch05.dengmen 这两把钥匙（同时是第 5 章目标链的完成旗标）也被送了出去，
    // 目标一下跳到第 5 章，先验当场不成立。以后每加一章，排除法都会再漏一次。
    const auto isChapterOneToThreeKey = [](const std::string& flag) {
        return flag.rfind("ch01.", 0) == 0 || flag.rfind("ch02.", 0) == 0 ||
               flag.rfind("ch03.", 0) == 0;
    };
    for (const PortalLink& any : app_.portalLinks()) {
        if (!isChapterOneToThreeKey(any.requireFlag)) continue;
        app_.state().setFlag(any.requireFlag);
    }
    ASSERT_GT(app_.portalLinks().size(), 10u) << "先验：真的读到了全部地图的门";
    // 先验：目标仍是第 4 章第一步。给钥匙时可能顺手把某一步的完成旗标也置了。
    const Objective* step = currentObjective(app_.data().objectives, app_.state());
    ASSERT_NE(step, nullptr);
    ASSERT_EQ(step->targetMap, "ch04_getang") << "先验：目标没被钥匙推走（当前 " << step->id << "）";

    const std::vector<std::string> lit = litPortals();
    ASSERT_EQ(lit.size(), 1u);
    EXPECT_EQ(lit.front(), "portal_to_shenshougu");
}

// ---------------------------------------------------------------------------
// 每一步都走得到（第 1 章「报名 / 山道」次序事故，独立审查 M3）
// ---------------------------------------------------------------------------
// 第 1 章目标链从前把「山道遇张铁」排在「青牛镇报名」前面，可地图是先报名、青牛镇通山道的
// 门才开（portal_to_caixiashan 的 require_flag=ch01.baoming_done）。话别之后目标指着彩霞山道，
// 指路只走开着的门，于是韩家村、青牛镇一道门都不亮；报了名，进度按「走得最远的那一步」往后
// 一跳，「到青牛镇报名处登名」那一行从没露过面。目标链单看对、门闸单看也对，错在两者之间——
// 之前没有一条检查把目标链放到地图的门闸上走一遍。
//
// 判据：从空存档起按真目标链逐步推进，每一步站在「做完上一步时人在的那张图」上，跟着
// WorldScene::guidePortal（世界层描门、左上角报「先到」问的都是它）一道门一道门地走，必须走到
// 这一步的目标图。
//   · 起点：第一步是开新局那张图（kNewGameMap）；之后是上一步的目标图——完成旗标就是那张图上的
//     脚本置的。脚本演完把人传送走的几步例外（kScriptTransfers），起点是传送的落点。
//   · 门开不开只看此刻的旗标：目标链上已做完的各步 done_flag，加上门钥匙里不在链上的那几把
//    （kOffChainDoorKeys，按表插在链上）。
//   · 两张表都拿脚本原文核（TheTransferAndDoorKeyTablesMatchTheScripts）：表里每一条对得上脚本；
//     反过来，置了目标链 done_flag 又 teleport 的脚本、不在链上的门钥匙，也都必须在表里。漏登一处，
//     起点或旗标就算错了，算错的那一步可能恰好有路——于是假绿。
// 走的是整条链（眼下第 1–5 章），新章的链落地就一起走。

// 脚本演完把人传送走的那几步：下一步的起点是 toMap，不是这一步的目标图。
struct ScriptTransfer {
    const char* doneFlag;  // 目标链上那一步的 done_flag
    const char* script;    // 置它、并 teleport 的脚本，相对仓库根
    const char* toMap;     // teleport 的目标图
};

constexpr ScriptTransfer kScriptTransfers[] = {
    {"ch05.shangchuan", "scripts/ch05/shangchuan.lua", "ch05_xicheng"},       // 上船，水路到西城
    {"ch05.yeru", "scripts/ch05/yeru.lua", "ch05_mofu"},                       // 夜里翻进墨府后园
    {"ch05.jianmianli", "scripts/ch05/jianmian.lua", "ch05_mofu"},             // 见面礼之后落在府里
    {"ch05.chuzheng", "scripts/ch05/zhuwu.lua", "ch05_dubashanzhuang"},        // 骑马十日到山庄外
    {"ch05.cisha", "scripts/ch05/shangyue.lua", "ch05_mofu"},                  // 得手，骑马回墨府
};

// 门的钥匙里不在目标链上的：哪个脚本置它、紧跟在链上哪一步之后到手。
// 与 NpcPresenceTests 的 kOffChainFlags 里 ch01.done 那一条是同一件事实：那边管 NPC 的在场旗标，
// 这边管门。
struct OffChainDoorKey {
    const char* flag;
    const char* after;   // 目标链上这一步的 done_flag 置位之后，这把钥匙跟着到手
    const char* script;  // 两个旗标都由它置
};

constexpr OffChainDoorKey kOffChainDoorKeys[] = {
    // 第 1 章收尾：shenshougu_koujue.lua 先置 ch01.koujue_received，紧跟着置它；神手谷北口（去药圃）挂着它。
    {"ch01.done", "ch01.koujue_received", "scripts/ch01/shenshougu_koujue.lua"},
};

std::string readRepoText(const std::string& relative) {
    std::ifstream in(fs::path(assetRoot()) / relative, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        out.push_back(line);
    }
    return out;
}

// 脚本的活代码：每行从第一个 -- 起算注释、去掉（与 NpcPresenceTests 的 luaCodeOf 同一个口径）。
std::string liveCodeOf(const std::string& text) {
    std::string code;
    for (const std::string& line : lines(text)) code += line.substr(0, line.find("--")) + "\n";
    return code;
}

// 脚本里有没有一句顶格的 statement：这一行从第 0 列起就是它，前面没有缩进、也没有同一行的条件。
// 缩在分支里的 teleport / flag.set 只在某一支上发生，表里的「起点」「钥匙」就说不准了。
bool hasTopLevel(const std::string& text, const std::string& statement) {
    for (const std::string& line : lines(text)) {
        if (line.rfind(statement, 0) == 0) return true;
    }
    return false;
}

std::string flagSetOf(const std::string& flag) { return "flag.set(\"" + flag + "\""; }

std::string joined(const std::vector<std::string>& items, const char* separator) {
    std::string out;
    for (const std::string& item : items) out += (out.empty() ? "" : separator) + item;
    return out;
}

// 从 from 出发、只穿此刻开着的门走得到的全部图——报错时说清楚人被关在哪几张图里。
std::set<std::string> openlyReachable(const std::vector<PortalLink>& links, const std::string& from,
                                      const GameState& state) {
    std::set<std::string> seen{from};
    std::vector<std::string> frontier{from};
    while (!frontier.empty()) {
        const std::string here = frontier.back();
        frontier.pop_back();
        for (const PortalLink& link : links) {
            if (link.fromMap != here || !fanren::rules::portalOpen(link, state)) continue;
            if (seen.insert(link.targetMap).second) frontier.push_back(link.targetMap);
        }
    }
    return seen;
}

// 从 app 当前那张图起，跟着 guidePortal 一道门一道门走到 goal。走到了返回空串，否则是卡住时的说明。
std::string followLitDoorsTo(Application& app, const std::string& goal) {
    std::set<std::string> passed;
    for (std::string here = app.currentMap()->id; here != goal; here = app.currentMap()->id) {
        if (!passed.insert(here).second) return "指路在 " + here + " 兜回了走过的图";
        const PortalLink* hop = WorldScene::guidePortal(app);
        if (hop == nullptr) {
            const std::vector<PortalLink>& links = app.portalLinks();
            const std::set<std::string> pen = openlyReachable(links, here, app.state());
            std::vector<std::string> closed;
            for (const PortalLink& link : links) {
                if (pen.count(link.fromMap) == 0 || fanren::rules::portalOpen(link, app.state())) continue;
                closed.push_back(link.fromMap + "/" + link.portalName + " → " + link.targetMap + "（差 " +
                                 link.requireFlag + "）");
            }
            return "在 " + here + " 上一道门也不亮。此刻走得到的图：" +
                   joined(std::vector<std::string>(pen.begin(), pen.end()), "、") + "；关着的门：" +
                   (closed.empty() ? std::string("（没有）") : joined(closed, "，"));
        }
        auto loaded = app.loadMap(hop->targetMap, "");
        if (!loaded.ok) return "穿过 " + here + "/" + hop->portalName + " 载图失败：" + loaded.error;
    }
    return {};
}

// 目标链上走不到的一步。
struct RouteGap {
    std::string step;    // 「第 N 章 <id>」
    std::string detail;  // 从哪去哪、卡在哪、关着的门各差哪把钥匙
};

// 从空存档起把 app.data().objectives 整条走一遍，返回走不到的那几步（空 = 每一步都走得到）。
// app 的存档状态会被换掉。走不到的那一步照样置上它的 done_flag 往下走：只报它自己，
// 不让后面的步骤跟着连片报红。
std::vector<RouteGap> walkObjectiveChain(Application& app) {
    const std::vector<Objective>& chain = app.data().objectives;
    std::vector<RouteGap> gaps;
    app.state() = GameState{};
    std::string at = fanren::game::kNewGameMap;
    for (const Objective& step : chain) {
        const std::string label = "第 " + std::to_string(step.chapter) + " 章 " + step.id;
        const Objective* now = currentObjective(chain, app.state());
        if (now != &step) {
            gaps.push_back({label, "目标行此刻不是这一步（是 " + (now != nullptr ? now->id : std::string("空")) +
                                       "）——链上的 done_flag 重了或空了"});
        } else if (auto loaded = app.loadMap(at, ""); !loaded.ok) {
            gaps.push_back({label, "载起点图 " + at + " 失败：" + loaded.error});
        } else if (const std::string stuck = followLitDoorsTo(app, step.targetMap); !stuck.empty()) {
            gaps.push_back({label, "从 " + at + " 去 " + step.targetMap + "：" + stuck});
        }

        app.state().setFlag(step.doneFlag, 1);
        for (const OffChainDoorKey& key : kOffChainDoorKeys) {
            if (step.doneFlag == key.after) app.state().setFlag(key.flag, 1);
        }
        at = step.targetMap;
        for (const ScriptTransfer& transfer : kScriptTransfers) {
            if (step.doneFlag == transfer.doneFlag) at = transfer.toMap;
        }
    }
    return gaps;
}

std::string describeGaps(const std::vector<RouteGap>& gaps) {
    std::string out;
    for (const RouteGap& gap : gaps) out += "\n  · " + gap.step + "：" + gap.detail;
    return out;
}

TEST_F(ObjectiveRoute, EveryObjectiveStepCanBeReachedThroughTheDoorsOpenAtThatStep) {
    // 先验：链真的覆盖到了已完成的五章。少一章，那一章就没被走到，这里却照样绿。
    for (int chapter = 1; chapter <= 5; ++chapter) {
        const bool has = std::any_of(app_.data().objectives.begin(), app_.data().objectives.end(),
                                     [chapter](const Objective& step) { return step.chapter == chapter; });
        ASSERT_TRUE(has) << "第 " << chapter << " 章没有目标链，走不到它";
    }

    const std::vector<RouteGap> gaps = walkObjectiveChain(app_);
    EXPECT_TRUE(gaps.empty()) << "目标链上有 " << gaps.size()
                              << " 步跟着亮着的门走不到（目标说去那儿，一道门也不亮）：" << describeGaps(gaps);
}

TEST_F(ObjectiveRoute, TheRouteWalkCatchesTheOldChapterOneOrder) {
    // 判据先证明自己有牙：把第 1 章「山道遇张铁」挪回「青牛镇报名」前面（修之前的次序），
    // 同一个判据必须报出、而且只报出山道那一步——卡在韩家村，说得出青牛镇北口差的是哪把钥匙。
    std::vector<Objective>& chain = app_.data().objectives;
    const auto stepSetting = [&chain](const std::string& flag) {
        return std::find_if(chain.begin(), chain.end(),
                            [&flag](const Objective& step) { return step.doneFlag == flag; });
    };
    const auto shandao = stepSetting("ch01.zhangtie_met");
    const auto baoming = stepSetting("ch01.baoming_done");
    ASSERT_NE(shandao, chain.end()) << "先验：链上有山道遇张铁那一步";
    ASSERT_NE(baoming, chain.end()) << "先验：链上有青牛镇报名那一步";
    const std::string shandaoId = shandao->id;
    if (baoming < shandao) std::rotate(baoming, shandao, shandao + 1);   // 山道挪到报名之前

    const std::vector<RouteGap> gaps = walkObjectiveChain(app_);
    ASSERT_EQ(gaps.size(), 1u) << "旧次序该报且只报一步：" << describeGaps(gaps);
    EXPECT_EQ(gaps.front().step, "第 1 章 " + shandaoId);
    EXPECT_NE(gaps.front().detail.find("从 ch01_hanjiacun 去 ch01_caixiashan"), std::string::npos)
        << gaps.front().detail;
    EXPECT_NE(gaps.front().detail.find("portal_to_caixiashan"), std::string::npos) << gaps.front().detail;
    EXPECT_NE(gaps.front().detail.find("ch01.baoming_done"), std::string::npos) << gaps.front().detail;
}

// 表 → 脚本：两张表每一条对得上脚本原文——传送那一步由表里那个脚本顶格置 done_flag、顶格 teleport
// 到表里那张图；门钥匙与它跟着的那一步都由表里那个脚本顶格置。返回问题清单，空 = 对得上。
std::vector<std::string> tableRowProblems(const std::set<std::string>& doneFlags) {
    std::vector<std::string> problems;
    const auto scriptText = [&problems](const std::string& script) {
        std::string text = readRepoText(script);
        if (text.empty()) problems.push_back(script + " 读不到（路径写错了，或文件是空的）");
        return text;
    };
    const auto requireStep = [&problems, &doneFlags](const std::string& flag) {
        if (doneFlags.count(flag) == 0) problems.push_back(flag + " 不是目标链上任何一步的 done_flag");
    };
    for (const ScriptTransfer& transfer : kScriptTransfers) {
        const std::string script = transfer.script;
        const std::string text = scriptText(script);
        requireStep(transfer.doneFlag);
        if (!hasTopLevel(text, flagSetOf(transfer.doneFlag))) {
            problems.push_back(script + " 里没有顶格的 " + flagSetOf(transfer.doneFlag));
        }
        if (!hasTopLevel(text, "teleport(\"" + std::string(transfer.toMap) + "\"")) {
            problems.push_back(script + " 里没有顶格的 teleport(\"" + transfer.toMap + "\"——落点对不上表");
        }
    }
    for (const OffChainDoorKey& key : kOffChainDoorKeys) {
        const std::string script = key.script;
        const std::string text = scriptText(script);
        requireStep(key.after);
        if (doneFlags.count(key.flag) != 0) {
            problems.push_back(std::string(key.flag) + " 已是目标链上的 done_flag，不该再登进 kOffChainDoorKeys");
        }
        for (const char* flag : {key.after, key.flag}) {
            if (!hasTopLevel(text, flagSetOf(flag))) problems.push_back(script + " 里没有顶格的 " + flagSetOf(flag));
        }
    }
    return problems;
}

// 脚本 → 表：scripts/ 下置了目标链某一步 done_flag、活代码里又 teleport 的脚本。
struct ScriptScan {
    std::set<std::string> teleporting;  // 相对仓库根
    std::size_t scanned = 0;            // 一共读了几个脚本
};

ScriptScan scanStepScripts(const std::set<std::string>& doneFlags) {
    ScriptScan scan;
    const fs::path scripts = fs::path(assetRoot()) / "scripts";
    for (const auto& entry : fs::recursive_directory_iterator(scripts)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") continue;
        ++scan.scanned;
        const std::string relative = "scripts/" + fs::relative(entry.path(), scripts).generic_string();
        const std::string code = liveCodeOf(readRepoText(relative));
        const bool setsAStep = std::any_of(doneFlags.begin(), doneFlags.end(), [&code](const std::string& flag) {
            return code.find(flagSetOf(flag)) != std::string::npos;
        });
        if (setsAStep && code.find("teleport(") != std::string::npos) scan.teleporting.insert(relative);
    }
    return scan;
}

TEST_F(ObjectiveRoute, TheTransferAndDoorKeyTablesMatchTheScripts) {
    std::set<std::string> doneFlags;
    for (const Objective& step : app_.data().objectives) doneFlags.insert(step.doneFlag);

    const std::vector<std::string> problems = tableRowProblems(doneFlags);
    EXPECT_TRUE(problems.empty()) << joined(problems, "\n");

    // 反过来，该登的都登了——拿扫出来的集合与表**比相等**。真数据里两个方向的对照都有：
    // 第 5 章 5 个传送脚本必须被扫出来（扫不出来就不相等）；ch03/ligu.lua、ch04/huicun.lua、
    // ch04/dongqu.lua 置着链上的 done_flag、只在注释里写了「不调 teleport()」，必须扫不出来。
    std::set<std::string> offChainKeys;
    for (const PortalLink& link : app_.portalLinks()) {
        if (!link.requireFlag.empty() && doneFlags.count(link.requireFlag) == 0) offChainKeys.insert(link.requireFlag);
    }
    std::set<std::string> tabledKeys;
    for (const OffChainDoorKey& key : kOffChainDoorKeys) tabledKeys.insert(key.flag);
    EXPECT_EQ(offChainKeys, tabledKeys)
        << "门的钥匙里不在目标链上的，与 kOffChainDoorKeys 对不上：没登的那把，这条时间线上那道门永远关着；"
           "多登的那一条，是表过期了";

    const ScriptScan scan = scanStepScripts(doneFlags);
    ASSERT_GT(scan.scanned, 50u) << "先验：真的读到了 scripts/ 下的脚本";
    std::set<std::string> tabledTransfers;
    for (const ScriptTransfer& transfer : kScriptTransfers) tabledTransfers.insert(transfer.script);
    EXPECT_EQ(scan.teleporting, tabledTransfers)
        << "置了目标链 done_flag、又 teleport 的脚本与 kScriptTransfers 对不上：没登的那个，演完人不在目标图上，"
           "下一步的起点就算错了";
}

}  // namespace
