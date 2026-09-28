// 路径行动的游戏层（契约 docs/interfaces-octo-pathactions.md 第 5 节）：无头下把菜单、效果施加、
// 对话口径走一遍。
//
// 规则层自己的判定（挂不挂、做不做得成、清单长什么样）在 tests/PathActionTests.cpp；这里只问
// 「游戏层照着规则层的话做了没有」——旗标置没置、东西给没给、钱扣没扣、说的是哪一句、谁说的。
// 用的是真数据（第 2 章的三条），ids 写死：换了条目这里就该红，那是有人得来看一眼的时候。
//
// 菜单走 PathActionScene 的逻辑动作（choose / answer），不经按键：与 ListView 同一个思路，
// update 收到按键调的也是这两个，被测的那条路就是上线的那条路。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/PathActionScene.h"
#include "game/ShopScene.h"
#include "game/WorldScene.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::PathAction;
using fanren::game::Application;
using fanren::game::PathActionScene;
using fanren::game::PathBubble;
using fanren::rules::Realm;
using Phase = PathActionScene::Phase;

constexpr double kFrame = 1.0 / 60.0;

fs::path findProjectRoot() {
    fs::path dir = fs::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        if (fs::exists(dir / "scripts" / "common" / "api.lua") && fs::is_directory(dir / "data")) {
            return dir;
        }
        const fs::path parent = dir.parent_path();
        if (parent.empty() || parent == dir) break;
        dir = parent;
    }
    return {};
}

// 境界名的词表：与 PathActionTests 的 RefusalsNeverNameARealm 同一张（存档里每一个境界的名字、
// 大境界名、功法名、「第 N 层」）。
bool namesARealm(const std::string& text) {
    std::vector<std::string> names = {"炼气", "筑基", "结丹", "长春功"};
    for (int v : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 21, 22, 23, 31, 32, 33}) {
        names.emplace_back(fanren::rules::nameOf(fanren::rules::fromValue(v)));
    }
    for (const char* n : {"一", "二", "三", "四", "五", "六", "七", "八", "九", "十", "十一", "十二", "十三"}) {
        names.push_back(std::string("第") + n + "层");
    }
    return std::any_of(names.begin(), names.end(),
                       [&text](const std::string& name) { return text.find(name) != std::string::npos; });
}

class PathActionFlow : public ::testing::Test {
protected:
    void SetUp() override {
        const fs::path root = findProjectRoot();
        ASSERT_FALSE(root.empty()) << "找不到工程根目录（应含 scripts/common/api.lua 与 data/）";
        auto ready = app_.init(root.string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    void enter(const std::string& mapId) {
        auto loaded = app_.loadMap(mapId, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        app_.pushScene(std::make_unique<fanren::game::WorldScene>());
        app_.tick(kFrame);
        ASSERT_EQ(top(), "World");
    }

    std::string top() {
        return app_.topScene() == nullptr ? std::string("<空>") : app_.topScene()->name();
    }

    const PathAction& entry(int chapter, const std::string& id) {
        static const PathAction missing;
        const auto& all = app_.data().pathActions;
        const auto it = std::find_if(all.begin(), all.end(), [&](const PathAction& a) {
            return a.chapter == chapter && a.id == id;
        });
        if (it != all.end()) return *it;
        ADD_FAILURE() << "真数据里没有第 " << chapter << " 章的 " << id;
        return missing;
    }

    // 按 E 的那一下：压菜单、走一帧让它上栈。没压（一条都没挂着）返回 nullptr。
    PathActionScene* open(const std::string& npc) {
        if (!app_.openPathActions(npc)) return nullptr;
        app_.tick(kFrame);
        return dynamic_cast<PathActionScene*>(app_.topScene());
    }

    // 在菜单里选 id 那一行。菜单上没有这一行返回 false（调用方 ASSERT 它）。
    bool pick(PathActionScene& scene, const std::string& id) {
        const auto& rows = scene.rows();
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i]->id != id) continue;
            scene.choose(app_, static_cast<int>(i));
            return true;
        }
        return false;
    }

