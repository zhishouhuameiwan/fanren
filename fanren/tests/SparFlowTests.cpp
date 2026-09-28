// 路径行动收尾（契约 docs/interfaces-octo-pathactions.md 5.4、5.7、6.1）：切磋开战与收场回填、
// 揭破绽写进存档。无头下走真数据、走上线的那条路（PathActionScene::choose → Application::applyPathEffects
// → startBattle → BattleScene 无头打完 → completeCommand 认出回调 → challengeResultEffects）。
//
// 四场切磋各走一遍「先输一次、再赢一次」：
//   · 输：说负那一句、什么也不发、不记做过、条目照旧挂着（可以再来）、气血至少 1、不 game over；
//   · 赢：说胜那一句、条目奖励**只发这一次**（编成 rewards 全 0，修为的增量恰好是条目那一份）、
//         记做过、条目收起。
// 邀战那一句必须先于那一仗上屏：选完之后头一帧栈顶是对话框，不是战斗。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/model/AttackCategory.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/PathActionScene.h"
#include "game/WorldScene.h"
#include "io/SaveFile.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::PathAction;
using fanren::game::Application;
using fanren::game::PathActionScene;
using fanren::rules::Realm;

constexpr double kFrame = 1.0 / 60.0;

fs::path findProjectRoot() {
    fs::path dir = fs::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        if (fs::exists(dir / "scripts" / "common" / "api.lua") && fs::is_directory(dir / "data")) return dir;
        const fs::path parent = dir.parent_path();
        if (parent.empty() || parent == dir) break;
        dir = parent;
    }
    return {};
}

struct Spar {
    int chapter;
    const char* id;
    const char* map;
    const char* npc;
    const char* openFlag;   // 条目 when 里那一面：置了才挂得出来
};

// 契约 6.3 的四场。
const Spar kSpars[] = {
    {4, "maliu_qiecuo", "ch02_wairentang", "npc_tongmen_maliu", "ch03.done"},
    {4, "feiyu_qiecuo", "ch04_getang", "npc_li_feiyu", "ch04.fawu_bingyong"},
    {5, "huyuan_b_qiecuo", "ch05_nancheng", "npc_huyuan_b", "ch05.dengmen"},
    {5, "huyuan_a_qiecuo", "ch05_nancheng", "npc_huyuan_a", "ch05.duizhi"},
};

class SparFlow : public ::testing::Test {
protected:
    void SetUp() override {
        const fs::path root = findProjectRoot();
        ASSERT_FALSE(root.empty()) << "找不到工程根目录";
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

    std::string top() { return app_.topScene() == nullptr ? std::string("<空>") : app_.topScene()->name(); }

    const PathAction& entry(int chapter, const std::string& id) {
        static const PathAction missing;
        const auto& all = app_.data().pathActions;
        const auto it = std::find_if(all.begin(), all.end(),
                                     [&](const PathAction& a) { return a.chapter == chapter && a.id == id; });
        if (it != all.end()) return *it;
        ADD_FAILURE() << "真数据里没有第 " << chapter << " 章的 " << id;
        return missing;
    }

    bool listed(const std::string& npc, const std::string& id) {
        const auto now = app_.pathActionsFor(npc);
        return std::any_of(now.begin(), now.end(), [&](const PathAction* a) { return a->id == id; });
    }

    // 按 E、选 id 那一行、走一帧让这一批落地。菜单上没有这一行返回 false。
    bool choose(const std::string& npc, const std::string& id) {
        if (!app_.openPathActions(npc)) return false;
        app_.tick(kFrame);
        auto* scene = dynamic_cast<PathActionScene*>(app_.topScene());
        if (scene == nullptr) return false;
        const auto& rows = scene->rows();
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i]->id != id) continue;
            scene->choose(app_, static_cast<int>(i));
            app_.tick(kFrame);
            return true;
        }
        return false;
    }

    // 替玩家把这一轮演完：对话框一个个收掉，仗在无头下自己打完，直到回到世界层。返回收了几个对话框。
    int playThrough() {
        int dialogues = 0;
        for (int guard = 0; guard < 64 && top() != "World"; ++guard) {
            if (top() == "Dialogue") {
                app_.popScene();
                ++dialogues;
            }
            app_.tick(kFrame);
        }
        return dialogues;
    }

    void makeHopeless(Realm realm) {
        auto& s = app_.state();
        s.realm = realm;
        s.hp = s.maxHp = 1;
        s.mp = s.maxMp = 0;
        s.party.clear();
    }

    void makeOverwhelming() {
        auto& s = app_.state();
        s.realm = Realm::CoreLate;
        s.hp = s.maxHp = 5000;
        s.mp = s.maxMp = 0;
        s.party.clear();
    }

    Application app_;
};

