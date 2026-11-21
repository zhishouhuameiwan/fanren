// 同道录 2a·路 U：界面（docs/ch07-tongdaolu-u.md）。
//   * 同道录页：顶替名册页；列出每一位（没遇到的是剪影 + 提示）；档名按线、封顶写「机缘未到」；档位给什么从表生成；
//   * 相处四个动作：一律问规则层，地点只在百药园；脚本不在写「（内容未到）」；赠礼 / 切磋 / 请教 / 叙话走真菜单；
//   * 蒲团：百药园、同道录里有人物、开关开着时先出「打坐 / 同修」；
//   * 升档 / 入册提示：运行时比较，不立旗标；无头不出；
//   * 事件图：表的形状、淡入淡出、垫在对话框下（软件渲染比像素）；
//   * 战斗：护持、定身等的持续标记跟着事件走；五种事件的截图口推得到那一刻；
//   * 教学卡四张只在第 7 章、第一次用到时弹。
// 截图：设 FANREN_TONGDAO_SHOTS=<目录> 时，像素用例把拍到的图另存一份到那里（施工记录第 7 节的图就是这样拍的）。
#include <gtest/gtest.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/rules/Companions.h"
#include "core/rules/Tongdao.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/CultivationScene.h"
#include "game/EventCg.h"
#include "game/MenuScene.h"
#include "game/ShotScenes.h"
#include "game/TongdaoPage.h"
#include "game/TongxiuScene.h"
#include "io/CgLoader.h"

