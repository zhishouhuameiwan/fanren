#include "game/ChapterCardScene.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <system_error>
#include <utility>

#include "game/Application.h"
#include "io/ChapterLoader.h"
#include "script/Command.h"
#include "ui/Widgets.h"

namespace fanren::game {
namespace {

constexpr float kW = static_cast<float>(engine::kLogicalWidth);
constexpr float kH = static_cast<float>(engine::kLogicalHeight);
constexpr float kCenterX = kW / 2.f;

// 饰线从中间往两边长：淡入时从三成宽长到全宽，淡出时再收回去——卡片「展开」「合上」
// 的那一下动作全靠它，字本身不动。
constexpr float kOrnamentHalf = 260.f;
constexpr float kOrnamentFloor = 0.35f;

// 大字的字距。六十像素的方块字挨着排是一堵墙，拉开之后才像卷轴上的题字。
constexpr int kHeadlineSpacing = 14;
constexpr int kKickerSpacing = 10;

// 章节卡的音效（docs/audio.md 第 4 节）。缺文件时引擎静默，不影响卡片本身。
constexpr const char* kCardSfx = "chapter_card";

[[nodiscard]] float easeOut(float t) {
    const float k = std::clamp(t, 0.f, 1.f);
    return 1.f - (1.f - k) * (1.f - k);
}

}  // namespace

// ---------------------------------------------------------------------------
// 章节表
// ---------------------------------------------------------------------------

const ChapterEntry* ChapterTable::find(int number) const {
    for (const ChapterEntry& entry : chapters) {
        if (entry.number == number) return &entry;
    }
    return nullptr;
}

bool ChapterTable::chapterDone(const core::GameState& state, int number) const {
    const ChapterEntry* entry = find(number);
    return entry != nullptr && state.flag(entry->doneFlag) != 0;
}

core::Result<ChapterTable> loadChapterTable(const std::string& path, const core::GameData& data) {
    namespace fs = std::filesystem;
    using R = core::Result<ChapterTable>;
    std::error_code ec;
    const bool present = fs::exists(path, ec);
    if (ec) return R::failure("查不了章节表在不在：" + path + "（" + ec.message() + "）");
    if (!present) return R::success(ChapterTable{});

    // 读与查（缺字段、章号不升序）在 io 层；这里只把文案 key 换成正文。
    const core::Result<io::ChapterFile> file = io::loadChapterFile(path);
    if (!file) return R::failure(file.error);

    ChapterTable table;
    table.closingWord = data.lookupText(file.value.closingKey);
    table.toBeContinued = data.lookupText(file.value.toBeContinuedKey);
    for (const io::ChapterRow& row : file.value.chapters) {
        ChapterEntry entry;
        entry.number = row.number;
        entry.numeral = data.lookupText(row.numeralKey);
        entry.title = data.lookupText(row.titleKey);
        entry.doneFlag = row.doneFlag;
        table.chapters.push_back(std::move(entry));
    }
    return R::success(std::move(table));
}

// ---------------------------------------------------------------------------
// 排程
// ---------------------------------------------------------------------------

std::vector<CardRequest> cardsForFlagChange(const ChapterTable& table, const std::string& flag,
                                            int before, int after) {
    // 只认「从没演完到演完」这一下。读档（旗标本来就是 1）、脚本重复置、清回 0，
    // 都不是「这一章刚刚演完」。
    if (before != 0 || after == 0) return {};
    for (std::size_t i = 0; i < table.chapters.size(); ++i) {
        if (table.chapters[i].doneFlag != flag) continue;
        std::vector<CardRequest> cards{{CardKind::Closing, table.chapters[i].number}};
        if (i + 1 < table.chapters.size()) {
            cards.push_back({CardKind::Opening, table.chapters[i + 1].number});
        } else {
            cards.push_back({CardKind::ToBeContinued, 0});
        }
        return cards;
    }
    return {};
}

std::vector<CardRequest> openingCards(const ChapterTable& table) {
    if (table.chapters.empty()) return {};
    return {{CardKind::Opening, table.chapters.front().number}};
}

Card resolveCard(const ChapterTable& table, const CardRequest& request) {
    Card card;
    card.kind = request.kind;
    const ChapterEntry* entry = table.find(request.chapter);
    // **不要加 default:**：新增一种卡时让编译器告警，而不是悄悄画成空卡。
    switch (request.kind) {
        case CardKind::Opening:
            if (entry != nullptr) {
                card.kicker = entry->numeral;
                card.headline = entry->title;
            }
            break;
        case CardKind::Closing:
            if (entry != nullptr) {
                card.kicker = entry->title;
                card.headline = entry->numeral + "　" + table.closingWord;
            }
            break;
        case CardKind::ToBeContinued:
            card.headline = table.toBeContinued;
            break;
        case CardKind::Ending:
            // 结局卡的字由脚本给（ChapterCardScene::ending），不经章节表。
            break;
    }
    return card;
}

// ---------------------------------------------------------------------------
// 场景
// ---------------------------------------------------------------------------

ChapterCardScene::ChapterCardScene(std::vector<Card> cards) : cards_(std::move(cards)) {}

std::unique_ptr<ChapterCardScene> ChapterCardScene::ending(std::string title, std::string body) {
    Card card;
    card.kind = CardKind::Ending;
    card.kicker = std::move(title);
    card.body = std::move(body);
    std::vector<Card> cards;
    cards.push_back(std::move(card));
    auto scene = std::make_unique<ChapterCardScene>(std::move(cards));
    scene->waitsForConfirm_ = true;
    scene->hold_ = -1.0;
    return scene;
}

float ChapterCardScene::visibilityAt(double t, double hold) {
    if (t <= 0.0) return 0.f;
    if (t < kFadeInSeconds) return static_cast<float>(t / kFadeInSeconds);
    if (hold < 0.0) return 1.f;
    const double out = t - (kFadeInSeconds + hold);
    if (out <= 0.0) return 1.f;
    if (out >= kFadeOutSeconds) return 0.f;
    return static_cast<float>(1.0 - out / kFadeOutSeconds);
}

void ChapterCardScene::onEnter(Application&) {
    current_ = 0;
    t_ = 0.0;
    started_ = false;
}

void ChapterCardScene::startCard(Application& app) {
    app.engine().playSfx(kCardSfx);
    started_ = true;
}

bool ChapterCardScene::update(Application& app, double deltaSeconds) {
    if (current_ >= cards_.size()) return false;
    // 无头模式没人看：章节卡不停留，第一帧就退场，不挡脚本也不挡场景栈。
    // 结局卡是一条脚本命令，照对话框的规矩等驱动方回填，不在这里替它结束。
    if (app.headless() && !waitsForConfirm_) return false;
    if (!started_) startCard(app);
    t_ += deltaSeconds;

    // 确认键：还没开始淡出就从当前的亮度接着淡出（不是一下子黑掉，也不是先跳回全亮）。
    // 结局卡要先完全显出来才收确认：前一句对话的确认键余势不该把它一下跳掉。
    const bool fadingOut = hold_ >= 0.0 && t_ >= kFadeInSeconds + hold_;
    const bool acceptsConfirm = !waitsForConfirm_ || t_ >= kFadeInSeconds;
    if (!fadingOut && acceptsConfirm &&
        app.engine().keyPressed(engine::Engine::Key::Confirm)) {
        const float shown = visibilityAt(t_, hold_);
        hold_ = 0.0;
        t_ = kFadeInSeconds + (1.0 - shown) * kFadeOutSeconds;
    }
    if (hold_ < 0.0 || t_ < kFadeInSeconds + hold_ + kFadeOutSeconds) return true;

    // 这一张播完了。
    if (waitsForConfirm_) {
        script::CommandResult result;
        result.ok = true;
        app.completeCommand(result);
        return false;
    }
    ++current_;
    t_ = 0.0;
    hold_ = kHoldSeconds;
    started_ = false;
    return current_ < cards_.size();
}

void ChapterCardScene::render(Application& app) {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const engine::RectF screen{0.f, 0.f, kW, kH};
    e.fillRect(screen, theme.inkDeep, engine::BlendMode::Alpha);
    if (current_ >= cards_.size()) return;

    const float reveal = visibilityAt(t_, hold_);
    drawCard(app, cards_[current_], reveal);
    // 淡入淡出靠一层黑纱，不靠逐帧改字的透明度：字色逐帧变，引擎就逐帧建一张新纹理
    // （docs/interfaces-octo-engine.md 2.5）。
    const auto veil = static_cast<std::uint8_t>(std::lround((1.f - reveal) * 255.f));
    if (veil > 0) e.fillRect(screen, ui::withAlpha(theme.inkDeep, veil), engine::BlendMode::Alpha);
}

void ChapterCardScene::drawCard(Application& app, const Card& card, float reveal) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const float grow = kOrnamentFloor + (1.f - kOrnamentFloor) * easeOut(reveal);

