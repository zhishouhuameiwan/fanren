#include "game/BattleHud.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "core/model/AttackCategory.h"
#include "game/BattleView.h"
#include "game/Wording.h"

namespace fanren::game {

using core::battle::BattlePhase;
using core::battle::Unit;
using engine::BlendMode;
using engine::Color;
using engine::DrawOptions;
using engine::RectF;

namespace {

constexpr float kIconScale = 2.f;
constexpr float kIcon = 16.f * kIconScale;

[[nodiscard]] Color fade(Color c, float alpha) {
    const float q = std::round(std::clamp(alpha, 0.f, 1.f) * 16.f) / 16.f;
    c.a = static_cast<std::uint8_t>(std::lround(static_cast<float>(c.a) * q));
    return c;
}

void drawIcon(engine::Engine& e, engine::TextureId icons, BattleIcon icon, float x, float y, float size, Color tint,
              BlendMode blend = BlendMode::Alpha) {
    DrawOptions opt{};
    opt.tint = tint;
    opt.blend = blend;
    e.drawTexture(icons, iconRect(icon), RectF{x, y, size, size}, opt);
}

// 右对齐 / 居中的一行字（带主题的投影）。
void drawTextRight(engine::Engine& e, const std::string& text, float right, float y, int size, Color color,
                   const engine::TextStyle& style) {
    const engine::Point m = e.measureText(text, size);
    e.drawText(text, static_cast<int>(right) - m.x, static_cast<int>(y), size, color, style);
}

void drawTextCenter(engine::Engine& e, const std::string& text, float cx, float y, int size, Color color,
                    const engine::TextStyle& style) {
    const engine::Point m = e.measureText(text, size);
    e.drawText(text, static_cast<int>(cx) - m.x / 2, static_cast<int>(y), size, color, style);
}

[[nodiscard]] engine::Rect toRect(const RectF& r) {
    return engine::Rect{static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.w), static_cast<int>(r.h)};
}

[[nodiscard]] float easeOut(float t) {
    const float u = 1.f - std::clamp(t, 0.f, 1.f);
    return 1.f - u * u * u;
}

// 战果卡上第 k 行开始飞的时刻。
[[nodiscard]] float lineStart(std::size_t k) { return 0.35f + 0.14f * static_cast<float>(k); }
constexpr float kLineFlight = 0.28f;

}  // namespace

// ---------------------------------------------------------------------------
// 图标
// ---------------------------------------------------------------------------

RectF iconRect(BattleIcon icon) {
    // 图集 128×48：第一行 剑 刀 拳 暗器 毒 金 木 水；第二行 火 土 ？ 架势 劲满 劲空 指针 修为；
    // 第三行 碎银 宝箱 药。
    static constexpr std::array<std::array<int, 2>, 19> kCells{{
        {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0},
        {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1},
        {0, 2}, {1, 2}, {2, 2},
    }};
    const auto& cell = kCells[static_cast<std::size_t>(icon)];
    return RectF{static_cast<float>(cell[0] * 16), static_cast<float>(cell[1] * 16), 16.f, 16.f};
}

BattleIcon categoryIcon(int category) {
    using namespace core;
    switch (category) {
        case kCategorySword: return BattleIcon::Sword;
        case kCategoryBlade: return BattleIcon::Blade;
        case kCategoryFist: return BattleIcon::Fist;
        case kCategoryHidden: return BattleIcon::Hidden;
        case kCategoryPoison: return BattleIcon::Poison;
        case kCategoryMetal: return BattleIcon::Metal;
        case kCategoryWood: return BattleIcon::Wood;
        case kCategoryWater: return BattleIcon::Water;
        case kCategoryFire: return BattleIcon::Fire;
        case kCategoryEarth: return BattleIcon::Earth;
        default: return BattleIcon::Unknown;
    }
}