namespace {

namespace fs = std::filesystem;
using fanren::game::Application;
using fanren::game::MenuScene;
using fanren::game::TongdaoPage;
using fanren::game::TongxiuScene;
namespace rules = fanren::rules;
namespace game = fanren::game;
namespace engine = fanren::engine;

constexpr double kFrame = 1.0 / 60.0;

fs::path repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

void writeFile(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

// 一张纯色 PNG（w×h）。
void writeSolidPng(const fs::path& path, int w, int h, SDL_Color color) {
    fs::create_directories(path.parent_path());
    SDL_Surface* surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    ASSERT_NE(surface, nullptr);
    SDL_FillSurfaceRect(surface, nullptr, SDL_MapSurfaceRGBA(surface, color.r, color.g, color.b, color.a));
    ASSERT_TRUE(SDL_SavePNG(surface, path.string().c_str()));
    SDL_DestroySurface(surface);
}

std::string role(const std::string& id, const std::string& name, int hp, int attack, const std::string& extra = "") {
    return R"({"id": ")" + id + R"(", "name": ")" + name + R"(", "realm": "QiRefining10", "maxHp": )" +
           std::to_string(hp) + R"(, "maxMp": 50, "attack": )" + std::to_string(attack) +
           R"(, "defence": 1, "speed": 5, "element": 0, "magics": ["magic_probe_one"])" + extra + "}";
}

// 测试树：仓库的 data / scripts / maps，加几位探针伙伴、她们的编成、叙话与请教的桩脚本、一张立绘与一幅事件图。
//   probe_a：红颜线，会追击、三档一门法术与被动、四档合招、在册加成、切磋（她的角色一碰就倒）；叙话二档与请教有桩脚本
//   probe_b：通用线，第 7 章封在四档（probe.cap 置位前）；没有脚本、没有加成
//   probe_c：从来没遇到（剪影），有一句提示
//   probe_d…probe_g：没遇到，只为凑满两页（九位）
class TongdaoUi : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_tongdao_ui");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(repoRoot() / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        const fs::path data = root_->path() / "data";
        for (const char* m : {"one", "two", "three"}) {
            writeFile(data / "magics" / (std::string("probe_") + m + ".json"),
                      std::string(R"({"id": "magic_probe_)") + m + R"(", "name": "探针)" + m +
                          R"(", "element": 1, "needMp": 0, "power": 5, "needRealm": "Mortal"})");
        }
        writeFile(data / "magics" / "probe_ward.json",
                  R"({"id": "magic_probe_ward", "name": "探针护持", "needMp": 0, "power": 0, "effect": "ward"})");
        writeFile(data / "magics" / "probe_wall.json",
                  R"({"id": "magic_probe_wall", "name": "探针剑幕", "needMp": 0, "power": 0, "effect": "bulwark", "amount": 30})");
        writeFile(data / "magics" / "probe_bind.json",
                  R"({"id": "magic_probe_bind", "name": "探针定身", "needMp": 0, "power": 0, "effect": "bind"})");
        writeFile(data / "roles" / "probe_a_role.json",
                  role("probe_a_role", "探针甲", 1, 1, R"(, "toughness": 1, "weaknesses": ["拳"])"));
        writeFile(data / "roles" / "probe_b_role.json", role("probe_b_role", "探针乙", 300, 10));
        writeFile(data / "roles" / "probe_c_role.json", role("probe_c_role", "探针丙", 300, 10));
        // 会三种辅助法术的帮手（战斗截图口用）：血厚、出手快。
        writeFile(data / "roles" / "probe_aide_role.json",
                  R"({"id": "probe_aide_role", "name": "探针帮手", "realm": "QiRefining10", "maxHp": 900, "maxMp": 90, )"
                  R"("attack": 1, "defence": 30, "speed": 99, "element": 0, )"
                  R"("magics": ["magic_probe_ward", "magic_probe_wall", "magic_probe_bind"]})");
        writeFile(data / "roles" / "probe_brute.json",
                  role("probe_brute", "探针恶狼", 900, 6, R"(, "toughness": 4, "weaknesses": ["火"])"));
        const fs::path companions = data / "companions";
        writeFile(companions / "probe_a.json", R"({
          "id": "probe_a", "line": "hongyan", "met_flag": "probe.a", "chase": true,
          "roles": [{"role": "probe_a_role",
                     "magics": [{"tier": 3, "magic": "magic_probe_two"}],
                     "passive": {"kind": "chase_power", "values": [20, 30]},
                     "duo": "magic_probe_three"}],
          "gifts": {"like": ["material_dansha"], "dislike": ["material_fuzhi"]},
          "roster_bonus": {"kind": "ambush_bp", "value": 1},
          "spar": {"battle": "probe_spar_a", "prizes": [{"tier": 2, "items": [{"item": "pill_huiqi_dan", "count": 2}]}]}
        })");
        writeFile(companions / "probe_b.json", R"({
          "id": "probe_b", "met_flag": "probe.b", "roles": [{"role": "probe_b_role"}],
          "bond_cap": {"until_flag": "probe.cap", "max_tier": 4}
        })");
        writeFile(companions / "probe_c.json",
                  R"({"id": "probe_c", "met_flag": "probe.c", "roles": [{"role": "probe_c_role"}]})");
        for (const char* extra : {"d", "e", "f", "g"}) {
            const std::string roleId = std::string("probe_") + extra + "_role";
            writeFile(data / "roles" / (roleId + ".json"), role(roleId, "探针", 300, 10));
            writeFile(companions / (std::string("probe_") + extra + ".json"),
                      std::string(R"({"id": "probe_)") + extra + R"(", "met_flag": "probe.)" + extra +
                          R"(", "roles": [{"role": ")" + roleId + R"("}]})");
        }
        writeFile(data / "companions" / "probe_aide.json",
                  R"({"id": "probe_aide", "met_flag": "probe.aide", "roles": [{"role": "probe_aide_role"}]})");
        writeFile(data / "battles" / "probe_spar_a.json",
                  R"({"id": "probe_spar_a", "name": "切磋", "chapter": 7, "terrain": "field", "can_escape": false, )"
                  R"("defeat_is_fatal": false, "spar": true, "units": [{"role_id": "probe_a_role", "faction": "enemy"}], )"
                  R"("rewards": {"cultivation": 0, "spirit_stones": 0, "drops": []}})");
        writeFile(data / "battles" / "probe_support.json",
                  R"({"id": "probe_support", "name": "探针", "chapter": 7, "terrain": "field", "can_escape": true, )"
                  R"("defeat_is_fatal": false, "units": [{"role_id": "probe_brute", "faction": "enemy"}], )"
                  R"("rewards": {"cultivation": 0, "spirit_stones": 0, "drops": []}})");
        writeFile(data / "text" / "test_tongdao.json",
                  R"({"tongdao.probe_c.hint": "传闻山中有位采药的姑娘。", "tongdao.probe_a.intro": "黄枫谷弟子"})");
        // 桩脚本：叙话二档、请教（路 N 的真脚本不在这里建）。
        writeFile(root_->path() / "scripts" / "ch07" / "tongdao_probe_a_t2.lua",
                  "flag.set(\"probe.xuhua_ran\", 1)\ntongdao.xuhua_mark(\"probe_a\", 2)\n");
        writeFile(root_->path() / "scripts" / "ch07" / "tongdao_probe_a_qingjiao.lua",
                  "flag.set(\"probe.qingjiao_ran\", 1)\ntongdao.qingjiao(\"probe_a\")\n");
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        prime(app_);
    }
    void TearDown() override { app_.shutdown(); }

    // 第 7 章里、人在百药园、probe_a 与 probe_b 遇到过。
    static void prime(Application& app) {
        auto& state = app.state();
        state.hp = state.maxHp = 400;
        state.mp = state.maxMp = 60;
        state.day = 30;
        state.setFlag("ch06.done");
        state.setFlag("probe.a");
        state.setFlag("probe.b");
        state.mapId = game::kTongdaoMap;
        state.bag.clear();
    }

    fanren::core::GameState& state() { return app_.state(); }
    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(kFrame);
    }
    static void pushKey(SDL_Scancode code, bool down) {
        SDL_Event event{};
        event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.scancode = code;
        event.key.down = down;
        SDL_PushEvent(&event);
    }
    void press(SDL_Scancode code) {
        pushKey(code, true);
        tick();
        pushKey(code, false);
        tick();
    }
    std::string top() {
        const game::Scene* scene = app_.topScene();
        return scene == nullptr ? std::string{} : scene->name();
    }
    MenuScene* menu() { return dynamic_cast<MenuScene*>(app_.topScene()); }
    std::string lastLine() const {
        const auto& log = app_.dialogueLog();
        return log.empty() ? std::string{} : log.back().body;
    }
    // 打开主菜单、光标停在同道录上（还没进去）。
    void openTongdaoPage() {
        app_.pushScene(std::make_unique<MenuScene>(MenuScene::Page::Roster));
        tick(2);
        ASSERT_EQ(top(), "Menu");
    }
    const game::TongdaoEntry* entry(const std::string& id) {
        static std::vector<game::TongdaoEntry> cache;
        cache = game::tongdaoEntries(app_);
        for (const auto& e : cache) {
            if (e.id == id) return &e;
        }
        return nullptr;
    }
    std::vector<fanren::ui::ListItem> rows(const std::string& id) { return game::tongdaoActionRows(app_, id); }

    static inline std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
};

using Act = game::TongdaoAction;
std::size_t at(Act act) { return static_cast<std::size_t>(act); }

// ---------------------------------------------------------------------------
// 页面
// ---------------------------------------------------------------------------