    switch (card.kind) {
        case CardKind::Opening:
            ui::drawSpacedText(e, card.kicker, kCenterX, 262, 32, theme.goldBright, theme.titleStyle,
                               kKickerSpacing);
            ui::drawOrnament(e, kCenterX, 318.f, kOrnamentHalf * grow, theme);
            ui::drawSpacedText(e, card.headline, kCenterX, 344, 60, theme.paper, theme.titleStyle,
                               kHeadlineSpacing);
            break;
        case CardKind::Closing:
            ui::drawSpacedText(e, card.kicker, kCenterX, 270, 26, theme.paperDim, theme.bodyStyle,
                               kKickerSpacing);
            ui::drawOrnament(e, kCenterX, 318.f, kOrnamentHalf * grow, theme);
            ui::drawSpacedText(e, card.headline, kCenterX, 344, 56, theme.goldBright,
                               theme.titleStyle, kHeadlineSpacing);
            break;
        case CardKind::ToBeContinued:
            ui::drawSpacedText(e, card.headline, kCenterX, 300, 56, theme.paper, theme.titleStyle,
                               kHeadlineSpacing + 6);
            ui::drawOrnament(e, kCenterX, 392.f, kOrnamentHalf * grow, theme);
            break;
        case CardKind::Ending: {
            ui::drawSpacedText(e, card.kicker, kCenterX, 150, 44, theme.goldBright,
                               theme.titleStyle, kKickerSpacing);
            ui::drawOrnament(e, kCenterX, 222.f, kOrnamentHalf * grow, theme);
            ui::drawTextBlock(e, card.body, engine::Rect{240, 246, 800, 360}, 24, theme);
            // 提示只在完全显出来、正等着确认时出现：淡入途中就摆出来像是在催人。
            if (waitsForConfirm_ && hold_ < 0.0 && reveal >= 1.f) {
                ui::drawSpacedText(e, app.text("ui.ending.continue_hint"), kCenterX, 640, 18,
                                   theme.paperDim, theme.bodyStyle, 2);
            }
            break;
        }
    }
}

}  // namespace fanren::game
