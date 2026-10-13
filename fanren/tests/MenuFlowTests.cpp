#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

#include "TempDir.h"
#include "game/Application.h"
#include "game/DialogueScene.h"
#include "game/MenuScene.h"

namespace {

using fanren::game::Application;
using fanren::game::MenuScene;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (std::filesystem::exists(std::filesystem::path(candidate) / "data/text/ui.json")) {
            return candidate;
        }
    }
    return ".";
}

std::string fileBytes(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

class MenuFlow : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_menu_flow");
        for (const char* dir : {"data", "scripts", "maps"}) {
            std::filesystem::copy(std::filesystem::path(assetRoot()) / dir, root_->path() / dir,
                                  std::filesystem::copy_options::recursive);
        }
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        const auto ready = app_.init(root_->path().string(), true);
        ASSERT_TRUE(ready.ok) << ready.error;
        ASSERT_EQ(std::filesystem::path(app_.quickSavePath()).parent_path().parent_path(), root_->path());
        const auto started = app_.startNewJourney();
        ASSERT_TRUE(started.ok) << started.error;
        tick();
        ASSERT_EQ(top(), "World");
    }

    void TearDown() override { app_.shutdown(); }
    void tick() { app_.tick(1.0 / 60.0); }
    std::string top() { return app_.topScene() == nullptr ? "" : app_.topScene()->name(); }

    void press(SDL_Scancode key) {
        for (const bool down : {true, false}) {
            SDL_Event event{};
            event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
            event.key.scancode = key;
            event.key.down = down;
            ASSERT_TRUE(SDL_PushEvent(&event)) << SDL_GetError();
            tick();
        }
    }

    void pressPad(Uint8 button) {
        for (const bool down : {true, false}) {
            SDL_Event event{};
            event.type = down ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
            event.gbutton.which = 901;
            event.gbutton.button = button;
            event.gbutton.down = down;
            ASSERT_TRUE(SDL_PushEvent(&event)) << SDL_GetError();
            tick();
        }
    }

    void openHome() {
        app_.pushScene(std::make_unique<MenuScene>(MenuScene::Page::Home));
        tick();
        ASSERT_EQ(top(), "Menu");
    }

    void returnHome() {
        openHome();
        press(SDL_SCANCODE_RETURN);
        ASSERT_EQ(top(), "Menu");
        press(SDL_SCANCODE_DOWN);
        press(SDL_SCANCODE_RETURN);
        ASSERT_EQ(top(), "Title");
    }

    Application app_;
    inline static std::unique_ptr<fanren::test::TempDir> root_;
};

TEST_F(MenuFlow, HomeIsReachableFromTheWorldMenuAndDefaultsToCancel) {
    EXPECT_EQ(MenuScene::pageLabel(app_.data(), app_.state(), MenuScene::Page::Home), "返回首页");
    press(SDL_SCANCODE_TAB);
    ASSERT_EQ(top(), "Menu");
    // 状态、物品、法门、记事、存盘、设置、返回首页、返回。
    for (int i = 0; i < 6; ++i) press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(top(), "Menu") << "确认界面默认取消，连按确认不能放弃进度";
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(top(), "World") << "取消后回到命令列表，下一项仍是关闭菜单的返回";
}

TEST_F(MenuFlow, EscapeCancelsAndReopeningResetsTheConfirmation) {
    app_.state().setFlag("menu.unsaved", 1);
    openHome();
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_ESCAPE);
    ASSERT_EQ(top(), "Menu");
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(top(), "Menu");
    EXPECT_EQ(app_.state().flag("menu.unsaved"), 1);
    press(SDL_SCANCODE_ESCAPE);
    EXPECT_EQ(top(), "World");
}

TEST_F(MenuFlow, ConfirmingHomeReplacesTheEntireSceneStack) {
    returnHome();
    for (int i = 0; i < 5; ++i) tick();
    EXPECT_EQ(top(), "Title");
    EXPECT_FALSE(app_.quitRequested());
    app_.popScene();
    tick();
    EXPECT_EQ(app_.topScene(), nullptr) << "首页底下不应残留世界或菜单";
}