TEST_F(TongdaoUi, TheRosterPageIsNowTongdaoAndListsEveryCompanionWithSilhouettesForTheUnmet) {
    EXPECT_EQ(MenuScene::pageLabel(app_.data(), state(), MenuScene::Page::Roster), "同道录");
    openTongdaoPage();
    const auto& pages = menu()->pages();
    EXPECT_NE(std::find(pages.begin(), pages.end(), MenuScene::Page::Roster), pages.end());

    const auto entries = game::tongdaoEntries(app_);
    EXPECT_EQ(entries.size(), app_.data().companions.size()) << "data/companions 里每一位都在";
    const game::TongdaoEntry* a = entry("probe_a");
    ASSERT_NE(a, nullptr);
    EXPECT_TRUE(a->met);
    EXPECT_EQ(a->name, "探针甲");
    EXPECT_EQ(a->intro, "黄枫谷弟子") << "tongdao.<id>.intro";
    const game::TongdaoEntry* c = entry("probe_c");
    ASSERT_NE(c, nullptr);
    EXPECT_FALSE(c->met);
    EXPECT_EQ(c->name, "？？？");
    EXPECT_EQ(c->intro, "传闻山中有位采药的姑娘。") << "tongdao.<id>.hint";
    const game::TongdaoEntry* d = entry("probe_d");
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->intro, app_.text("ui.tongdao.hint.default")) << "提示缺了退回缺省";
    const game::TongdaoEntry* b = entry("probe_b");
    EXPECT_EQ(b->intro, "默契 0") << "简介缺了写默契";

    // 确认进到卡片层，光标停在第一位遇到过的人上；没遇到的人没有详情。
    press(SDL_SCANCODE_RETURN);
    ASSERT_TRUE(menu()->tongdaoFocused());
    const TongdaoPage& page = menu()->tongdao();
    EXPECT_EQ(page.focus(), TongdaoPage::Focus::Cards);
    EXPECT_EQ(page.selectedId(), "probe_a");
    // 九位以上分两页：光标往右走到第七位就翻到第二页。
    EXPECT_GE(page.pageCount(), 2);
    for (int i = 0; i < 8; ++i) press(SDL_SCANCODE_RIGHT);
    EXPECT_EQ(page.page(), page.cursor() / TongdaoPage::kPerPage);
    EXPECT_GE(page.page(), 1);
    while (page.selectedId() != "probe_c") press(SDL_SCANCODE_LEFT);
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(page.focus(), TongdaoPage::Focus::Cards) << "剪影按确认不进详情";
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_FALSE(menu()->tongdaoFocused()) << "Esc 回左栏";
    EXPECT_EQ(top(), "Menu");
}

TEST_F(TongdaoUi, TierNamesFollowTheLineAndACapShowsAsNotYet) {
    state().bonds["probe_a"].value = 50;   // 三档
    EXPECT_EQ(entry("probe_a")->tierName, "信任") << "红颜线三档";
    state().bonds["probe_b"].value = 50;
    EXPECT_EQ(entry("probe_b")->tierName, "默契") << "通用线三档";
    EXPECT_EQ(entry("probe_b")->capTier, 4) << "封顶没解开";
    state().setFlag("probe.cap");
    EXPECT_EQ(entry("probe_b")->capTier, 5);
}

TEST_F(TongdaoUi, WhatEachTierGivesIsGeneratedFromTheTables) {
    const auto& a = app_.data().companions.at("probe_a");
    const auto has = [](const std::vector<std::string>& list, const std::string& want) {
        return std::find(list.begin(), list.end(), want) != list.end();
    };
    const auto t1 = game::tierGains(app_, a, 1);
    EXPECT_TRUE(has(t1, "法术「探针one」")) << "一档就会的写在角色表里";
    const auto t2 = game::tierGains(app_, a, 2);
    EXPECT_TRUE(has(t2, "追击"));
    EXPECT_TRUE(has(t2, "可切磋"));
    EXPECT_TRUE(has(t2, "喜好全知"));
    EXPECT_TRUE(has(t2, "叙话一段")) << "二档的叙话脚本在";
    const auto t3 = game::tierGains(app_, a, 3);
    EXPECT_TRUE(has(t3, "法术「探针two」"));
    EXPECT_TRUE(has(t3, "被动：追击伤害 +20%"));
    EXPECT_TRUE(has(t3, "可请教"));
    EXPECT_FALSE(has(t3, "叙话一段")) << "三档的叙话脚本不在";
    EXPECT_TRUE(has(game::tierGains(app_, a, 4), "合招「探针three」"));
    EXPECT_TRUE(has(game::tierGains(app_, a, 5), "被动加强：追击伤害 +30%"));
    // 第 7 章以外按档的法术与被动不作数：不写。
    state().setFlag("ch07.done");
    EXPECT_FALSE(has(game::tierGains(app_, a, 3), "法术「探针two」"));
    // 在册加成的说明由种类与数值生成。
    EXPECT_EQ(game::perkText(app_.data(), a.rosterBonus, a.rosterBonus.value), "偷袭得手时全队开场再多 1 点劲");
    fanren::core::CompanionPerk fire;
    fire.kind = fanren::core::PerkKind::ElementStagger;
    fire.element = fanren::core::kElementFire;
    EXPECT_EQ(game::perkText(app_.data(), fire, 1), "你的火属法术打中破绽多削 1 点架势");
}

// ---------------------------------------------------------------------------
// 相处的四个动作：能不能做
// ---------------------------------------------------------------------------

TEST_F(TongdaoUi, AwayFromBaiyaoyuanTheFourActionsAreGreyWithTheReason) {
    state().mapId = "ch06_huangfenggu";
    state().addItem("material_dansha", 1);
    state().bonds["probe_a"].value = 60;
    const auto r = rows("probe_a");
    for (const Act act : {Act::Gift, Act::Spar, Act::Qingjiao, Act::Xuhua}) {
        EXPECT_FALSE(r[at(act)].enabled);
        EXPECT_EQ(r[at(act)].disabledReason, "回百药园后可相处");
    }
}