std::vector<RewardLine> rewardLines(const GrantedReward& granted, const core::GameData& data,
                                    const core::GameState& state, const std::string& cultivationWord) {
    std::vector<RewardLine> lines;
    if (granted.cultivation > 0) {
        lines.push_back({BattleIcon::Cultivation, cultivationWord + " +" + std::to_string(granted.cultivation)});
    }
    // 钱的名字按阶段取（Wording）：第 1–5 章是「碎银 N 块」，不许冒出「灵石」。
    if (granted.money > 0) lines.push_back({BattleIcon::Silver, currencyAmount(state, granted.money)});
    for (const core::BagEntry& drop : granted.drops) {
        const core::Item* item = data.findItem(drop.itemId);
        const std::string name = item != nullptr ? item->name : drop.itemId;
        const bool medicine = item != nullptr && (item->restoreHp > 0 || item->restoreMp > 0);
        lines.push_back({medicine ? BattleIcon::Medicine : BattleIcon::Chest,
                         name + " ×" + std::to_string(drop.count)});
    }
    return lines;
}

// ---------------------------------------------------------------------------
// 顶端：行动序条、回合与波次
// ---------------------------------------------------------------------------

void drawOrderStrip(engine::Engine& e, const HudFrame& f, const std::string& nowWord, const std::string& nextWord) {
    const std::vector<Unit>& units = f.battle.units();
    std::vector<int> now;
    const std::vector<int>& order = f.battle.roundOrder();
    for (std::size_t i = f.battle.orderPosition(); i < order.size(); ++i) {
        const Unit& u = units[static_cast<std::size_t>(order[i])];
        if (u.alive() && !u.broken()) now.push_back(order[i]);
    }
    const OrderStrip strip = layoutOrderStrip(now, f.battle.nextRoundOrder(), 16.f, 1000.f);
    const float end = strip.chips.empty() ? 200.f : strip.chips.back().x + strip.chips.back().size + 40.f;
    // 底：一条向右淡掉的墨带，压得住任何背景又不像一块板子。
    e.drawGradientH(RectF{0.f, 0.f, end, 74.f}, fade(f.theme.inkDeep, 0.78f), fade(f.theme.inkDeep, 0.f),
                    BlendMode::Alpha);
    const engine::TextStyle& style = f.theme.bodyStyle;
    // 本回合的人都动完了（只剩下回合的预览）时不写「本回合」，免得两个字压在一起。
    if (!now.empty()) e.drawText(nowWord, 18, 0, 13, f.theme.goldDim, style);
    for (const OrderChip& chip : strip.chips) {
        const RectF box{chip.x, chip.y, chip.size, chip.size};
        const Unit& u = units[static_cast<std::size_t>(chip.unit)];
        e.fillRect(box, fade(f.theme.inkDeep, 0.9f), BlendMode::Alpha);
        f.view.drawPortrait(e, chip.unit, RectF{box.x + 2.f, box.y + 2.f, box.w - 4.f, box.h - 4.f},
                            Color{255, 255, 255, static_cast<std::uint8_t>(chip.nextRound ? 170 : 255)});
        e.fillRect(RectF{box.x + 2.f, box.y + box.h - 5.f, box.w - 4.f, 3.f}, u.ally ? f.theme.azure : f.theme.cinnabar,
                   BlendMode::Alpha);
        if (chip.current) {
            ui::drawFrame(e, box, f.theme.goldBright);
            ui::drawFrame(e, RectF{box.x - 2.f, box.y - 2.f, box.w + 4.f, box.h + 4.f}, f.theme.gold);
        } else {
            ui::drawFrame(e, box, chip.nextRound ? f.theme.goldDim : f.theme.gold);
        }
    }
    if (strip.dividerX >= 0.f) {
        ui::drawRuleV(e, strip.dividerX, 18.f, 46.f, f.theme);
        ui::drawDiamond(e, strip.dividerX, 41.f, 4.f, f.theme.gold);
        e.drawText(nextWord, static_cast<int>(strip.dividerX) + 12, 0, 13, f.theme.goldDim, style);
    }
}

void drawRoundInfo(engine::Engine& e, const HudFrame& f, const std::string& round, const std::string& wave) {
    drawTextRight(e, round, 1262.f, 12.f, 20, f.theme.paper, f.theme.bodyStyle);
    if (!wave.empty()) drawTextRight(e, wave, 1262.f, 40.f, 16, f.theme.goldBright, f.theme.bodyStyle);
}

