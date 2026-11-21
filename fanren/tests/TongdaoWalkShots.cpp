// 第 7 章同道录 2a 集成：用真数据、软件渲染把相处与战斗的接缝走一遍并拍图（docs/ch07-tongdaolu-integration.md 第 2 节）。
//
// 不是验收用例：截图口（main.cpp 的 --scene）拍得到的是一屏一屏的定格，拍不到「走着走着才出来」的东西——
// 切磋那张教学卡、战斗里给我方择人的菜单、叙话一句句的立绘与事件图的淡入淡出、升档提示。这里用真树（data/、scripts/、assets/），
// 非无头、软件渲染，按键照真入口推（SDL 键盘事件），一路拍 PNG。
// 只有设了 FANREN_TONGDAO_WALK=<目录> 才跑，没设就跳过（全量 ctest 里它是 Skipped）。不写任何入库的文件。
#include <gtest/gtest.h>
#include <SDL3/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ChapterFixture.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "core/rules/Companions.h"
#include "core/rules/Tongdao.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/DialogueScene.h"
#include "game/MenuScene.h"
#include "game/TongdaoPage.h"
#include "game/TutorialScene.h"
#include "game/WorldScene.h"
#include "io/SaveFile.h"

namespace {

namespace fs = std::filesystem;
using fanren::game::Application;
using fanren::game::BattleMenuMode;
using fanren::game::BattleScene;
using fanren::game::MenuScene;
using fanren::game::TongdaoPage;
namespace rules = fanren::rules;

constexpr double kFrame = 1.0 / 60.0;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "companions" / "han_yunzhi.json")) return candidate;
    }
    return ".";
}

std::string envDir(const char* name) {
#if defined(_MSC_VER)
    char* value = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) return {};
    std::string out = value;
    std::free(value);
    return out;
#else
    const char* value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string(value);
#endif
}