TEST_F(TongdaoUi, AtBaiyaoyuanEachActionAsksTheRulesAndMissingScriptsSayContentNotYet) {
    // 一档、身上空空：赠礼没东西；切磋、叙话要到二档；请教要到三档；加成还没生效。
    auto r = rows("probe_a");
    EXPECT_EQ(r[at(Act::Gift)].disabledReason, "身上没有送得出手的东西");
    EXPECT_EQ(r[at(Act::Spar)].disabledReason, "到「相熟」才行");
    EXPECT_EQ(r[at(Act::Qingjiao)].disabledReason, "到「信任」才行");
    EXPECT_EQ(r[at(Act::Xuhua)].disabledReason, "到「相熟」才行");
    EXPECT_EQ(r[at(Act::Bonus)].disabledReason, "还没生效");
    EXPECT_EQ(rows("probe_b")[at(Act::Bonus)].disabledReason, "没有在册加成");
    EXPECT_EQ(rows("probe_b")[at(Act::Spar)].disabledReason, "她不与人切磋") << "表里没有编成（规则层先判这一条）";

    state().addItem("material_dansha", 1);
    state().bonds["probe_a"].value = 20;   // 二档
    r = rows("probe_a");
    EXPECT_TRUE(r[at(Act::Gift)].enabled);
    EXPECT_TRUE(r[at(Act::Spar)].enabled);
    EXPECT_TRUE(r[at(Act::Xuhua)].enabled) << "升到二档、二档的脚本在";

    state().bonds["probe_a"].value = 45;   // 三档：请教有了；叙话还欠二档那一段
    r = rows("probe_a");
    EXPECT_TRUE(r[at(Act::Qingjiao)].enabled);
    ASSERT_EQ(rules::recordXuhua(app_.data().companions, "probe_a", state(), 2), rules::TongdaoCheck::Ok);
    EXPECT_EQ(rows("probe_a")[at(Act::Xuhua)].disabledReason, "（内容未到）") << "三档的叙话脚本不在";
    state().bonds["probe_b"].value = 45;
    EXPECT_EQ(rows("probe_b")[at(Act::Qingjiao)].disabledReason, "（内容未到）") << "她的请教脚本不在";
    EXPECT_EQ(rows("probe_b")[at(Act::Spar)].disabledReason, "她不与人切磋");

    // 今天送过了；今天切磋过了；请教过了。
    state().bonds["probe_a"].giftDay = state().day;
    state().setFlag(rules::sparDayFlag("probe_a"), state().day);
    ASSERT_EQ(rules::recordQingjiao(app_.data().companions, "probe_a", state()), rules::TongdaoCheck::Ok);
    r = rows("probe_a");
    EXPECT_EQ(r[at(Act::Gift)].disabledReason, "今天已经送过了");
    EXPECT_EQ(r[at(Act::Spar)].disabledReason, "今天做过了");
    EXPECT_EQ(r[at(Act::Qingjiao)].disabledReason, "已经请教过了");
    EXPECT_TRUE(r[at(Act::Bonus)].enabled) << "请教过：加成解锁，能勾";
}

// ---------------------------------------------------------------------------
// 相处的四个动作：走真菜单
// ---------------------------------------------------------------------------

TEST_F(TongdaoUi, GiftingFromTheDetailPageUsesTheRulesAndSaysHowMuchBondWasGained) {
    state().addItem("material_dansha", 2);   // 合意
    state().addItem("material_fuzhi", 1);    // 不喜
    state().bonds["probe_a"].value = 20;     // 二档：列表里标合意 / 不喜
    openTongdaoPage();
    press(SDL_SCANCODE_RETURN);   // 卡片层（probe_a）
    press(SDL_SCANCODE_RETURN);   // 详情
    const TongdaoPage& page = menu()->tongdao();
    ASSERT_EQ(page.focus(), TongdaoPage::Focus::Detail);
    ASSERT_EQ(page.actions().selection(), 0) << "光标在赠礼";
    press(SDL_SCANCODE_RETURN);   // 赠礼列表
    ASSERT_EQ(page.focus(), TongdaoPage::Focus::Gift);
    ASSERT_EQ(page.gifts().count(), 2);
    EXPECT_NE(page.gifts().items()[0].detail.find("合意"), std::string::npos);
    EXPECT_NE(page.gifts().items()[1].detail.find("不喜"), std::string::npos);
    press(SDL_SCANCODE_RETURN);
    tick();
    EXPECT_EQ(state().bondOf("probe_a"), 25) << "合意 +5";
    EXPECT_EQ(state().itemCount("material_dansha"), 1);
    EXPECT_EQ(lastLine(), "探针甲收下了，很合心意。默契 +5。");
    EXPECT_EQ(top(), "Dialogue");
    press(SDL_SCANCODE_RETURN);
    tick(2);
    ASSERT_EQ(top(), "Menu");
    EXPECT_EQ(menu()->tongdao().focus(), TongdaoPage::Focus::Detail);
    EXPECT_FALSE(game::tongdaoActionRows(app_, "probe_a")[at(Act::Gift)].enabled) << "今天送过了";
}

TEST_F(TongdaoUi, GiftListAtTierOneDoesNotRevealTastes) {
    state().addItem("material_dansha", 1);
    ASSERT_TRUE(app_.state().mapId == game::kTongdaoMap);
    auto menuScene = std::make_unique<MenuScene>(MenuScene::Page::Roster, true);
    menuScene->startTongdaoAt("probe_a", /*gift=*/true);
    app_.pushScene(std::move(menuScene));
    tick(2);
    ASSERT_EQ(menu()->tongdao().focus(), TongdaoPage::Focus::Gift);
    EXPECT_EQ(menu()->tongdao().gifts().items()[0].detail.find("合意"), std::string::npos);
}

TEST_F(TongdaoUi, SparClosesTheMenuFightsAndReportsWinBondAndPrize) {
    state().bonds["probe_a"].value = 20;
    openTongdaoPage();
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_RETURN);
    ASSERT_EQ(menu()->tongdao().actions().selection(), static_cast<int>(at(Act::Spar))) << "赠礼灰着，光标落在切磋";
    press(SDL_SCANCODE_RETURN);
    tick(6);
    EXPECT_NE(top(), "Menu") << "菜单合上了";
    EXPECT_EQ(state().bondOf("probe_a"), 24) << "赢 +4";
    EXPECT_EQ(state().itemCount("pill_huiqi_dan"), 2) << "二档首胜奖品";
    EXPECT_NE(lastLine().find("切磋胜了探针甲。默契 +4。"), std::string::npos) << lastLine();
    EXPECT_NE(lastLine().find("首胜之礼"), std::string::npos) << lastLine();
}