// ---------------------------------------------------------------------------
// 敌人脚下：架势盾、破绽格、细血条
// ---------------------------------------------------------------------------

void drawEnemyPlates(engine::Engine& e, const HudFrame& f, const std::string& chargeWord) {
    const std::vector<Unit>& units = f.battle.units();
    const bool devour = f.battle.devourMode();
    for (std::size_t i = 0; i < units.size() && i < f.shown.size(); ++i) {
        const Unit& u = units[i];
        const ShownUnit& s = f.shown[i];
        if (u.ally || !s.visible) continue;
        const StagePoint feet = f.view.anchors()[i];
        const HudPulse& pulse = f.view.pulse(static_cast<int>(i));
        const bool targeted = static_cast<int>(i) == f.target;
        if (devour || u.maxToughness <= 0) {
            // 识海：没有架势与破绽，只剩一条「体积」。
            ui::drawGauge(e, engine::Rect{static_cast<int>(feet.x) - 50, static_cast<int>(feet.y) + 14, 100, 7}, s.hp,
                          u.maxHp, f.theme.cinnabar, f.theme);
            continue;
        }
        const std::vector<int> bits = core::categoryBits(u.weaknesses);
        const float rowW = kIcon + 8.f + static_cast<float>(bits.size()) * (kIcon + 2.f) - 2.f;
        const float x0 = feet.x - rowW / 2.f;
        const float y0 = feet.y + 10.f;
        // 底：一块墨，选中的那一个描金。
        const RectF plate{x0 - 6.f, y0 - 4.f, rowW + 12.f, kIcon + 20.f};
        // 墨底压得很淡：后排那一行难免擦到前排的肩膀，人得透得出来。
        e.fillRect(plate, fade(f.theme.inkDeep, 0.42f), BlendMode::Alpha);
        if (targeted) ui::drawFrame(e, plate, f.theme.goldBright);

        // 架势盾：被削时跳一下，回满时亮一圈；破势时整块朱红。
        const float hit = pulse.shieldHit;
        const float jump = hit > 0.f ? std::sin(hit * 40.f) * 3.f : 0.f;
        const float grow = 1.f + 0.3f * std::min(1.f, hit / 0.3f);
        const float size = kIcon * grow;
        const float sx = x0 + (kIcon - size) / 2.f + jump;
        const float sy = y0 + (kIcon - size) / 2.f;
        drawIcon(e, f.icons, BattleIcon::Stance, sx, sy, size, s.broken ? f.theme.cinnabar : Color{255, 255, 255, 255});
        if (pulse.shieldRefill > 0.f) {
            drawIcon(e, f.icons, BattleIcon::Stance, sx, sy, size, fade(f.theme.goldBright, pulse.shieldRefill / 0.6f),
                     BlendMode::Add);
        }
        if (hit > 0.f) drawIcon(e, f.icons, BattleIcon::Stance, sx, sy, size, fade(Color{255, 255, 255, 255}, hit / 0.5f),
                                BlendMode::Add);
        engine::TextStyle numberStyle{};
        numberStyle.outline = true;
        numberStyle.outlineColor = f.theme.inkDeep;
        numberStyle.outlineWidth = 2;
        if (!s.broken) {
            // 递减：被削的那一下，旧数字往下掉着淡掉，新数字从上面落进来。破势时盾上不写数（盾碎了）。
            const float k = std::clamp(hit / 0.3f, 0.f, 1.f);
            const float cx = x0 + kIcon / 2.f + jump;
            if (k > 0.f && pulse.shieldFrom > s.toughness) {
                drawTextCenter(e, std::to_string(pulse.shieldFrom), cx, y0 + 5.f + (1.f - k) * 16.f, 18,
                               fade(f.theme.cinnabar, k), numberStyle);
            }
            drawTextCenter(e, std::to_string(s.toughness), cx, y0 + 5.f - k * 10.f, 18,
                           fade(f.theme.paper, 1.f - 0.6f * k), numberStyle);
        }

        // 破绽格：已揭开的画类别，没揭开的画「？」。这一击揭开 / 打中的那几格亮一下。
        float x = x0 + kIcon + 8.f;
        for (const int bit : bits) {
            const bool known = (s.revealed & bit) != 0;
            drawIcon(e, f.icons, known ? categoryIcon(bit) : BattleIcon::Unknown, x, y0, kIcon,
                     known ? Color{255, 255, 255, 255} : Color{200, 196, 186, 230});
            if (pulse.reveal > 0.f && (pulse.revealBits & bit) != 0) {
                drawIcon(e, f.icons, known ? categoryIcon(bit) : BattleIcon::Unknown, x, y0, kIcon,
                         fade(f.theme.goldBright, pulse.reveal / 0.55f), BlendMode::Add);
            }
            x += kIcon + 2.f;
        }
        ui::drawGauge(e, toRect(RectF{x0, y0 + kIcon + 5.f, rowW, 6.f}), s.hp, u.maxHp, f.theme.cinnabar, f.theme);
        float below = y0 + kIcon + 14.f;
        if (s.broken) {
            drawTextCenter(e, breakStatusText(f.battle, u), feet.x, below, 14, f.theme.breakRed,
                           f.theme.bodyStyle);
            below += 16.f;
        }
        if (const std::string poison = poisonStatusText(u); !poison.empty()) {
            drawTextCenter(e, poison, feet.x, below, 13, f.theme.poison, f.theme.bodyStyle);
        }
        if (u.charging && !s.broken) {
            engine::TextStyle tag{};
            tag.outline = true;
            tag.outlineColor = f.theme.inkDeep;
            e.drawText(chargeWord, static_cast<int>(x0 + rowW + 12.f), static_cast<int>(y0 + 6.f), 16,
                       f.theme.breakRed, tag);
        }
    }
}