TEST_F(MenuFlow, TheOriginalBackCommandStillReturnsToTheGame) {
    app_.pushScene(std::make_unique<MenuScene>(MenuScene::Page::Back));
    tick();
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(top(), "World");
}

TEST_F(MenuFlow, TheGamepadCanCancelAndConfirmHome) {
    openHome();
    pressPad(SDL_GAMEPAD_BUTTON_SOUTH);
    pressPad(SDL_GAMEPAD_BUTTON_EAST);
    EXPECT_EQ(top(), "Menu");
    pressPad(SDL_GAMEPAD_BUTTON_SOUTH);
    pressPad(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    pressPad(SDL_GAMEPAD_BUTTON_SOUTH);
    EXPECT_EQ(top(), "Title");
}

TEST_F(MenuFlow, ReturningHomeDoesNotOverwriteSavesAndContinueLoadsSavedProgress) {
    const std::string savePath = app_.quickSavePath();
    app_.state().setFlag("menu.saved", 1);
    app_.state().position = {5, 6};
    const auto saved = app_.quickSave();
    ASSERT_TRUE(saved.ok) << saved.error;
    const std::string savedBytes = fileBytes(savePath);
    ASSERT_FALSE(savedBytes.empty());
    app_.state().setFlag("menu.saved", 2);
    app_.state().position = {8, 9};
    returnHome();
    EXPECT_EQ(fileBytes(savePath), savedBytes);
    for (int i = 0; i < 40; ++i) tick();
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(top(), "World");
    EXPECT_EQ(app_.state().flag("menu.saved"), 1);
    EXPECT_EQ(app_.state().position.x, 5);
    EXPECT_EQ(app_.state().position.y, 6);
}

TEST_F(MenuFlow, ANewJourneyClearsOldProgressButKeepsSettings) {
    app_.state().setFlag("menu.old_game", 1);
    auto settings = app_.settings();
    settings.bgmVolume = 3;
    app_.setSettings(settings);
    returnHome();
    for (int i = 0; i < 40; ++i) tick();
    press(SDL_SCANCODE_RETURN);
    for (int i = 0; i < 60; ++i) tick();
    EXPECT_EQ(top(), "World");
    EXPECT_EQ(app_.state().mapId, "ch01_hanjiacun");
    EXPECT_EQ(app_.state().flag("menu.old_game"), 0);
    EXPECT_EQ(app_.settings().bgmVolume, 3);
}

TEST_F(MenuFlow, HomeClearsPendingScenesScriptsAndSessionState) {
    auto* shop = app_.shop("ch02_wairentang_yaoshang");
    ASSERT_NE(shop, nullptr);
    ASSERT_FALSE(shop->entries.empty());
    const int stock = shop->entries.front().stock;
    shop->entries.front().stock = 0;
    shop->entries.front().lastRestockDay = 99;
    app_.pushDialogueLine("韩立", "上一局");
    app_.setScreenFade(1.f);
    const auto started = app_.startEvent("ch01/sanshu.lua");
    ASSERT_TRUE(started.ok) << started.error;
    tick();
    ASSERT_TRUE(app_.scripts().isRunning());
    ASSERT_TRUE(app_.awaitingCommand());
    app_.pushScene(std::make_unique<fanren::game::DialogueScene>("", "待压场景"));
    app_.returnToTitle();
    tick();
    EXPECT_EQ(top(), "Title");
    EXPECT_FALSE(app_.scripts().isRunning());
    EXPECT_FALSE(app_.awaitingCommand());
    EXPECT_TRUE(app_.dialogueLog().empty());
    EXPECT_TRUE(app_.spokenKeys().empty());
    EXPECT_TRUE(app_.pendingCards().empty());
    EXPECT_FLOAT_EQ(app_.screenFade(), 0.f);
    ASSERT_NE(app_.shop("ch02_wairentang_yaoshang"), nullptr);
    EXPECT_EQ(app_.shop("ch02_wairentang_yaoshang")->entries.front().stock, stock);
    EXPECT_EQ(app_.shop("ch02_wairentang_yaoshang")->entries.front().lastRestockDay, 0);
}

}  // namespace