TEST_F(TongdaoUi, XuhuaAndQingjiaoRunTheirScriptsWhichDoTheirOwnBookkeeping) {
    state().bonds["probe_a"].value = 45;   // 三档：叙话欠二档那一段、能请教
    openTongdaoPage();
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_RETURN);
    // 赠礼灰着 → 切磋 → 请教 → 叙话
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_DOWN);
    ASSERT_EQ(menu()->tongdao().actions().selection(), static_cast<int>(at(Act::Xuhua)));
    press(SDL_SCANCODE_RETURN);
    tick(6);
    EXPECT_EQ(state().flag("probe.xuhua_ran"), 1);
    EXPECT_EQ(state().flag(rules::xuhuaFlag("probe_a")), 2) << "脚本自己记账";
    EXPECT_NE(top(), "Menu");

    openTongdaoPage();
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_DOWN);
    ASSERT_EQ(menu()->tongdao().actions().selection(), static_cast<int>(at(Act::Qingjiao)));
    press(SDL_SCANCODE_RETURN);
    tick(6);
    EXPECT_EQ(state().flag("probe.qingjiao_ran"), 1);
    EXPECT_TRUE(rules::rosterBonusUnlocked(app_.data().companions.at("probe_a"), state()));
}

TEST_F(TongdaoUi, TheBonusToggleWritesOnlyItsOwnFlag) {
    state().bonds["probe_a"].value = 45;
    ASSERT_EQ(rules::recordQingjiao(app_.data().companions, "probe_a", state()), rules::TongdaoCheck::Ok);
    state().mapId = "ch06_huangfenggu";   // 勾选哪里都能做（不是相处）
    openTongdaoPage();
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_RETURN);
    ASSERT_EQ(menu()->tongdao().actions().selection(), static_cast<int>(at(Act::Bonus))) << "别处只有这一项亮着";
    const auto before = state().flags;
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(state().flag(rules::bonusOnFlag("probe_a")), 1);
    auto after = state().flags;
    after.erase(rules::bonusOnFlag("probe_a"));
    auto expected = before;
    expected.erase(rules::bonusOnFlag("probe_a"));
    EXPECT_EQ(after, expected) << "别的旗一面不动";
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(state().flag(rules::bonusOnFlag("probe_a")), 0);
}

// ---------------------------------------------------------------------------
// 蒲团
// ---------------------------------------------------------------------------

class TongdaoCushion : public TongdaoUi {
protected:
    // 载入 mapId，面朝那张图上第一个打坐蒲团按下去（Application::openFacility 那条真入口）。
    std::string sitAt(const std::string& mapId) {
        auto loaded = app_.loadMap(mapId, "");
        EXPECT_TRUE(loaded.ok) << loaded.error;
        state().mapId = mapId;
        const fanren::core::TileMap* map = app_.currentMap();
        if (map == nullptr) return {};
        for (const auto& object : map->objects) {
            if (object.type == "facility" && object.property("kind") == "meditate") {
                app_.openFacility(object);
                tick(2);
                return top();
            }
        }
        ADD_FAILURE() << mapId << " 上没有蒲团";
        return {};
    }
};

TEST_F(TongdaoCushion, OnlyBaiyaoyuanWithSomeoneInTheRosterAndTheSwitchOnOffersTongxiu) {
    EXPECT_EQ(sitAt(game::kTongdaoMap), "Cultivation") << "开关缺省关着：与改版前一样";
    app_.popScene();
    tick();
    app_.setCushionChoiceEnabled(true);
    EXPECT_EQ(sitAt(game::kTongdaoMap), "Tongxiu");
    app_.popScene();
    tick();
    state().flags.erase("probe.a");
    state().flags.erase("probe.b");
    for (const auto& [id, c] : app_.data().companions) state().flags.erase(c.metFlag);
    EXPECT_EQ(sitAt(game::kTongdaoMap), "Cultivation") << "同道录里没有人物：与现在一模一样";
}

TEST_F(TongdaoCushion, SitGoesToTheOldPanelAndTongxiuSpendsADay) {
    app_.setCushionChoiceEnabled(true);
    ASSERT_EQ(sitAt(game::kTongdaoMap), "Tongxiu");
    press(SDL_SCANCODE_RETURN);   // 打坐
    tick(2);
    EXPECT_EQ(top(), "Cultivation") << "打坐：原来的修炼面板";
    app_.popScene();
    tick();

    ASSERT_EQ(sitAt(game::kTongdaoMap), "Tongxiu");
    const int day = state().day;
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);   // 同修 → 选人
    auto* scene = dynamic_cast<TongxiuScene*>(app_.topScene());
    ASSERT_NE(scene, nullptr);
    ASSERT_EQ(scene->mode(), TongxiuScene::Mode::Pick);
    ASSERT_EQ(scene->list().count(), 2) << "同道录里的两位人物";
    press(SDL_SCANCODE_RETURN);   // probe_a
    tick(2);
    EXPECT_EQ(state().bondOf("probe_a"), 4);
    EXPECT_EQ(state().day, day + 1) << "过了一天";
    EXPECT_EQ(state().flag(rules::kTongxiuBonusDayFlag), day + 1) << "明天第一场多 1 点劲";
    EXPECT_EQ(lastLine(), "与探针甲同修一日。默契 +4，明天第一场战斗全队多 1 点劲。");
}

// ---------------------------------------------------------------------------
// 教学卡
// ---------------------------------------------------------------------------

TEST_F(TongdaoUi, FourCardsOnlyInChapterSevenAndAtFirstUse) {
    app_.setTutorialsEnabled(true);
    openTongdaoPage();
    press(SDL_SCANCODE_RETURN);
    tick(2);
    const auto& log = app_.tutorialLog();
    EXPECT_NE(std::find(log.begin(), log.end(), "tongdao"), log.end());
    EXPECT_EQ(top(), "Tutorial") << "当场弹在菜单上";
    press(SDL_SCANCODE_RETURN);
    tick(40);
    while (top() == "Tutorial") press(SDL_SCANCODE_RETURN);
    ASSERT_EQ(top(), "Menu");
    press(SDL_SCANCODE_RETURN);   // 详情（在百药园）
    tick(2);
    EXPECT_NE(std::find(log.begin(), log.end(), "tongdao_xiangchu"), log.end());

    // 第 7 章以外不弹。
    Application other;
    ASSERT_TRUE(other.init(root_->path().string(), true).ok);
    prime(other);
    other.state().setFlag("ch07.done");
    other.setTutorialsEnabled(true);
    other.tongdaoTutorial("tongdao.open");
    EXPECT_TRUE(other.tutorialLog().empty());
    other.shutdown();
}

