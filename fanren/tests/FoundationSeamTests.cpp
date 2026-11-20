// 第一期地基包集成：三路各自看不见的接缝（docs/ch07-foundation-integration.md 第 3 节）。
//   · 整段跳过（路 D）途中解锁的见闻，跳完之后「见闻：XXX」（路 G）照常报；
//   · 整段跳过途中脚本排的教学卡（路 G），脚本结束后照常弹。
// 状态取第 7 章章首存档（教学卡只在第 7 章弹），脚本写进临时资产根的 scripts/test_seam/ 下，按键走 SDL 事件。
#include <gtest/gtest.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <string>

#include "TempDir.h"
#include "core/rules/Tutorials.h"
#include "game/Application.h"
#include "game/BoardScene.h"
#include "game/DialogueExtras.h"
#include "game/ItemToasts.h"
#include "game/LoreNotices.h"
#include "game/WorldScene.h"
#include "io/SaveFile.h"

namespace fs = std::filesystem;
using fanren::game::Application;

namespace {

std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

void pushKey(SDL_Scancode scancode, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.down = down;
    event.key.repeat = false;
    SDL_PushEvent(&event);
}

// 跳过的这一段：两句、置一面解锁见闻「岳麓殿的门禁」的旗、叫一张教学卡、再两句。
constexpr const char* kLoreFlag = "ch07.yuelu";
constexpr const char* kLoreTitleKey = "lore.ch07.yuelu_ru_menjin.title";
constexpr const char* kCard = "alchemy";

class FoundationSeam : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_foundation_seam_root");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(fs::path(repoRoot()) / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        fs::create_directories(root_->path() / "scripts" / "test_seam");
        std::ofstream(root_->path() / "scripts" / "test_seam" / "skip_unlock.lua", std::ios::binary)
            << "talk(\"\", \"ui.dialogue.skip.ask\")\n"
               "talk(\"\", \"ui.dialogue.skip.yes\")\n"
               "flag.set(\"" << kLoreFlag << "\", 1)\n"
               "tutorial(\"" << kCard << "\")\n"
               "talk(\"\", \"ui.dialogue.skip.no\")\n"
               "talk(\"\", \"ui.dialogue.auto\")\n";
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        auto save = fanren::io::loadGame((fs::path(repoRoot()) / "saves" / "ch07-start.sav").string());
        ASSERT_TRUE(save.ok) << save.error;
        app_.state() = save.value;
        ASSERT_EQ(app_.state().flag(kLoreFlag), 0) << "先验：章首还没解锁这条见闻";
        ASSERT_TRUE(app_.loadMap(app_.state().mapId, std::string{}).ok);
        app_.pushScene(std::make_unique<fanren::game::WorldScene>());
        tick(2);
        ASSERT_EQ(top(), "World");
        // 读档那一刻就有的见闻只记不报；保险起见把队里可能已有的提示清掉，只看这一段新报的。
        app_.loreNotices().clear();
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
    void startAndSkip() {
        const auto started = app_.startEvent("test_seam/skip_unlock.lua");
        ASSERT_TRUE(started.ok) << started.error;
        tick(2);
        ASSERT_EQ(top(), "Dialogue");
        press(SDL_SCANCODE_ESCAPE);   // 跳过确认框
        press(SDL_SCANCODE_RETURN);   // 默认停在「跳过」
    }

    static std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
};

std::unique_ptr<fanren::test::TempDir> FoundationSeam::root_;

}  // namespace

TEST_F(FoundationSeam, LoreUnlockedDuringASkipIsStillAnnouncedAfterIt) {
    startAndSkip();
    tick(3);
    ASSERT_FALSE(app_.scripts().isRunning()) << "一路跳到脚本结束";
    EXPECT_EQ(app_.state().flag(kLoreFlag), 1) << "跳过途中 flag.set 照常";
    EXPECT_TRUE(app_.skipNotice().holding()) << "「已跳过 N 句」照弹";
    ASSERT_TRUE(app_.loreNotices().active()) << "跳过途中解锁的见闻，跳完照样报";
    EXPECT_EQ(app_.loreNotices().current(),
              app_.data().lookupText("ui.guide.lore.unlocked") + app_.data().lookupText(kLoreTitleKey));
    // 提示的钟只在世界层露着时走：跳完回到地图上，它淡得出来、也走得完。
    tick(5 * 60);
    EXPECT_FALSE(app_.loreNotices().active());
}