TEST_F(SparFlow, EachOfTheFourSparsLosesWithoutGameOverThenWinsAndPaysOnce) {
    for (const Spar& spar : kSpars) {
        SCOPED_TRACE(spar.id);
        app_.state() = fanren::core::GameState{};
        app_.clearSpokenKeys();
        enter(spar.map);
        app_.state().setFlag(spar.openFlag);
        const PathAction& a = entry(spar.chapter, spar.id);
        ASSERT_EQ(a.kind, fanren::core::PathActionKind::Challenge);
        ASSERT_FALSE(a.battleId.empty());
        ASSERT_NE(app_.battleSetup(a.battleId), nullptr) << "编成要建好（契约 6.2：pending 已去掉）";
        ASSERT_TRUE(listed(spar.npc, spar.id)) << "先验：置了 " << spar.openFlag << " 条目就挂出来";

        // ---- 输一次 ----
        // 条目的阅历门槛（马六那一条是炼气五层）要够，否则连仗都开不起来——输要输在仗上。
        makeHopeless(a.minRealm);
        const int cultivationBefore = app_.state().cultivation;
        ASSERT_TRUE(choose(spar.npc, spar.id));
        EXPECT_EQ(top(), "Dialogue") << "邀战那一句要先于那一仗上屏";
        playThrough();
        ASSERT_EQ(top(), "World");
        ASSERT_GE(app_.spokenKeys().size(), 2u);
        EXPECT_EQ(app_.spokenKeys()[0], a.textKey) << "先说邀战";
        ASSERT_EQ(app_.lastSpokenKey(), a.loseKey) << "先验：这一场要真的输掉";
        EXPECT_FALSE(app_.quitRequested()) << "切磋输了不 game over（契约 6.1）";
        EXPECT_GE(app_.state().hp, 1) << "气血按至少 1 放回来";
        EXPECT_EQ(app_.state().cultivation, cultivationBefore) << "输了却涨了修为";
        EXPECT_EQ(app_.state().flag(a.doneFlag), 0) << "输了不记做过";
        EXPECT_TRUE(listed(spar.npc, spar.id)) << "输了可以再来";

        // ---- 再来，赢 ----
        makeOverwhelming();
        app_.clearSpokenKeys();
        std::vector<int> itemsBefore;
        for (const auto& item : a.rewardItems) itemsBefore.push_back(app_.state().itemCount(item.itemId));
        const int before = app_.state().cultivation;
        ASSERT_TRUE(choose(spar.npc, spar.id));
        playThrough();
        ASSERT_EQ(top(), "World");
        ASSERT_EQ(app_.lastSpokenKey(), a.winKey) << "先验：这一场要真的打赢";
        EXPECT_EQ(app_.state().cultivation - before, a.rewardCultivation)
            << "修为只发条目那一份（编成 rewards 全 0，不许一场拿两份）";
        for (std::size_t i = 0; i < a.rewardItems.size(); ++i) {
            EXPECT_EQ(app_.state().itemCount(a.rewardItems[i].itemId) - itemsBefore[i], a.rewardItems[i].count)
                << a.rewardItems[i].itemId;
        }
        EXPECT_EQ(app_.state().flag(a.doneFlag), 1) << "赢了记做过";
        EXPECT_FALSE(listed(spar.npc, spar.id)) << "赢过就收起：奖励只发一次";
        EXPECT_FALSE(app_.quitRequested());
    }
}

// 揭破绽（契约 5.4）：打探那一刻写进 GameState::knownWeaknesses，旁白补一句「记下了某某的破绽」，
// 情报先说、旁白后说；存档往返后还在。第 3 章谷外遇狼之前向药圃管事打探（揭恶狼的「拳」）。
TEST_F(SparFlow, AnInquiryRevealWritesKnownWeaknessesAndSurvivesASave) {
    enter("ch02_yaopu");
    app_.state().setFlag("ch02.done");
    const PathAction& a = entry(3, "guanshi_dating");
    ASSERT_EQ(a.reveals.size(), 1u) << "先验：这一条揭一样破绽";
    const std::string role = a.reveals[0].roleId;
    const int bit = fanren::core::categoryFromName(a.reveals[0].category);
    ASSERT_NE(bit, 0);
    ASSERT_EQ(app_.state().knownWeaknessesOf(role) & bit, 0) << "先验：新开一局什么都不知道";

    ASSERT_TRUE(choose("npc_yaopu_guanshi", a.id));
    EXPECT_EQ(playThrough(), 2) << "两句：情报，然后「记下了……的破绽」";
    EXPECT_NE(app_.state().knownWeaknessesOf(role) & bit, 0) << "打探完就该记进已知破绽";

    const auto& log = app_.dialogueLog();
    ASSERT_GE(log.size(), 2u);
    EXPECT_EQ(log[log.size() - 2].body, app_.text(a.textKey)) << "情报先说";
    EXPECT_EQ(log.back().body, app_.text("ui.path.reveal.lead") + app_.speakerName(role) +
                                   app_.text("ui.path.reveal.tail") + a.reveals[0].category);
    EXPECT_TRUE(log.back().speaker.empty()) << "那一句是旁白，不是管事在说";

    // 存档不必另加字段（契约 5.5 的推法照样成立），但写进去的这一笔要能往返。
    fanren::test::TempDir dir("fanren_reveal_save");
    const std::string path = (dir.path() / "reveal.sav").string();
    ASSERT_TRUE(fanren::io::saveGame(app_.state(), path).ok);
    const auto loaded = fanren::io::loadGame(path);
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_NE(loaded.value.knownWeaknessesOf(role) & bit, 0);
}

}  // namespace