// ---------------------------------------------------------------------------
// 升档 / 入册提示
// ---------------------------------------------------------------------------

TEST_F(TongdaoUi, HeadlessNeverWatchesOrPushesNotices) {
    tick(2);
    const auto flags = state().flags;
    state().bonds["probe_a"].value = 45;
    state().setFlag("probe.c");
    auto expected = state().flags;
    tick(3);
    EXPECT_TRUE(app_.loreNotices().queue().empty());
    EXPECT_TRUE(app_.tongdaoMarks().empty()) << "无头不记基线";
    EXPECT_EQ(state().flags, expected) << "不立任何旗";
}

// ---------------------------------------------------------------------------
// 事件图：表
// ---------------------------------------------------------------------------

TEST(TongdaoCgTable, TheShapeIsStrict) {
    const std::string sha(64, 'a');
    auto ok = fanren::io::parseCgTable(R"({"_doc": "x", "cg": {"qiaoqian_t5": {"file": "qiaoqian_t5.png"}}, "files": {"qiaoqian_t5.png": ")" + sha + R"("}})");
    ASSERT_TRUE(ok.ok) << ok.error;
    EXPECT_EQ(ok.value.at("qiaoqian_t5"), "qiaoqian_t5.png");
    EXPECT_TRUE(fanren::io::parseCgTable(R"({"cg": {}})").ok) << "空表可以";
    for (const char* bad : {R"({})", R"({"cg": {"a": {"file": "a.jpg"}}})", R"({"cg": {"a": {"file": "x/a.png"}}})",
                            R"({"cg": {"a": {"file": "a.png", "w": 1}}})", R"({"cg": {}, "extra": 1})",
                            R"({"cg": {}, "files": {"a.png": "zz"}})", R"({"cg": {"a": "a.png"}})"}) {
        const auto parsed = fanren::io::parseCgTable(bad);
        EXPECT_FALSE(parsed.ok) << bad;
        if (!parsed.ok) EXPECT_EQ(parsed.error.rfind("cg.json", 0), 0u) << parsed.error;
    }
}

TEST(TongdaoCgStage, FadesInAndOutInPointThreeSecondsAndCrossFades) {
    game::CgStage stage;
    stage.advance("a", 0.15);
    ASSERT_EQ(stage.layers().size(), 1u);
    EXPECT_NEAR(stage.layers()[0].alpha, 0.5f, 0.01f);
    stage.advance("a", 0.2);
    EXPECT_FLOAT_EQ(stage.layers()[0].alpha, 1.f);
    stage.advance("b", 0.15);   // 换图：a 淡出、b 淡入，两层
    auto layers = stage.layers();
    ASSERT_EQ(layers.size(), 2u);
    EXPECT_EQ(layers[0].id, "a");
    EXPECT_EQ(layers[1].id, "b");
    stage.advance("", 0.3);
    stage.advance("", 0.3);
    EXPECT_TRUE(stage.empty()) << "收起之后淡完就清";
}

TEST_F(TongdaoUi, HeadlessNeverReadsTheCgTable) {
    app_.setCurrentCg("whatever");
    tick(30);
    EXPECT_FALSE(app_.cgBook().tableLoaded());
    app_.drawScenes();
    EXPECT_FALSE(app_.cgBook().tableLoaded());
}

// ---------------------------------------------------------------------------
// 有窗口（软件渲染）：比像素、提示、战斗
// ---------------------------------------------------------------------------

struct Rgb {
    int r = 0, g = 0, b = 0;
};
struct Frame {
    int w = 0, h = 0;
    std::vector<Rgb> pixels;
    [[nodiscard]] Rgb at(int x, int y) const { return pixels[static_cast<std::size_t>(y * w + x)]; }
};

constexpr SDL_Color kBustColor{220, 40, 40, 255};
constexpr SDL_Color kCgColor{30, 200, 60, 255};

class TongdaoPixels : public TongdaoUi {
protected:
    void SetUp() override {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        dir_ = fanren::test::uniqueTempPath("fanren_tongdao_px");
        fs::create_directories(dir_);
        const fs::path art = root_->path() / "assets" / "art";
        writeSolidPng(art / "busts" / "probe.png", 168, 252, kBustColor);
        writeSolidPng(art / "cg" / "probe_cg.png", 64, 36, kCgColor);
        writeFile(root_->path() / "data" / "visual" / "busts.json",
                  R"({"busts": {"probe_a_role": [{"file": "probe.png", "frame_w": 168, "frame_h": 252, "cols": 1,
                       "faces": {"normal": 0, "smile": 0, "angry": 0, "hurt": 0}}],
                     "probe_c_role": [{"file": "probe.png", "frame_w": 168, "frame_h": 252, "cols": 1,
                       "faces": {"normal": 0, "smile": 0, "angry": 0, "hurt": 0}}]}})");
        writeFile(root_->path() / "data" / "visual" / "cg.json",
                  R"({"_doc": "测试", "cg": {"probe_cg": {"file": "probe_cg.png"}}})");
        auto ready = app_.init(root_->path().string(), /*headless=*/false);
        ASSERT_TRUE(ready.ok) << ready.error;
        prime(app_);
    }
    void TearDown() override {
        app_.shutdown();
        SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
        // 真树里的立绘表与事件图表还原（别的用例不该看见测试图）。
        fs::copy_file(repoRoot() / "data" / "visual" / "busts.json", root_->path() / "data" / "visual" / "busts.json",
                      fs::copy_options::overwrite_existing);
        std::error_code ignored;
        fs::remove(root_->path() / "data" / "visual" / "cg.json", ignored);
        fs::remove_all(dir_, ignored);
    }
    Frame capture(const std::string& name) {
        Frame frame;
        app_.engine().beginFrame();
        app_.drawScenes();
        const fs::path png = dir_ / (name + ".png");
        const auto shot = app_.engine().captureFrame(png.string());
        app_.engine().endFrame();
        EXPECT_TRUE(shot.ok) << shot.error;
        if (!shot.ok) return frame;
        if (const char* keep = std::getenv("FANREN_TONGDAO_SHOTS")) {
            fs::create_directories(keep);
            fs::copy_file(png, fs::path(keep) / png.filename(), fs::copy_options::overwrite_existing);
        }
        SDL_Surface* loaded = SDL_LoadPNG(png.string().c_str());
        if (loaded == nullptr) return frame;
        SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(loaded);
        if (rgba == nullptr) return frame;
        frame.w = rgba->w;
        frame.h = rgba->h;
        for (int y = 0; y < frame.h; ++y) {
            const auto* row = static_cast<const std::uint8_t*>(rgba->pixels) + y * rgba->pitch;
            for (int x = 0; x < frame.w; ++x) frame.pixels.push_back(Rgb{row[x * 4], row[x * 4 + 1], row[x * 4 + 2]});
        }
        SDL_DestroySurface(rgba);
        return frame;
    }
    static bool near(Rgb got, SDL_Color want, int slack = 24) {
        return std::abs(got.r - want.r) <= slack && std::abs(got.g - want.g) <= slack && std::abs(got.b - want.b) <= slack;
    }
    // 主菜单右栏的正文区（MenuScene 的 kPagePanel 扣掉标题带）。
    engine::Rect pageArea() {
        const engine::Rect panel{372, 48, 860, 624};
        return fanren::ui::panelContentArea(panel, "同道录", app_.theme());
    }
    int slotOf(const std::string& id) {
        const auto entries = game::tongdaoEntries(app_);
        for (std::size_t i = 0; i < entries.size(); ++i) {
            if (entries[i].id == id) return static_cast<int>(i) % TongdaoPage::kPerPage;
        }
        return -1;
    }

    fs::path dir_;
};