TEST_F(FoundationSeam, ATutorialQueuedDuringASkipPopsOnceTheScriptEnds) {
    // 只留这一张卡没看过：别的卡（走图时的「键位」、名册、负伤……）不来插队。
    for (const fanren::core::TutorialCard& card : app_.data().tutorials) {
        if (card.id != kCard) app_.state().flags[fanren::rules::tutorialSeenFlag(card.id)] = 1;
    }
    app_.setTutorialsEnabled(true);
    tick(2);
    ASSERT_EQ(top(), "World");
    ASSERT_TRUE(app_.tutorialLog().empty());

    startAndSkip();
    // 跳到脚本结束、回到走图：卡片这时才弹，压在世界层上。
    tick(3);
    ASSERT_FALSE(app_.scripts().isRunning());
    const auto& log = app_.tutorialLog();
    EXPECT_NE(std::find(log.begin(), log.end(), std::string(kCard)), log.end()) << "跳过途中排的卡，脚本结束后照弹";
    EXPECT_EQ(top(), "Tutorial");
    EXPECT_EQ(app_.state().flag(fanren::rules::tutorialSeenFlag(kCard)), 1);
}

// 屏幕上沿同屏的几样（第 3 节第 1 条）：「已跳过 N 句」碰上宽到正中的目标框时往右让开，不压它；
// 目标框窄、或上下不交叠时仍在正中。
TEST_F(FoundationSeam, TheSkipNoticeStepsRightOfAWideObjectiveColumn) {
    fanren::engine::Engine& engine = app_.engine();
    const std::string text = "已跳过 12 句 · 按 Tab 回看";
    const fanren::engine::Rect centered = fanren::game::skipNoticeArea(engine, text);
    EXPECT_EQ(centered.x, (fanren::engine::kLogicalWidth - centered.w) / 2);

    const fanren::engine::RectF narrow{16.f, 88.f, 316.f, 90.f};
    EXPECT_EQ(fanren::game::skipNoticeArea(engine, text, narrow).x, centered.x) << "窄框够不着正中，不用让";

    const fanren::engine::RectF wide{16.f, 88.f, 560.f, 90.f};   // 右沿 576，与提示（120 起）上下交叠
    const fanren::engine::Rect moved = fanren::game::skipNoticeArea(engine, text, wide);
    EXPECT_GE(moved.x, 576 + 12) << "让到目标框右边";
    EXPECT_EQ(moved.y, centered.y);

    const fanren::engine::RectF below{16.f, 200.f, 560.f, 60.f};   // 上下不交叠
    EXPECT_EQ(fanren::game::skipNoticeArea(engine, text, below).x, centered.x);
}

// 得物卡片的上沿可以往下挪（存盘提示亮着时排到它下面）；缺省仍从小地图下面 172 起。
TEST(FoundationSeamLayout, ItemToastsStartWhereTheyAreToldAndDefaultBelowTheMinimap) {
    EXPECT_FLOAT_EQ(fanren::game::itemToastRect(0).y, fanren::game::kItemToastTop);
    EXPECT_FLOAT_EQ(fanren::game::itemToastRect(0, 220.f).y, 220.f);
    EXPECT_GT(fanren::game::itemToastRect(1, 220.f).y, 220.f + fanren::game::itemToastRect(0, 220.f).h);
}

// 记事「见闻」页装下合并后的 28 条（第 3 节第 3 条）：四类都有、列表翻页，翻页时「n/28」刻度单占一行，
// 不与最后一行叠字（少排一行让给它）；右栏每一条的标题与正文都在。
TEST_F(FoundationSeam, TheLorePageHoldsAllTwentyEightAndLeavesARowForTheCounter) {
    using fanren::game::BoardScene;
    std::set<std::string> categories;
    // 只解锁一条：一页放得下，页长就是整块区域排得下的行数。
    app_.state().flags[kLoreFlag] = 1;
    BoardScene few(BoardScene::Page::Lore);
    few.onEnter(app_);
    ASSERT_EQ(few.list().count(), 1);
    const int fullPage = few.list().pageSize();

    for (const fanren::core::LoreEntry& entry : app_.data().lore) {
        app_.state().flags[entry.unlockFlag] = 1;
        if (!entry.roleId.empty()) {
            if (const auto* role = app_.data().findRole(entry.roleId)) app_.state().learnWeaknesses(role->id, role->weaknesses);
        }
        categories.insert(entry.category);
    }
    EXPECT_EQ(categories.size(), 4u) << "妖兽、灵草、人物、所闻四类都有";
    BoardScene all(BoardScene::Page::Lore);
    all.onEnter(app_);
    ASSERT_EQ(all.list().count(), 28) << "路 G 5 条 + 路 D 23 条";
    EXPECT_EQ(all.list().pageSize(), fullPage - 1) << "超过一页：少排一行给「n/28」刻度";
    EXPECT_LT(all.list().pageSize(), all.list().count());
    for (int row = 0; row < all.list().count(); ++row) {
        BoardScene at(BoardScene::Page::Lore, row);   // 光标停在第 row 条（截图口同一个入口）
        at.onEnter(app_);
        ASSERT_EQ(at.list().selection(), row);
        const std::vector<std::string> pane = at.paneLines(app_);
        ASSERT_GE(pane.size(), 2u) << "第 " << row << " 条";
        EXPECT_FALSE(pane[0].empty());
        EXPECT_FALSE(pane[1].empty());
    }
}