    bool listed(const std::string& npc, const std::string& id) {
        const auto now = app_.pathActionsFor(npc);
        return std::any_of(now.begin(), now.end(), [&](const PathAction* a) { return a->id == id; });
    }

    // 替玩家把这一轮话说完：先走一帧让刚压的对话框上栈，再一个个收掉（替玩家按确认），
    // 最后再走一帧——收起了的菜单要等它自己的下一次 update 才退场。返回收了几个对话框。
    int finishTalking() {
        app_.tick(kFrame);
        int dialogues = 0;
        while (top() == "Dialogue" && dialogues < 16) {
            app_.popScene();
            app_.tick(kFrame);
            ++dialogues;
        }
        app_.tick(kFrame);
        return dialogues;
    }

    int money() { return fanren::game::spiritStones(app_.state()); }

    Application app_;
};

// 打探：说情报（说话人是那个 NPC 的 role，进对话回看）、头一回给东西记做过；
// 再开菜单那一条仍在，重说一遍，东西不再给。
TEST_F(PathActionFlow, AnInquirySpeaksGivesOnceAndStaysUpForRereading) {
    enter("ch02_jusuo");
    app_.state().setFlag("ch02.duan3_start");
    const PathAction& a = entry(2, "zhangtie_dating2");
    ASSERT_EQ(a.gives.size(), 1u) << "先验：这一条头一回给东西";
    const std::string herb = a.gives[0].itemId;
    const int age = a.gives[0].herbAge;
    const int before = app_.state().itemCountOfAge(herb, age);

    PathActionScene* scene = open("npc_zhang_tie");
    ASSERT_NE(scene, nullptr) << "张铁身上挂着 zhangtie_dating2，按 E 该有菜单";
    ASSERT_TRUE(pick(*scene, "zhangtie_dating2"));
    EXPECT_EQ(finishTalking(), 1) << "打探只说一句：那段情报";
    EXPECT_EQ(top(), "World") << "话说完菜单收起";
    EXPECT_EQ(app_.lastSpokenKey(), a.textKey);
    ASSERT_FALSE(app_.dialogueLog().empty());
    EXPECT_EQ(app_.dialogueLog().back().speaker, app_.speakerName("zhang_tie"))
        << "说话人取 npc 对象的 role_id，与脚本 talk 同一个口径";
    EXPECT_EQ(app_.dialogueLog().back().body, app_.text(a.textKey));
    EXPECT_EQ(app_.state().flag(a.doneFlag), 1);
    EXPECT_EQ(app_.state().itemCountOfAge(herb, age), before + a.gives[0].count);

    scene = open("npc_zhang_tie");
    ASSERT_NE(scene, nullptr) << "打探做过仍挂着（契约第 0 节）";
    app_.clearSpokenKeys();
    ASSERT_TRUE(pick(*scene, "zhangtie_dating2"));
    EXPECT_EQ(finishTalking(), 1);
    EXPECT_EQ(app_.spokenKeys(), std::vector<std::string>{a.textKey}) << "重看：只重说情报";
    EXPECT_EQ(app_.state().itemCountOfAge(herb, age), before + a.gives[0].count) << "效果不重发";
}