TEST_F(TongdaoPixels, CardsShowTheBustForTheMetAndASilhouetteForTheUnmet) {
    app_.pushScene(std::make_unique<MenuScene>(MenuScene::Page::Roster, true));
    tick(3);
    const Frame frame = capture("u_cards");
    ASSERT_GT(frame.w, 0);
    const engine::Rect area = pageArea();
    const engine::Rect met = TongdaoPage::cardRect(area, slotOf("probe_a"));
    const engine::Rect unmet = TongdaoPage::cardRect(area, slotOf("probe_c"));
    // 画的那一块（卡片上半截）正中。
    const Rgb a = frame.at(met.x + met.w / 2, met.y + 4 + TongdaoPage::kCardArtH - 20);
    const Rgb c = frame.at(unmet.x + 30, unmet.y + 4 + TongdaoPage::kCardArtH - 20);
    EXPECT_TRUE(near(a, kBustColor)) << "遇到过：立绘原色 " << a.r << "," << a.g << "," << a.b;
    EXPECT_LT(c.r, 40) << "没遇到：剪影（压成近黑）" << c.r << "," << c.g << "," << c.b;
}

TEST_F(TongdaoPixels, CardsAndDetailBlocksStayInsideThePageWithoutOverlapping) {
    const engine::Rect area = pageArea();
    const auto inside = [&area](const engine::Rect& r) {
        return r.x >= area.x && r.y >= area.y && r.x + r.w <= area.x + area.w && r.y + r.h <= area.y + area.h;
    };
    for (int i = 0; i < TongdaoPage::kPerPage; ++i) {
        const engine::Rect a = TongdaoPage::cardRect(area, i);
        EXPECT_TRUE(inside(a)) << i;
        for (int j = i + 1; j < TongdaoPage::kPerPage; ++j) {
            const engine::Rect b = TongdaoPage::cardRect(area, j);
            const bool apart = a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y;
            EXPECT_TRUE(apart) << i << " 与 " << j;
        }
    }
    EXPECT_TRUE(inside(TongdaoPage::detailArtRect(area)));
    EXPECT_TRUE(inside(TongdaoPage::detailInfoRect(area)));
    const engine::Rect actions = TongdaoPage::actionListRect(area, app_.theme());
    EXPECT_TRUE(inside(actions));
    EXPECT_GE(actions.x, TongdaoPage::detailArtRect(area).x + TongdaoPage::detailArtRect(area).w);
    EXPECT_GE(actions.y, area.y + 146 + 4 * 50) << "动作列表在四块说明之下";
    EXPECT_TRUE(inside(TongdaoPage::giftListRect(area)));
}

TEST_F(TongdaoPixels, TheDetailPageDrawsTheBustOnTheLeft) {
    state().bonds["probe_a"].value = 50;
    auto scene = std::make_unique<MenuScene>(MenuScene::Page::Roster, true);
    scene->startTongdaoAt("probe_a");
    app_.pushScene(std::move(scene));
    tick(3);
    const Frame frame = capture("u_detail_probe_a");
    const engine::Rect art = TongdaoPage::detailArtRect(pageArea());
    EXPECT_TRUE(near(frame.at(art.x + art.w / 2, art.y + art.h - 60), kBustColor)) << "左栏是她的立绘";
    const engine::Rect info = TongdaoPage::detailInfoRect(pageArea());
    EXPECT_FALSE(near(frame.at(info.x + info.w / 2, info.y + 300), kBustColor)) << "右栏没被立绘压住";
}

TEST_F(TongdaoPixels, TierUpAndJoiningPushNoticesWithoutAnyFlag) {
    tick(2);   // 先记基线（入册的、在第几档的）
    const auto flags = state().flags;
    state().bonds["probe_a"].value = 20;
    tick();
    ASSERT_FALSE(app_.loreNotices().queue().empty());
    EXPECT_EQ(app_.loreNotices().queue().back(), "默契渐深：探针甲 · 相熟");
    state().setFlag("probe.c");
    tick();
    EXPECT_EQ(app_.loreNotices().queue().back(), "已记入同道录：探针丙");
    auto expected = flags;
    expected["probe.c"] = 1;
    EXPECT_EQ(state().flags, expected) << "提示不立旗标";
    EXPECT_TRUE(entry("probe_a")->xuhuaReady) << "升到二档、脚本在：卡片标「可叙话」";
    // 第 7 章以外不报。
    state().setFlag("ch07.done");
    tick();
    state().bonds["probe_a"].value = 45;
    tick();
    for (const std::string& line : app_.loreNotices().queue()) {
        EXPECT_EQ(line.find("默契渐深：探针甲 · 信任"), std::string::npos) << line;
    }
}