void drawTargetPointer(engine::Engine& e, const HudFrame& f) {
    const std::vector<Unit>& units = f.battle.units();
    if (f.target < 0 || static_cast<std::size_t>(f.target) >= units.size()) return;
    const auto i = static_cast<std::size_t>(f.target);
    const StagePoint feet = f.view.anchors()[i];
    const float bob = std::sin(f.view.clock() * 6.f) * 4.f;
    const float y = f.view.headY(f.target) - 40.f + bob;
    drawIcon(e, f.icons, BattleIcon::Pointer, feet.x - kIcon / 2.f, y, kIcon, Color{255, 255, 255, 255});
    // 名牌：小牌子压在指针上方。
    const engine::Point m = e.measureText(units[i].name, 16);
    const auto w = static_cast<float>(m.x) + 32.f;
    const RectF plate{std::clamp(feet.x - w / 2.f, 8.f, 1272.f - w), y - 32.f, w, 28.f};
    ui::drawPlate(e, toRect(plate), f.theme);
    drawTextCenter(e, units[i].name, plate.x + plate.w / 2.f, plate.y + 4.f, 16, f.theme.goldBright, f.theme.bodyStyle);
}

// ---------------------------------------------------------------------------
// 右下：我方
// ---------------------------------------------------------------------------