class TongdaoWalk : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = envDir("FANREN_TONGDAO_WALK");
        if (dir_.empty()) GTEST_SKIP() << "集成走查：设 FANREN_TONGDAO_WALK=<目录> 才跑";
        fs::create_directories(dir_);
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        auto ready = app_.init(assetRoot(), /*headless=*/false);
        ASSERT_TRUE(ready.ok) << ready.error;
        // 与 main.cpp 正常游玩时同一组开关。
        app_.setEncountersEnabled(true);
        app_.setTutorialsEnabled(true);
        app_.setCushionChoiceEnabled(true);
        started_ = true;
    }
    void TearDown() override {
        if (!started_) return;
        app_.shutdown();
        SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
    }

    fanren::core::GameState& state() { return app_.state(); }
    void tick(int frames = 1) {
        // 栈顶一换就画一帧：战斗开场的碎屏要先在 render 里拍下世界层（BattleScene::render）才会往下走。
        // 软件渲染每帧都画太慢，别的时候不画（演出按 tick 的时长走，不看画没画）。
        for (int i = 0; i < frames; ++i) {
            app_.tick(kFrame);
            if (app_.topScene() != drawnTop_) {
                drawnTop_ = app_.topScene();
                app_.engine().beginFrame();
                app_.drawScenes();
                app_.engine().endFrame();
            }
        }
    }
    static void pushKey(SDL_Scancode code, bool down) {
        SDL_Event event{};
        event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.scancode = code;
        event.key.down = down;
        SDL_PushEvent(&event);
    }
    void press(SDL_Scancode code, int settle = 2) {
        pushKey(code, true);
        tick();
        pushKey(code, false);
        tick(settle);
    }
    std::string top() {
        const auto* scene = app_.topScene();
        return scene == nullptr ? std::string{} : scene->name();
    }
    MenuScene* menu() { return dynamic_cast<MenuScene*>(app_.topScene()); }
    BattleScene* battle() { return dynamic_cast<BattleScene*>(app_.topScene()); }

    void shot(const std::string& name) {
        app_.engine().beginFrame();
        app_.drawScenes();
        const fs::path png = fs::path(dir_) / (name + ".png");
        const auto wrote = app_.engine().captureFrame(png.string());
        app_.engine().endFrame();
        EXPECT_TRUE(wrote.ok) << wrote.error;
        std::printf("[walk] %s  top=%s\n", name.c_str(), top().c_str());
    }
    // 教学卡压上来就拍一张、按确认收起。
    void clearTutorials(const std::string& prefix) {
        for (int guard = 0; guard < 8 && top() == "Tutorial"; ++guard) {
            const auto* card = dynamic_cast<const fanren::game::TutorialScene*>(app_.topScene());
            tick(30);
            shot(prefix + "_tutorial_" + (card != nullptr ? card->cardId() : std::string("?")));
            press(SDL_SCANCODE_RETURN, 10);
        }
    }

    // 刚出禁地的检查点，演过谢师礼，站在茅屋里蒲团前（与 TongdaoPlaySaves 的乙、丙同一个起点）。
    void startInTheHut() {
        const auto loaded = fanren::io::loadGame(
            fanren::test::chapterFixturePath(assetRoot(), "companions/ch07-chujindi-first.sav").string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        state() = loaded.value;
        state().setFlag("ch07.xieshili");
        ASSERT_TRUE(app_.loadMap("ch06_baiyaoyuan", std::string{}).ok);
        state().position = {23, 5};
        state().facing = 0;
        app_.pushScene(std::make_unique<fanren::game::WorldScene>());
        tick(20);
        for (const auto& [id, companion] : app_.data().companions) {
            for (const std::string& like : companion.giftsLike) state().addItem(like, 3);
        }
        state().addItem("pill_yangjing_dan", 8);
    }

    // 主菜单停在某位的详情。
    void openDetail(const std::string& id) {
        backToWorld();
        auto scene = std::make_unique<MenuScene>(MenuScene::Page::Roster, true);
        scene->startTongdaoAt(id);
        app_.pushScene(std::move(scene));
        tick(20);
        clearTutorials("detail_" + id);
    }
    // 菜单上压着的一句话（赠礼的回话）按确认收掉。
    void dismissLines() {
        for (int guard = 0; guard < 6 && top() == "Dialogue"; ++guard) press(SDL_SCANCODE_RETURN, 10);
    }
    // 一路按取消，直到回到世界层（菜单一层层退出去）。
    void backToWorld() {
        for (int guard = 0; guard < 12 && top() != "World"; ++guard) {
            if (top() == "Tutorial" || top() == "Dialogue") {
                press(SDL_SCANCODE_RETURN, 10);
                continue;
            }
            press(SDL_SCANCODE_ESCAPE, 15);
        }
        tick(10);
    }
    // 详情里把动作列表的光标挪到第 index 行再确认。
    void chooseAction(int index) {
        ASSERT_NE(menu(), nullptr);
        tick(10);
        // 列表会跳过灰掉的行：按到光标真落在那一行为止。
        for (int guard = 0; guard < 8 && menu() != nullptr && menu()->tongdao().actions().selection() != index; ++guard) {
            press(menu()->tongdao().actions().selection() < index ? SDL_SCANCODE_DOWN : SDL_SCANCODE_UP);
        }
        ASSERT_EQ(menu()->tongdao().actions().selection(), index) << "那一行灰着，挪不过去";
        press(SDL_SCANCODE_RETURN, 10);
    }
    // 演一段事件：每句拍一张（等字打完），按确认往下，直到回到世界层。
    void playEvent(const std::string& prefix) {
        int line = 0;
        for (int guard = 0; guard < 400; ++guard) {
            tick(4);
            clearTutorials(prefix);
            if (top() == "Dialogue") {
                tick(40);
                char name[16];
                std::snprintf(name, sizeof name, "_%02d", ++line);
                shot(prefix + name);
                press(SDL_SCANCODE_RETURN, 4);
                if (top() == "Dialogue") press(SDL_SCANCODE_RETURN, 4);   // 字还没打完时第一下只是补全
                continue;
            }
            if (top() == "World" && !app_.scripts().isRunning()) break;
        }
        tick(30);
        shot(prefix + "_after");
    }

    std::string dir_;
    bool started_ = false;
    const fanren::game::Scene* drawnTop_ = nullptr;
    Application app_;
};