TEST_F(TongdaoPixels, TheCgFadesInUnderTheDialogueBox) {
    app_.setCurrentCg("probe_cg");
    app_.sayAs("", "ui.tongdao.title");
    tick(9);   // 0.15 秒：一半
    const Frame half = capture("u_cg_half");
    tick(30);
    const Frame full = capture("u_cg_under_dialogue");
    ASSERT_GT(full.w, 0);
    EXPECT_TRUE(near(full.at(40, 40), kCgColor)) << "整屏铺满";
    const Rgb mid = half.at(40, 40);
    EXPECT_GT(mid.g, 40);
    EXPECT_LT(mid.g, 180) << "淡入到一半";
    // 对话框压在图上：框里那一点不是图的原色。
    EXPECT_FALSE(near(full.at(640, 650), kCgColor, 10));
    // 收起：0.3 秒淡完。
    app_.setCurrentCg("");
    tick(30);
    EXPECT_TRUE(app_.cgStage().empty());
}

TEST_F(TongdaoPixels, AMissingCgWarnsOnceAndDrawsNothing) {
    ::testing::internal::CaptureStderr();
    app_.setCurrentCg("no_such_cg");
    app_.sayAs("", "ui.tongdao.title");
    tick(20);
    (void)capture("u_cg_missing_a");
    (void)capture("u_cg_missing_b");
    const std::string err = ::testing::internal::GetCapturedStderr();
    const auto first = err.find("no_such_cg");
    ASSERT_NE(first, std::string::npos) << err;
    EXPECT_EQ(err.find("no_such_cg", first + 1), std::string::npos) << "只警告一次：" << err;
}

TEST_F(TongdaoPixels, MenuShotsOfTheCushionAndTheTongdaoPageFrames) {
    // 截图口：几屏都压得上去（拍出来不是一片空），图另存一份供施工记录。
    state().addItem("material_dansha", 2);
    state().addItem("material_fuzhi", 1);
    state().bonds["probe_a"].value = 30;
    int warmup = 0;
    ASSERT_TRUE(game::pushTongdaoShot(app_, "cushion.pick", warmup));
    tick(4);
    EXPECT_EQ(app_.topScene()->name(), "Tongxiu");
    (void)capture("u_cushion_pick");
    app_.popScene();
    tick();
    ASSERT_TRUE(game::pushTongdaoShot(app_, "gift=probe_a", warmup));
    tick(4);
    (void)capture("u_gift_probe_a");
    EXPECT_FALSE(game::pushTongdaoShot(app_, "nonsense", warmup));
}

// 战斗：五种事件的截图口推得到那一刻，持续标记跟着事件走。
class TongdaoBattleShots : public TongdaoPixels {
protected:
    game::BattleScene* shoot(game::BattleShot shot) {
        state().party.clear();
        state().setFlag("probe.aide");
        (void)state().partyAdd("probe_aide_role");
        app_.pushScene(std::make_unique<game::BattleScene>("probe_support", shot));
        tick(4);
        return dynamic_cast<game::BattleScene*>(app_.topScene());
    }
};

TEST_F(TongdaoBattleShots, WardAndWardBlockMarkThenClearTheAlly) {
    game::BattleScene* scene = shoot(game::BattleShot::Ward);
    ASSERT_NE(scene, nullptr);
    ASSERT_EQ(scene->lastPlayed(), fanren::core::battle::BattleEventKind::Ward);
    bool anyWarded = false;
    for (const auto& s : scene->shown()) anyWarded = anyWarded || s.warded;
    EXPECT_TRUE(anyWarded) << "挂上护持：标记亮着";
    (void)capture("u_battle_ward");
}

TEST_F(TongdaoBattleShots, EachOfTheFiveMomentsIsReachedAndShot) {
    using K = fanren::core::battle::BattleEventKind;
    const std::vector<std::pair<game::BattleShot, K>> moments{{game::BattleShot::Ward, K::Ward},
                                                              {game::BattleShot::WardBlock, K::WardBlock},
                                                              {game::BattleShot::Bulwark, K::Bulwark},
                                                              {game::BattleShot::Bind, K::Bind},
                                                              {game::BattleShot::BoundSkip, K::BoundSkip}};
    const char* names[] = {"u_battle_ward", "u_battle_wardblock", "u_battle_bulwark", "u_battle_bind",
                           "u_battle_boundskip"};
    for (std::size_t i = 0; i < moments.size(); ++i) {
        game::BattleScene* scene = shoot(moments[i].first);
        ASSERT_NE(scene, nullptr) << i;
        EXPECT_EQ(scene->lastPlayed(), moments[i].second) << i;
        bool marked = false;
        int blocked = -1;
        for (const auto& ev : scene->battle().events()) {
            if (ev.kind == K::WardBlock) blocked = ev.target;
        }
        for (const auto& s : scene->shown()) {
            if (moments[i].second == K::Ward) marked = marked || s.warded;
            if (moments[i].second == K::Bind) marked = marked || s.bound;
            if (moments[i].second == K::Bulwark) marked = marked || s.bulwark;
            if (moments[i].second == K::WardBlock || moments[i].second == K::BoundSkip) marked = true;
        }
        EXPECT_TRUE(marked) << i;
        if (moments[i].second == K::WardBlock) {
            ASSERT_GE(blocked, 0);
            EXPECT_FALSE(scene->shown()[static_cast<std::size_t>(blocked)].warded) << "挡下之后那一层护持散了";
        }
        if (moments[i].second == K::BoundSkip) {
            for (const auto& s : scene->shown()) EXPECT_FALSE(s.bound) << "轮过之后定身散了";
        }
        (void)capture(names[i]);
        app_.popScene();
        tick(2);
    }
}

}  // namespace
