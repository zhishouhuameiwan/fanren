#include "game/ItemToasts.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "game/BattleHud.h"
#include "game/ShopScene.h"
#include "game/Wording.h"
#include "game/WorldHud.h"

namespace fanren::game {
namespace {

using engine::BlendMode;
using engine::Color;
using engine::RectF;

// UTF-8 一个码点占几个字节（看首字节）。不是合法首字节的按 1，免得一个坏字节卡死循环。
std::size_t glyphBytes(unsigned char lead) {
    if (lead < 0x80) return 1;
    if (lead >= 0xF0) return 4;
    if (lead >= 0xE0) return 3;
    if (lead >= 0xC0) return 2;
    return 1;
}

// 卡片的版式（逻辑像素，1280×720）。
constexpr float kCardW = 380.f;
constexpr float kCardH = 60.f;
constexpr float kCardGap = 8.f;
constexpr float kCardTop = 132.f;     // 右上角的存盘提示之下
constexpr float kCardRight = 20.f;
constexpr float kSlide = 24.f;        // 淡入时从右边滑进来这么远
constexpr float kIconBox = 36.f;
constexpr int kNameFont = 20;
constexpr int kCountFont = 17;
constexpr int kNoteFont = 14;
// 说明那一行放得下多少个字：卡片宽减去图标与边距约 300 像素，14 号字一个汉字约 14 像素。
constexpr std::size_t kNoteGlyphs = 21;
constexpr float kCoveringAlpha = 0.35f;   // 盖住主角时淡成这样（与目标框同一个数，WorldView::drawHud）

// 不算「收起」时，出场 age 秒的浓度：排队 0 → 淡入 → 停 → 淡出。
float timelineAlpha(float age) {
    if (age < 0.f) return 0.f;
    if (age < ItemToasts::kFadeIn) return age / ItemToasts::kFadeIn;
    const float out = age - ItemToasts::kFadeIn - ItemToasts::kHold;
    if (out <= 0.f) return 1.f;
    return std::clamp(1.f - out / ItemToasts::kFadeOut, 0.f, 1.f);
}

// 文字的浓度按十分之一取整：逐帧改文字颜色会逐帧新建文字纹理（docs/interfaces-octo-engine.md 2.5），
// 一张卡片淡入淡出一次至多十来张。
float stepped(float alpha) {
    return std::round(std::clamp(alpha, 0.f, 1.f) * 10.f) / 10.f;
}

engine::TextStyle fadedStyle(engine::TextStyle style, float alpha) {
    style.shadowColor = hud::faded(style.shadowColor, alpha);
    style.outlineColor = hud::faded(style.outlineColor, alpha);
    return style;
}

bool overlaps(const RectF& a, const engine::Rect& b) {
    if (b.w <= 0 || b.h <= 0) return false;
    const auto bx = static_cast<float>(b.x);
    const auto by = static_cast<float>(b.y);
    return a.x < bx + static_cast<float>(b.w) && bx < a.x + a.w && a.y < by + static_cast<float>(b.h) &&
           by < a.y + a.h;
}

}  // namespace

int gradeTier(int grade) {
    if (grade <= 1) return 0;
    if (grade <= 3) return 1;
    if (grade <= 5) return 2;
    if (grade <= 7) return 3;
    return 4;
}

std::string firstSentence(const std::string& text, std::size_t maxGlyphs) {
    static const std::string kStops[] = {"。", "！", "？"};
    std::size_t end = text.size();
    for (const std::string& stop : kStops) end = std::min(end, text.find(stop));
    std::vector<std::size_t> starts;   // 每个码点的起始字节
    for (std::size_t at = 0; at < end; at += glyphBytes(static_cast<unsigned char>(text[at]))) starts.push_back(at);
    if (starts.size() <= maxGlyphs) return text.substr(0, end);
    if (maxGlyphs == 0) return {};
    return text.substr(0, starts[maxGlyphs - 1]) + "…";
}

BattleIcon itemIcon(const core::Item* item, const std::string& itemId) {
    if (itemId == kSpiritStoneItemId) return BattleIcon::Silver;
    if (item == nullptr) return BattleIcon::Chest;
    using core::ItemKind;
    switch (item->kind) {
        case ItemKind::Pill: return item->poison > 0 ? BattleIcon::Poison : BattleIcon::Medicine;
        case ItemKind::Herb: return BattleIcon::Wood;
        case ItemKind::Material: return BattleIcon::Earth;
        case ItemKind::Talisman: return BattleIcon::Stance;
        case ItemKind::Armor: return BattleIcon::Stance;
        case ItemKind::Artifact: return item->weapon != 0 ? categoryIcon(item->weapon) : BattleIcon::Metal;
        case ItemKind::Trinket: return BattleIcon::BeadFull;
        case ItemKind::Manual: return BattleIcon::Cultivation;
        case ItemKind::Story: return BattleIcon::Chest;
    }
    return BattleIcon::Chest;
}

ItemToast makeItemToast(const core::GameData& data, const core::GameState& state, const std::string& itemId,
                        int count) {
    ItemToast card;
    card.itemId = itemId;
    card.count = std::max(1, count);
    const core::Item* item = data.findItem(itemId);
    card.icon = itemIcon(item, itemId);
    if (itemId == kSpiritStoneItemId) {
        // 钱：名字跟着阶段走（第 1–5 章是碎银），不写说明——灵石那句说明里有「灵气」，凡人篇说不得。
        card.name = currencyName(state);
        card.tier = 0;
        return card;
    }
    card.name = item != nullptr && !item->name.empty() ? item->name : itemId;
    card.tier = item != nullptr ? gradeTier(item->grade) : 0;
    if (item != nullptr && !item->descKey.empty()) {
        const auto it = data.text.find(item->descKey);
        if (it != data.text.end()) card.note = firstSentence(it->second, kNoteGlyphs);
    }
    return card;
}

// ---------------------------------------------------------------------------
// ItemToasts
// ---------------------------------------------------------------------------

void ItemToasts::push(ItemToast toast) {
    // 一次给几样：后一张比前一张晚 kStagger 出场；前一张早就出场了的，这一张立刻出场。
    toast.age = cards_.empty() ? 0.f : std::min(0.f, cards_.back().age - kStagger);
    toast.dismissed = false;
    cards_.push_back(std::move(toast));
    while (cards_.size() > kMaxCards) cards_.pop_front();
}

void ItemToasts::advance(float deltaSeconds) {
    if (deltaSeconds <= 0.f) return;
    for (ItemToast& card : cards_) card.age += deltaSeconds;
    cards_.erase(std::remove_if(cards_.begin(), cards_.end(), [](const ItemToast& c) { return finished(c); }),
                 cards_.end());
}

void ItemToasts::dismiss() {
    for (ItemToast& card : cards_) {
        if (card.dismissed) continue;
        card.dismissed = true;
        card.dismissAge = card.age;
    }
}

bool ItemToasts::dismissable() const {
    return std::any_of(cards_.begin(), cards_.end(), [](const ItemToast& c) { return !c.dismissed; });
}

float ItemToasts::alpha(const ItemToast& card) {
    float shown = timelineAlpha(card.age);
    if (card.dismissed) {
        // 从收起那一刻的浓度起，kDismissFade 秒淡完；还在排队的那几张（那一刻浓度 0）就此不出场。
        const float from = timelineAlpha(card.dismissAge);
        shown = std::min(shown, from * (1.f - (card.age - card.dismissAge) / kDismissFade));
    }
    return std::clamp(shown, 0.f, 1.f);
}

bool ItemToasts::finished(const ItemToast& card) {
    if (card.dismissed) return card.dismissAge < 0.f || card.age - card.dismissAge >= kDismissFade;
    return card.age >= kFadeIn + kHold + kFadeOut;
}

// ---------------------------------------------------------------------------
// 画
// ---------------------------------------------------------------------------

RectF itemToastRect(std::size_t index) {
    const float x = static_cast<float>(engine::kLogicalWidth) - kCardRight - kCardW;
    const float y = kCardTop + static_cast<float>(index) * (kCardH + kCardGap);
    return RectF{x, y, kCardW, kCardH};
}

void drawItemToasts(engine::Engine& engine, const ui::Theme& theme, hud::Geometry& geometry, engine::TextureId icons,
                    const ItemToasts& toasts, const engine::Rect& hero) {
    std::size_t slot = 0;
    for (const ItemToast& card : toasts.cards()) {
        const std::size_t index = slot++;
        float a = ItemToasts::alpha(card);
        if (a <= 0.f) continue;
        RectF rect = itemToastRect(index);
        if (card.age < ItemToasts::kFadeIn) rect.x += kSlide * (1.f - card.age / ItemToasts::kFadeIn);
        if (overlaps(rect, hero)) a *= kCoveringAlpha;
        a = stepped(a);
        if (a <= 0.f) continue;

        const Color tierColor = theme.gradeColors[std::clamp(card.tier, 0, 4)];
        hud::drawPanel(engine, theme, geometry, rect, a);
        // 左缘一道品阶色：远远瞥一眼就知道是好东西还是寻常货。
        engine.fillRect(RectF{rect.x + 5.f, rect.y + 7.f, 3.f, rect.h - 14.f}, hud::faded(tierColor, a),
                        BlendMode::Alpha);

        const RectF box{rect.x + 16.f, rect.y + (rect.h - kIconBox) / 2.f, kIconBox, kIconBox};
        engine.fillRect(box, hud::faded(theme.inkDeep, a * 0.85f), BlendMode::Alpha);
        ui::drawFrame(engine, box, hud::faded(tierColor, a));
        if (icons != engine::kInvalidTexture) {
            engine::DrawOptions tint;
            tint.tint = hud::faded(Color{255, 255, 255, 255}, a);
            engine.drawTexture(icons, iconRect(card.icon), RectF{box.x + 2.f, box.y + 2.f, 32.f, 32.f}, tint);
        }

        const engine::TextStyle style = fadedStyle(theme.bodyStyle, a);
        const int textX = static_cast<int>(rect.x + 64.f);
        const int nameY = static_cast<int>(rect.y + (card.note.empty() ? 17.f : 7.f));
        engine.drawText(card.name, textX, nameY, kNameFont, hud::faded(tierColor, a), style);
        const engine::Point nameSize = engine.measureText(card.name, kNameFont);
        engine.drawText("×" + std::to_string(card.count), textX + nameSize.x + 8, nameY + 2, kCountFont,
                        hud::faded(theme.paper, a), style);
        if (!card.note.empty()) {
            engine.drawText(card.note, textX, static_cast<int>(rect.y + 35.f), kNoteFont,
                            hud::faded(theme.paperDim, a), style);
        }
    }
}

}  // namespace fanren::game