void drawPartyPanel(engine::Engine& e, const HudFrame& f) {
    const RectF panel = kPartyPanelRect;
    ui::drawPanel(e, toRect(panel), "", f.theme);
    const std::vector<Unit>& units = f.battle.units();
    constexpr float kRow = 34.f;
    float y = panel.y + 10.f;
    for (std::size_t i = 0; i < units.size() && i < f.shown.size(); ++i) {
        const Unit& u = units[i];
        if (!u.ally) continue;
        if (y + kRow > panel.y + panel.h) break;
        const ShownUnit& s = f.shown[i];
        const bool current = static_cast<int>(i) == f.actor;
        const float x = panel.x + 18.f;
        if (current) ui::drawDiamond(e, x - 6.f, y + 10.f, 5.f, f.theme.goldBright);
        e.drawText(u.name, static_cast<int>(x) + 4, static_cast<int>(y), 17,
                   current ? f.theme.goldBright : s.hp <= 0 ? f.theme.paperDim : f.theme.paper, f.theme.bodyStyle);
        std::string status = poisonStatusText(u);
        if (u.guarding) status += status.empty() ? "守势" : " · 守势";
        if (!status.empty()) {
            e.drawText(status, static_cast<int>(x) + 4, static_cast<int>(y) + 19, 11, f.theme.poisonDim,
                       f.theme.bodyStyle);
        }
        // 气血条在上、法力条贴在它下面，数字在条的右边：劲珠要让出一截 ×2 的宽度。
        ui::drawGauge(e, toRect(RectF{panel.x + 112.f, y + 4.f, 112.f, 8.f}), s.hp, u.maxHp, f.theme.jade, f.theme);
        if (u.maxMp > 0) {
            ui::drawGauge(e, toRect(RectF{panel.x + 112.f, y + 15.f, 112.f, 5.f}), u.mp, u.maxMp, f.theme.azure, f.theme);
        }
        e.drawText(std::to_string(std::max(0, s.hp)) + "/" + std::to_string(u.maxHp), static_cast<int>(panel.x) + 230,
                   static_cast<int>(y) + 2, 13, f.theme.paperDim, f.theme.bodyStyle);
        if (!f.battle.devourMode()) {
            // 劲：五颗（图标 ×2，珠子本身 10 像素见方居中，步长 22 正好留一道缝）。当前这位正要蓄的那几颗
            // （最后面的 boost 颗）一明一暗、描朱红，与台上的气焰同步。
            const bool spending = current && f.choosing && f.boost > 0;
            const float glow = 0.55f + 0.45f * std::sin(f.view.clock() * 9.f);
            for (int pip = 0; pip < core::battle::kBpMax; ++pip) {
                const float bx = panel.x + 286.f + static_cast<float>(pip) * 22.f;
                const float by = y - 6.f;
                drawIcon(e, f.icons, pip < u.bp ? BattleIcon::BeadFull : BattleIcon::BeadEmpty, bx, by, kIcon,
                         Color{255, 255, 255, 255});
                if (spending && pip < u.bp && pip >= u.bp - f.boost) {
                    drawIcon(e, f.icons, BattleIcon::BeadFull, bx, by, kIcon, fade(Color{255, 170, 110, 255}, glow),
                             BlendMode::Add);
                    ui::drawFrame(e, RectF{bx + 5.f, by + 5.f, 22.f, 22.f}, fade(f.theme.cinnabar, glow));
                }
            }
        }
        y += kRow;
    }
}

// ---------------------------------------------------------------------------
// 横幅、日志
// ---------------------------------------------------------------------------

void drawChargeBanner(engine::Engine& e, const ui::Theme& theme, const std::string& text, float age, float life) {
    if (text.empty() || age >= life) return;
    const float alpha = std::min(std::clamp(age / 0.15f, 0.f, 1.f), std::clamp((life - age) / 0.3f, 0.f, 1.f));
    const float slide = (1.f - easeOut(age / 0.2f)) * 40.f;
    const RectF band{240.f - slide, 84.f, 800.f, 44.f};
    e.drawGradient(band, fade(theme.chargeBandTop, alpha), fade(theme.chargeBandBottom, alpha), BlendMode::Alpha);
    ui::drawFrame(e, band, fade(theme.cinnabar, alpha));
    ui::drawDiamond(e, band.x, band.y + band.h / 2.f, 6.f, fade(theme.cinnabar, alpha));
    ui::drawDiamond(e, band.x + band.w, band.y + band.h / 2.f, 6.f, fade(theme.cinnabar, alpha));
    drawTextCenter(e, text, band.x + band.w / 2.f, band.y + 11.f, 18, fade(theme.goldBright, alpha), theme.bodyStyle);
}

void drawBattleLog(engine::Engine& e, const ui::Theme& theme, const std::string& line, const std::string& hint) {
    const RectF box = kLogRect;
    e.drawGradientH(box, fade(theme.inkDeep, 0.8f), fade(theme.inkDeep, 0.1f), BlendMode::Alpha);
    ui::drawRuleH(e, box.x, box.y, box.w, theme);
    if (!line.empty()) e.drawText(line, static_cast<int>(box.x) + 14, static_cast<int>(box.y) + 7, 17, theme.paper, theme.bodyStyle);
    e.drawText(hint, static_cast<int>(box.x) + 14, static_cast<int>(box.y) + 33, 13, theme.paperDim, theme.bodyStyle);
}