TEST_F(TongdaoWalk, TogetherInTheGardenAndTheSpar) {
    startInTheHut();
    for (const char* id : {"chen_qiaoqian", "nangong_wan", "han_yunzhi"}) state().bonds[id].value = 42;

    // 同道录卡片层（头一回进：教学卡「同道录」）。
    app_.pushScene(std::make_unique<MenuScene>(MenuScene::Page::Roster));
    tick(10);
    press(SDL_SCANCODE_RETURN, 20);
    clearTutorials("w01");
    shot("w01_cards");
    // 菡云芝：照真按键挑她、确认进详情（头一回在百药园看详情：教学卡「相处」）→ 赠礼合意 → 三档。
    for (int guard = 0; guard < 8 && menu() != nullptr && menu()->tongdao().selectedId() != "han_yunzhi"; ++guard) {
        press(SDL_SCANCODE_RIGHT);
    }
    press(SDL_SCANCODE_RETURN, 20);
    clearTutorials("w02");
    shot("w02_han_detail_tier2");
    chooseAction(static_cast<int>(fanren::game::TongdaoAction::Gift));
    shot("w03_han_gift_list");
    {
        const auto& gifts = menu()->tongdao().gifts();
        int want = -1;
        for (int i = 0; i < static_cast<int>(gifts.items().size()); ++i) {
            if (gifts.items()[static_cast<std::size_t>(i)].label.find("寒蟾酥") != std::string::npos) want = i;
        }
        ASSERT_GE(want, 0) << "赠礼列表里有寒蟾酥";
        for (int i = gifts.selection(); i < want; ++i) press(SDL_SCANCODE_DOWN);
    }
    shot("w04_han_gift_pick");
    press(SDL_SCANCODE_RETURN, 10);
    tick(40);
    shot("w05_han_gift_done");
    dismissLines();
    EXPECT_EQ(state().bonds["han_yunzhi"].value, 47);
    // 赠礼办完菜单自己停在详情（TongdaoPage::giveSelected），不用再按取消。
    shot("w06_han_detail_tier3");
    // 二档、三档的叙话各一段（一次升两档也要演两回）。
    chooseAction(static_cast<int>(fanren::game::TongdaoAction::Xuhua));
    playEvent("w07_han_t2");
    EXPECT_EQ(state().flag("tongdao.han_yunzhi.xuhua"), 2);
    EXPECT_EQ(state().itemCount("talisman_huoqiu_fu"), 8 + 3) << "二档叙话给 3 张火球符";
    openDetail("han_yunzhi");
    shot("w08_han_detail_after_t2");
    chooseAction(static_cast<int>(fanren::game::TongdaoAction::Xuhua));
    playEvent("w09_han_t3");
    EXPECT_EQ(state().flag("tongdao.han_yunzhi.xuhua"), 3);
    openDetail("han_yunzhi");
    chooseAction(static_cast<int>(fanren::game::TongdaoAction::Qingjiao));
    playEvent("w10_han_qingjiao");
    EXPECT_EQ(state().flag("tongdao.han_yunzhi.qingjiao"), 1);
    openDetail("han_yunzhi");
    shot("w11_han_detail_bonus_on");
    EXPECT_TRUE(rules::rosterBonusUnlocked(app_.data().companions.at("han_yunzhi"), state()));

    // 切磋：菡云芝（三档，带回春符；敌方 AI 不放辅助法术）。头一回轮到下令：教学卡「切磋」。
    const int hpBefore = state().hp;
    const int mpBefore = state().mp;
    chooseAction(static_cast<int>(fanren::game::TongdaoAction::Spar));
    bool sawMenu = false;
    bool sawEnd = false;
    int round = 0;
    for (int guard = 0; guard < 6000; ++guard) {
        tick();
        clearTutorials("w12_spar");
        BattleScene* b = battle();
        if (b == nullptr) {
            if (top() == "World" || top() == "Dialogue") break;
            continue;
        }
        if (b->menuMode() == BattleMenuMode::Root && !b->playing()) {
            if (!sawMenu) {
                tick(30);
                shot("w13_spar_first_command");
                sawMenu = true;
            }
            ++round;
            b->menuChoose(app_, fanren::game::kBattleMenuAttack);
            tick(2);
            if (b->menuMode() == BattleMenuMode::Weapon) b->menuChoose(app_, 0);
            tick(2);
            if (round == 1 && b->menuMode() == BattleMenuMode::Target) shot("w14_spar_target");
            if (b->menuMode() == BattleMenuMode::Target) b->menuChoose(app_, 0);
            if (round == 2) {
                tick(20);
                shot("w15_spar_midway");
            }
        }
        if (b->battle().phase() != fanren::core::battle::BattlePhase::Ongoing && !b->playing()) {
            if (!sawEnd) {
                tick(90);
                shot("w15b_spar_result");
                sawEnd = true;
            }
            press(SDL_SCANCODE_RETURN, 20);
        }
    }
    tick(60);
    shot("w16_spar_after");
    EXPECT_TRUE(sawMenu);
    EXPECT_EQ(state().hp, hpBefore) << "切磋打完气血放回开打前";
    EXPECT_EQ(state().mp, mpBefore) << "切磋打完法力放回开打前";
    std::printf("[walk] spar: bond han=%d prize=%d rounds=%d\n", state().bonds["han_yunzhi"].value,
                state().flag("tongdao.han_yunzhi.spar_prize"), round);

    // 蒲团：「打坐 / 同修」（头一回：教学卡「同修」）→ 同修 → 挑陈巧倩 → 过一天。
    backToWorld();
    const int dayBefore = state().day;
    const int chenBefore = state().bonds["chen_qiaoqian"].value;
    const fanren::core::TileMap* map = app_.currentMap();
    ASSERT_NE(map, nullptr);
    for (const auto& object : map->objects) {
        if (object.name == "facility_dazuo") app_.openFacility(object);
    }
    tick(20);
    clearTutorials("w17_cushion");
    shot("w17_cushion_choose");
    ASSERT_EQ(top(), "Tongxiu");
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN, 10);
    shot("w18_cushion_pick");
    press(SDL_SCANCODE_RETURN, 20);
    tick(40);
    shot("w19_tongxiu_done");
    backToWorld();
    EXPECT_EQ(state().day, dayBefore + 1) << "同修过一天";
    EXPECT_EQ(state().bonds["chen_qiaoqian"].value, chenBefore + 4) << "同修默契 +4";
    EXPECT_EQ(state().flag("tongdao.tongxiu_bp_day"), state().day) << "劲挂到第二天";
}