// 求购：菜单上写价码 → 先开价（不扣钱）→「买」→ 扣钱给货、记买过 → 收起不再挂。
TEST_F(PathActionFlow, APurchaseQuotesThenTakesTheMoneyHandsOverTheGoodsAndIsGone) {
    enter("ch02_wairentang");
    app_.state().setFlag("ch02.duan2_start");
    app_.state().setFlag("ch02.duan3_start");
    const PathAction& a = entry(2, "maliu_qiugou");
    app_.state().addItem(fanren::game::kSpiritStoneItemId, a.price + 4);
    const int goods = app_.state().itemCount(a.goods.itemId);

    const fanren::core::Item* item = app_.data().findItem(a.goods.itemId);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(PathActionScene::rowFor(app_.data(), a).detail,
              item->name + " × " + std::to_string(a.goods.count) + " · " + std::to_string(a.price) + " 块")
        << "价码写在界面上、从 price 取（契约 5.3）";

    PathActionScene* scene = open("npc_tongmen_maliu");
    ASSERT_NE(scene, nullptr);
    ASSERT_TRUE(pick(*scene, "maliu_qiugou"));
    EXPECT_EQ(finishTalking(), 1) << "先开价";
    EXPECT_EQ(app_.lastSpokenKey(), a.textKey);
    ASSERT_EQ(top(), "PathAction") << "开价说完回到「买 / 不买」";
    EXPECT_EQ(scene->phase(), Phase::Confirm);
    EXPECT_EQ(money(), a.price + 4) << "开价不扣钱";

    scene->answer(app_, true);
    EXPECT_EQ(finishTalking(), 1);
    EXPECT_EQ(top(), "World");
    EXPECT_EQ(app_.lastSpokenKey(), a.dealKey);
    EXPECT_EQ(money(), 4);
    EXPECT_EQ(app_.state().itemCount(a.goods.itemId), goods + a.goods.count);
    EXPECT_EQ(app_.state().flag(a.doneFlag), 1);
    EXPECT_FALSE(listed("npc_tongmen_maliu", "maliu_qiugou")) << "买过收起，每条只买一次";
}

// 钱不够：说 poor_key，一文不扣、一件不给、不记买过，条目还挂着；「不买」则一句不说、什么都不动。
TEST_F(PathActionFlow, DecliningOrComingUpShortChangesNothing) {
    enter("ch02_wairentang");
    app_.state().setFlag("ch02.duan2_start");
    app_.state().setFlag("ch02.duan3_start");
    const PathAction& a = entry(2, "maliu_qiugou");
    app_.state().addItem(fanren::game::kSpiritStoneItemId, a.price - 1);
    const int goods = app_.state().itemCount(a.goods.itemId);

    PathActionScene* scene = open("npc_tongmen_maliu");
    ASSERT_NE(scene, nullptr);
    ASSERT_TRUE(pick(*scene, "maliu_qiugou"));
    finishTalking();
    ASSERT_EQ(scene->phase(), Phase::Confirm);
    app_.clearSpokenKeys();
    scene->answer(app_, false);
    EXPECT_EQ(finishTalking(), 0) << "不买：一句不说";
    EXPECT_EQ(top(), "World");
    EXPECT_TRUE(app_.spokenKeys().empty());

    scene = open("npc_tongmen_maliu");
    ASSERT_NE(scene, nullptr);
    ASSERT_TRUE(pick(*scene, "maliu_qiugou"));
    finishTalking();
    ASSERT_EQ(scene->phase(), Phase::Confirm) << "钱不够也照样开价：钱要到说「买」时才问";
    scene->answer(app_, true);
    EXPECT_EQ(finishTalking(), 1);
    EXPECT_EQ(app_.lastSpokenKey(), a.poorKey);
    EXPECT_EQ(money(), a.price - 1) << "钱不够：一文不扣";
    EXPECT_EQ(app_.state().itemCount(a.goods.itemId), goods);
    EXPECT_EQ(app_.state().flag(a.doneFlag), 0);
    EXPECT_TRUE(listed("npc_tongmen_maliu", "maliu_qiugou")) << "没买成：还挂着，攒够了再来";
}