// ---------------------------------------------------------------------------
// 战果卡
// ---------------------------------------------------------------------------

float resultCardSettleTime(const ResultCard& card) {
    if (card.phase != BattlePhase::Won || card.lines.empty()) return 0.5f;
    return lineStart(card.lines.size() - 1) + kLineFlight;
}

void drawResultCard(engine::Engine& e, const ui::Theme& theme, engine::TextureId icons, const ResultCard& card) {
    ui::drawScrim(e, theme);
    const float settle = resultCardSettleTime(card);
    const float hintAlpha = card.age < settle ? 0.f : 0.6f + 0.4f * std::sin((card.age - settle) * 4.f);
    if (card.phase != BattlePhase::Won) {
        // 败北、逃走、对手遁走：一条横带、一句话。
        const float alpha = std::clamp(card.age / 0.25f, 0.f, 1.f);
        const RectF band{290.f, 290.f, 700.f, 128.f};
        ui::drawPanel(e, toRect(band), "", theme);
        const Color title = card.phase == BattlePhase::Lost ? theme.cinnabar : theme.goldBright;
        ui::drawSpacedText(e, card.title, band.x + band.w / 2.f, static_cast<int>(band.y) + 18, 40, fade(title, alpha),
                           theme.titleStyle, 10);
        if (!card.subtitle.empty()) {
            drawTextCenter(e, card.subtitle, band.x + band.w / 2.f, band.y + 72.f, 16, fade(theme.paperDim, alpha),
                           theme.bodyStyle);
        }
        drawTextCenter(e, card.hint, band.x + band.w / 2.f, band.y + 98.f, 14, fade(theme.paperDim, hintAlpha),
                       theme.bodyStyle);
        return;
    }
    const RectF panel{400.f, 138.f, 480.f, 128.f + 40.f * static_cast<float>(std::max<std::size_t>(1, card.lines.size())) + 56.f};
    ui::drawPanel(e, toRect(panel), "", theme);
    const float cx = panel.x + panel.w / 2.f;
    // 「胜」：弹进来，再稳住。
    const float pop = 1.f + 0.5f * (1.f - easeOut(card.age / 0.22f));
    const int size = static_cast<int>(std::lround(88.f * pop / 2.f)) * 2;
    ui::drawSpacedText(e, card.title, cx, static_cast<int>(panel.y + 60.f) - size / 2, size,
                       fade(theme.goldBright, std::clamp(card.age / 0.12f, 0.f, 1.f)), theme.titleStyle, 0);
    drawTextCenter(e, card.subtitle, cx, panel.y + 108.f, 16, theme.paperDim, theme.bodyStyle);
    ui::drawOrnament(e, cx, panel.y + 136.f, 170.f, theme);
    // 收获逐行从右边飞进来。
    float y = panel.y + 152.f;
    for (std::size_t k = 0; k < card.lines.size(); ++k) {
        const float t = (card.age - lineStart(k)) / kLineFlight;
        if (t > 0.f) {
            const float k01 = easeOut(t);
            const float dx = (1.f - k01) * 200.f;
            const float alpha = std::clamp(t * 1.6f, 0.f, 1.f);
            drawIcon(e, icons, card.lines[k].icon, panel.x + 118.f + dx, y, kIcon, fade(Color{255, 255, 255, 255}, alpha));
            e.drawText(card.lines[k].text, static_cast<int>(panel.x + 162.f + dx), static_cast<int>(y) + 4, 22,
                       fade(theme.paper, alpha), theme.bodyStyle);
        }
        y += 40.f;
    }
    drawTextCenter(e, card.hint, cx, panel.y + panel.h - 34.f, 14, fade(theme.paperDim, hintAlpha), theme.bodyStyle);
}

}  // namespace fanren::game