TEST_F(TongdaoWalk, TheHongyanEventPictureAndAFightWithAllThree) {
    startInTheHut();
    struct Gate {
        const char* id;
        int bond;
        int talked;
    };
    for (const Gate& g : {Gate{"chen_qiaoqian", 93, 4}, Gate{"nangong_wan", 67, 3}, Gate{"han_yunzhi", 93, 4}}) {
        state().bonds[g.id].value = g.bond;
        state().setFlag(rules::xuhuaFlag(g.id), g.talked);
        state().setFlag(rules::qingjiaoFlag(g.id), 1);
    }
    // 陈巧倩：合意赠礼 93 → 98，进「红颜」；叙话五档（事件图）。
    openDetail("chen_qiaoqian");
    clearTutorials("x01");
    shot("x01_chen_detail_tier4");
    chooseAction(static_cast<int>(fanren::game::TongdaoAction::Gift));
    {
        const auto& gifts = menu()->tongdao().gifts();
        int want = -1;
        for (int i = 0; i < static_cast<int>(gifts.items().size()); ++i) {
            if (gifts.items()[static_cast<std::size_t>(i)].label.find("火龙草") != std::string::npos) want = i;
        }
        ASSERT_GE(want, 0) << "赠礼列表里有火龙草";
        for (int i = gifts.selection(); i < want; ++i) press(SDL_SCANCODE_DOWN);
    }
    press(SDL_SCANCODE_RETURN, 10);
    tick(40);
    shot("x02_chen_gift_done");
    dismissLines();
    EXPECT_EQ(state().bonds["chen_qiaoqian"].value, 98);
    press(SDL_SCANCODE_ESCAPE, 10);
    shot("x03_chen_detail_tier5");
    backToWorld();
    tick(20);
    shot("x04_world_tierup_notice");
    openDetail("chen_qiaoqian");
    chooseAction(static_cast<int>(fanren::game::TongdaoAction::Xuhua));
    // 事件图的淡入、淡出各多拍一张中途。
    int line = 0;
    std::string lastCg;
    for (int guard = 0; guard < 400; ++guard) {
        tick(2);
        if (app_.currentCg() != lastCg) {
            tick(9);
            shot(std::string("x05_cg_") + (app_.currentCg().empty() ? "off_mid" : "on_mid"));
            lastCg = app_.currentCg();
        }
        if (top() == "Dialogue") {
            tick(40);
            char name[24];
            std::snprintf(name, sizeof name, "x06_chen_t5_%02d", ++line);
            shot(name);
            press(SDL_SCANCODE_RETURN, 4);
            if (top() == "Dialogue") press(SDL_SCANCODE_RETURN, 4);
            continue;
        }
        if (top() == "World" && !app_.scripts().isRunning() && line > 0) break;
    }
    tick(30);
    shot("x07_chen_t5_after");
    EXPECT_EQ(state().flag("tongdao.chen_qiaoqian.xuhua"), 5);
    EXPECT_TRUE(app_.currentCg().empty());

    // 历练明雷：三位都带上（陈 98、南宫 69→94、菡 95），看按档的法术、辅助法术的择人、合招。
    state().bonds["nangong_wan"].value = 94;
    state().bonds["han_yunzhi"].value = 95;
    for (const char* role : {"chen_shimei", "nangong_wan_ch07", "han_yunzhi"}) ASSERT_TRUE(state().partyAdd(role));
    app_.pushScene(std::make_unique<BattleScene>("bl07_huoyan_shu_t1"));
    std::map<std::string, int> turns;
    int frames = 0;
    for (; frames < 20000; ++frames) {
        tick();
        clearTutorials("y_battle");
        BattleScene* b = battle();
        if (b == nullptr) break;
        if (b->battle().phase() != fanren::core::battle::BattlePhase::Ongoing) {
            if (!b->playing()) {
                tick(90);
                shot("y99_battle_result");
                break;
            }
            continue;
        }
        if (b->menuMode() != BattleMenuMode::Root || b->playing()) continue;
        const int actor = b->battle().currentActor();
        if (actor < 0) continue;
        const auto& unit = b->battle().units()[static_cast<std::size_t>(actor)];
        const int seen = turns[unit.id]++;
        const auto pick = [&](const char* word) {
            const auto& items = b->menuList().items();
            for (int i = 0; i < static_cast<int>(items.size()); ++i) {
                if (items[static_cast<std::size_t>(i)].label.find(word) != std::string::npos) return i;
            }
            return -1;
        };
        const auto attack = [&] {
            b->menuChoose(app_, fanren::game::kBattleMenuAttack);
            tick(2);
            if (b->menuMode() == BattleMenuMode::Weapon) b->menuChoose(app_, 0);
            tick(2);
            if (b->menuMode() == BattleMenuMode::Target) b->menuChoose(app_, 0);
        };
        const std::string tag = "y_" + unit.id + "_" + std::to_string(seen);
        if (seen == 0) {
            tick(20);
            shot(tag + "_root");
            b->menuChoose(app_, fanren::game::kBattleMenuCast);
            tick(10);
            shot(tag + "_magic");
            const char* spell = nullptr;
            std::string spellTag;   // 文件名只用 ASCII（MSVC 的窄字符串路径走本机代码页）
            if (unit.id == "chen_shimei") spell = "剑幕", spellTag = "jianmu";
            if (unit.id == "nangong_wan_ch07") spell = "环护", spellTag = "huanhu";
            if (unit.id == "han_yunzhi") spell = "定身符", spellTag = "dingshen";
            const int at = spell == nullptr ? -1 : pick(spell);
            // 光标挪到这一门上再拍一张：说明区写的是光标那一行的法术（路 U ac094c4 补的新效果说明）。
            for (int i = 0; at > 0 && i < 8 && b->menuList().selection() < at; ++i) press(SDL_SCANCODE_DOWN);
            if (at >= 0) shot(tag + "_magic_" + spellTag);
            if (at >= 0 && b->menuChoose(app_, at)) {
                tick(10);
                if (b->menuMode() == BattleMenuMode::Target) {
                    shot(tag + "_target_" + spellTag);
                    b->menuChoose(app_, b->menuList().selection());
                }
                for (int i = 0; i < 25; ++i) tick();
                shot(tag + "_cast_" + spellTag);
                continue;
            }
            b->menuBack(app_);
            tick(2);
            attack();
            continue;
        }
        if (seen == 1 && unit.id == "han_yunzhi") {
            // 第二回：回春符的择人（谁挨了打谁在前头），救一下。
            b->menuChoose(app_, fanren::game::kBattleMenuCast);
            tick(4);
            const int at = pick("回春符");
            for (int i = 0; at > 0 && i < 8 && b->menuList().selection() < at; ++i) press(SDL_SCANCODE_DOWN);
            if (at >= 0) shot(tag + "_magic_huichun");
            if (at >= 0 && b->menuChoose(app_, at)) {
                tick(10);
                shot(tag + "_target_huichun");
                if (!b->menuChoose(app_, b->menuList().selection())) {
                    b->menuBack(app_);
                    b->menuBack(app_);
                    tick(2);
                    attack();
                }
                for (int i = 0; i < 25; ++i) tick();
                shot(tag + "_cast_huichun");
                continue;
            }
            b->menuBack(app_);
            tick(2);
            attack();
            continue;
        }
        if (seen == 1 && unit.id == "hanli") {
            const int duo = 5;   // 第六项「合招」
            if (static_cast<int>(b->menuList().items().size()) > duo && b->menuChoose(app_, duo)) {
                tick(10);
                shot(tag + "_duo_list");
                b->menuBack(app_);
                tick(2);
            }
        }
        attack();
    }
    std::printf("[walk] battle frames=%d\n", frames);
}

}  // namespace