// 阅历不足：只说那一句回绝（不点名境界），什么都不记；境界够了再来，说的就是情报。
TEST_F(PathActionFlow, TooGreenForHimHeRefusesWithoutNamingARealm) {
    enter("ch02_yaopu");
    app_.state().setFlag("ch02.liao_modaifu");
    app_.state().realm = Realm::Mortal;
    const PathAction& a = entry(2, "modaifu_dating");
    ASSERT_GT(static_cast<int>(a.minRealm), 0) << "先验：这一条有阅历门槛";

    PathActionScene* scene = open("npc_mo_daifu");
    ASSERT_NE(scene, nullptr);
    ASSERT_TRUE(pick(*scene, "modaifu_dating"));
    EXPECT_EQ(finishTalking(), 1);
    EXPECT_EQ(app_.lastSpokenKey(), a.refuseKey);
    ASSERT_FALSE(app_.dialogueLog().empty());
    EXPECT_FALSE(namesARealm(app_.dialogueLog().back().body)) << app_.dialogueLog().back().body;
    EXPECT_EQ(app_.state().flag(a.doneFlag), 0) << "回绝不算做过";

    app_.state().realm = a.minRealm;
    scene = open("npc_mo_daifu");
    ASSERT_NE(scene, nullptr);
    ASSERT_TRUE(pick(*scene, "modaifu_dating"));
    EXPECT_EQ(finishTalking(), 1);
    EXPECT_EQ(app_.lastSpokenKey(), a.textKey) << "拦下它的是阅历，不是别的";
    EXPECT_EQ(app_.state().flag(a.doneFlag), 1);
}

// 界面上任何地方都不出现境界名（契约 5.3）：这一层的固有字，加上五章每一条的菜单行。
TEST_F(PathActionFlow, NoRealmNameEverReachesTheMenu) {
    ASSERT_TRUE(namesARealm("阅历不到" + std::string(fanren::rules::nameOf(Realm::QiRefining2))))
        << "先验：词表抓得住境界名";
    ASSERT_FALSE(namesARealm("求购")) << "先验：不误报";
    std::vector<std::string> shown = PathActionScene::fixedStrings(app_.data());
    for (const PathAction& a : app_.data().pathActions) {
        const fanren::ui::ListItem row = PathActionScene::rowFor(app_.data(), a);
        shown.push_back(row.label + row.detail);
    }
    ASSERT_GT(shown.size(), 7u) << "先验：读到了条目";
    for (const std::string& text : shown) {
        EXPECT_FALSE(text.empty()) << "有一个固有字的 key 没进 data/text/ui.json";
        EXPECT_EQ(text.rfind("ui.", 0), std::string::npos) << "文案 key 没查到正文：" << text;
        EXPECT_FALSE(namesARealm(text)) << text;
    }
}

// 世界层的两个钩子（Application::init 接上）：头顶气泡按挂着的几种行动浮，E 键对面前的人开菜单。
TEST_F(PathActionFlow, TheWorldSeesTheBubbleAndTheActionKeyOpensTheMenu) {
    enter("ch02_wairentang");
    auto* world = dynamic_cast<fanren::game::WorldScene*>(app_.topScene());
    ASSERT_NE(world, nullptr);
    // 站到马六（29,17）下面一格、面朝上。
    app_.state().position = fanren::core::Point{29, 18};
    app_.state().facing = 0;
    const std::string npc = "npc_tongmen_maliu";

    EXPECT_EQ(fanren::game::pathBubbleFor(app_.pathActionsFor(npc)), PathBubble::None);
    EXPECT_FALSE(world->pathAction(app_)) << "一条都没挂着：E 键不响";
    app_.tick(kFrame);
    EXPECT_EQ(top(), "World");

    app_.state().setFlag("ch02.liao_maliu");
    EXPECT_EQ(fanren::game::pathBubbleFor(app_.pathActionsFor(npc)), PathBubble::Inquire);
    app_.state().setFlag("ch02.duan2_start");
    app_.state().setFlag("ch02.duan3_start");
    EXPECT_EQ(fanren::game::pathBubbleFor(app_.pathActionsFor(npc)), PathBubble::Generic)
        << "打探加求购：两种都有，浮通用的那一个";

    EXPECT_TRUE(world->pathAction(app_));
    app_.tick(kFrame);
    ASSERT_EQ(top(), "PathAction");
    const auto* scene = dynamic_cast<PathActionScene*>(app_.topScene());
    ASSERT_EQ(scene->rows().size(), 2u);
    EXPECT_EQ(scene->rows()[0]->id, "maliu_dating") << "打探排在求购前面";
    EXPECT_EQ(scene->rows()[1]->id, "maliu_qiugou");
}

}  // namespace
