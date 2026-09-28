#include "game/WorldHud.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace fanren::game::hud {
namespace {

using engine::BlendMode;
using engine::Color;
using engine::RectF;
using engine::Vertex;

constexpr float kTwoPi = 2.f * std::numbers::pi_v<float>;

// 字号。名牌比对白小一圈（它常年挂在那儿）；地名要大，一眼就是「到了一个新地方」。
constexpr int kPlateFont = 16;
constexpr int kObjectiveFont = 18;
constexpr int kWhereFont = 16;
constexpr int kNoticeFont = 18;
constexpr int kBannerFont = 30;
// 危险度那一行：比地名小一半，塞进地名金线与目标框（kBannerBottom）之间那一截。
constexpr int kDangerFont = 15;
constexpr int kArrowLabelFont = 14;

// 横幅与目标框的位置。目标框排在横幅下面：横幅淡出之后目标框也不跟着往上跳——
// 一块会自己挪位置的框，玩家每次找它都得重新找一遍。
constexpr float kBannerX = 24.f;
constexpr float kBannerY = 16.f;
constexpr float kBannerBottom = 92.f;

Vertex vtx(float x, float y, const Color& c) {
    return Vertex{x, y, c, 0.f, 0.f};
}

// 1px 的矩形框，四条边各一次 fillRect。
void frameRect(engine::Engine& engine, const RectF& r, const Color& c) {
    engine.fillRect({r.x, r.y, r.w, 1.f}, c, BlendMode::Alpha);
    engine.fillRect({r.x, r.y + r.h - 1.f, r.w, 1.f}, c, BlendMode::Alpha);
    engine.fillRect({r.x, r.y + 1.f, 1.f, r.h - 2.f}, c, BlendMode::Alpha);
    engine.fillRect({r.x + r.w - 1.f, r.y + 1.f, 1.f, r.h - 2.f}, c, BlendMode::Alpha);
}

}  // namespace

Color faded(Color color, float alpha) {
    const float a = std::clamp(alpha, 0.f, 1.f) * static_cast<float>(color.a);
    color.a = static_cast<std::uint8_t>(std::lround(a));
    return color;
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

void Geometry::quad(float x0, float y0, float x1, float y1, const Color& top, const Color& bottom) {
    polygon4(vtx(x0, y0, top), vtx(x1, y0, top), vtx(x1, y1, bottom), vtx(x0, y1, bottom));
}

void Geometry::polygon4(const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d) {
    const int base = vertexCount();
    vertices_.insert(vertices_.end(), {a, b, c, d});
    indices_.insert(indices_.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

void Geometry::triangle(const Vertex& a, const Vertex& b, const Vertex& c) {
    const int base = vertexCount();
    vertices_.insert(vertices_.end(), {a, b, c});
    indices_.insert(indices_.end(), {base, base + 1, base + 2});
}

void Geometry::fan(float cx, float cy, float rx, float ry, const Color& center, const Color& rim,
                   int segments) {
    const int base = vertexCount();
    vertices_.push_back(vtx(cx, cy, center));
    for (int i = 0; i < segments; ++i) {
        const float a = kTwoPi * static_cast<float>(i) / static_cast<float>(segments);
        vertices_.push_back(vtx(cx + rx * std::cos(a), cy + ry * std::sin(a), rim));
    }
    for (int i = 0; i < segments; ++i) {
        indices_.insert(indices_.end(), {base, base + 1 + i, base + 1 + (i + 1) % segments});
    }
}

void Geometry::diamond(float cx, float cy, float rx, float ry, const Color& color) {
    polygon4(vtx(cx, cy - ry, color), vtx(cx + rx, cy, color), vtx(cx, cy + ry, color),
             vtx(cx - rx, cy, color));
}

void Geometry::draw(engine::Engine& engine, BlendMode blend, engine::TextureId texture) {
    if (!vertices_.empty()) engine.drawGeometry(texture, vertices_, indices_, blend);
    vertices_.clear();
    indices_.clear();
}

// ---------------------------------------------------------------------------
// 面板与文字
// ---------------------------------------------------------------------------

void drawPanel(engine::Engine& engine, const ui::Theme& theme, Geometry& geometry, const RectF& rect,
               float alpha) {
    engine.drawGradient(rect, faded(theme.ink, alpha), faded(theme.inkLow, alpha), BlendMode::Alpha);
    frameRect(engine, rect, faded(theme.gold, alpha));
    frameRect(engine, {rect.x + 3.f, rect.y + 3.f, rect.w - 6.f, rect.h - 6.f}, faded(theme.goldDim, alpha));
    for (const auto& [x, y] : {std::pair{rect.x, rect.y}, std::pair{rect.x + rect.w, rect.y},
                               std::pair{rect.x + rect.w, rect.y + rect.h},
                               std::pair{rect.x, rect.y + rect.h}}) {
        geometry.diamond(x, y, 4.f, 4.f, faded(theme.gold, alpha));
    }
    geometry.draw(engine, BlendMode::Alpha);
}

RectF namePlateRect(engine::Engine& engine, const std::string& label, float centerX, float bottomY) {
    if (label.empty()) return RectF{};
    const engine::Point size = engine.measureText(label, kPlateFont);
    if (size.x <= 0) return RectF{};
    const float w = static_cast<float>(size.x);
    const float h = static_cast<float>(size.y);
    return RectF{centerX - w / 2.f, bottomY - h, w, h};
}

void drawNamePlate(engine::Engine& engine, const ui::Theme& theme, const std::string& label,
                   float centerX, float bottomY) {
    if (label.empty()) return;
    const engine::Point size = engine.measureText(label, kPlateFont);
    if (size.x <= 0) return;   // 无头量不出字，画了也是空操作
    // 标题字样：投影 + 一圈描边。压在草地、白墙、夜色上都读得出，不要底条。
    engine.drawText(label, static_cast<int>(std::lround(centerX - static_cast<float>(size.x) / 2.f)),
                    static_cast<int>(std::lround(bottomY - static_cast<float>(size.y))), kPlateFont,
                    theme.paper, theme.titleStyle);
}

namespace {

constexpr float kBoxPadX = 14.f;
constexpr float kBoxPadY = 9.f;
constexpr float kBoxBullet = 18.f;   // 行首那枚金菱形占的宽

}  // namespace

RectF objectiveBoxRect(engine::Engine& engine, float x, float y, const std::string& text,
                       const std::string& where) {
    if (text.empty()) return RectF{};
    const engine::Point body = engine.measureText(text, kObjectiveFont);
    if (body.x <= 0) return RectF{};
    const engine::Point place = where.empty() ? engine::Point{} : engine.measureText(where, kWhereFont);
    const float width = static_cast<float>(std::max(body.x, place.x)) + kBoxPadX * 2.f + kBoxBullet;
    const float height = static_cast<float>(body.y) +
                         (where.empty() ? 0.f : static_cast<float>(place.y) + 4.f) + kBoxPadY * 2.f;
    return RectF{x, y, width, height};
}

void drawObjectiveBox(engine::Engine& engine, const ui::Theme& theme, Geometry& geometry, float x, float y,
                      const std::string& text, const std::string& where, float alpha) {
    const RectF box = objectiveBoxRect(engine, x, y, text, where);
    if (box.w <= 0.f) return;
    const engine::Point body = engine.measureText(text, kObjectiveFont);
    drawPanel(engine, theme, geometry, box, alpha);

    geometry.diamond(x + kBoxPadX + 5.f, y + kBoxPadY + static_cast<float>(body.y) / 2.f, 4.f, 5.f,
                     faded(theme.goldBright, alpha));
    geometry.draw(engine, BlendMode::Alpha);

    // 淡的时候字色也跟着淡：只有「盖住主角」与「没盖住」两档，文字缓存里多一份而已。
    const int textX = static_cast<int>(x + kBoxPadX + kBoxBullet);
    engine.drawText(text, textX, static_cast<int>(y + kBoxPadY), kObjectiveFont, faded(theme.paper, alpha),
                    theme.bodyStyle);
    if (!where.empty()) {
        engine.drawText(where, textX, static_cast<int>(y + kBoxPadY) + body.y + 4, kWhereFont,
                        faded(theme.goldBright, alpha), theme.bodyStyle);
    }
}

void drawSaveNotice(engine::Engine& engine, const ui::Theme& theme, Geometry& geometry,
                    const std::string& text) {
    if (text.empty()) return;
    const engine::Point size = engine.measureText(text, kNoticeFont);
    if (size.x <= 0) return;
    const float width = static_cast<float>(size.x) + 32.f;
    const float height = static_cast<float>(size.y) + 18.f;
    const float x = static_cast<float>(engine::kLogicalWidth) - width - 16.f;
    drawPanel(engine, theme, geometry, {x, 16.f, width, height}, 1.f);
    engine.drawText(text, static_cast<int>(x + 16.f), 25, kNoticeFont, theme.paper, theme.bodyStyle);
}

// ---------------------------------------------------------------------------
// 目标标记
// ---------------------------------------------------------------------------

void drawObjectiveDiamond(engine::Engine& engine, const ui::Theme& theme, Geometry& geometry,
                          engine::TextureId glow, float cx, float cy) {
    engine::DrawOptions light;
    light.tint = faded(theme.gold, 0.7f);
    light.blend = BlendMode::Add;
    engine.drawTexture(glow, RectF{}, RectF{cx - 30.f, cy - 30.f, 60.f, 60.f}, light);

    // 深色描边 → 金 → 上半一抹亮金：一颗有体积的菱形，任何底色上都压得住。
    geometry.diamond(cx, cy, 11.f, 15.f, theme.inkDeep);
    geometry.diamond(cx, cy, 8.f, 12.f, theme.gold);
    geometry.polygon4(vtx(cx, cy - 12.f, theme.goldBright), vtx(cx + 8.f, cy, theme.goldBright),
                      vtx(cx, cy - 3.f, theme.goldBright), vtx(cx - 8.f, cy, theme.goldBright));
    geometry.draw(engine, BlendMode::Alpha);
}

void drawEdgeArrow(engine::Engine& engine, const ui::Theme& theme, Geometry& geometry,
                   engine::TextureId glow, float x, float y, float dx, float dy, const std::string& label) {
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length <= 0.f) return;
    const float ux = dx / length;
    const float uy = dy / length;
    const float px = -uy;   // 垂直方向
    const float py = ux;

    engine::DrawOptions light;
    light.tint = faded(theme.gold, 0.75f);
    light.blend = BlendMode::Add;
    const float gx = x - ux * 12.f;
    const float gy = y - uy * 12.f;
    engine.drawTexture(glow, RectF{}, RectF{gx - 36.f, gy - 36.f, 72.f, 72.f}, light);

    const auto point = [&](float along, float across, const Color& color) {
        return vtx(x + ux * along + px * across, y + uy * along + py * across, color);
    };
    geometry.triangle(point(3.f, 0.f, theme.inkDeep), point(-27.f, 16.f, theme.inkDeep),
                      point(-27.f, -16.f, theme.inkDeep));
    geometry.triangle(point(0.f, 0.f, theme.gold), point(-24.f, 13.f, theme.gold),
                      point(-24.f, -13.f, theme.gold));
    geometry.triangle(point(-4.f, 0.f, theme.goldBright), point(-18.f, 7.f, theme.goldBright),
                      point(-18.f, -7.f, theme.goldBright));
    geometry.draw(engine, BlendMode::Alpha);

    if (label.empty()) return;
    const engine::Point size = engine.measureText(label, kArrowLabelFont);
    if (size.x <= 0) return;
    const float lx = x - ux * 46.f - static_cast<float>(size.x) / 2.f;
    const float ly = y - uy * 46.f - static_cast<float>(size.y) / 2.f;
    engine.drawText(label, static_cast<int>(std::lround(lx)), static_cast<int>(std::lround(ly)),
                    kArrowLabelFont, theme.goldBright, theme.titleStyle);
}

// ---------------------------------------------------------------------------
// 地名横幅
// ---------------------------------------------------------------------------

namespace {

// 危险度那一行打头的界面词（setDangerLabel）。整个进程一份：横幅只有世界层那一个。
std::string& dangerLabel() {
    static std::string label;
    return label;
}

}  // namespace

void PlaceBanner::setDangerLabel(std::string label) {
    dangerLabel() = std::move(label);
}

std::string PlaceBanner::dangerLine(int stars) {
    std::string line = dangerLabel() + "　";
    for (int i = 0; i < kMaxStars; ++i) line += i < stars ? "★" : "☆";
    return line;
}

void PlaceBanner::set(std::string title, int dangerStars) {
    if (title == title_ && dangerStars == dangerStars_) return;
    title_ = std::move(title);
    dangerStars_ = dangerStars;
    dirty_ = true;
}

float PlaceBanner::bottom() {
    return kBannerBottom;
}

RectF PlaceBanner::area() const {
    if (textW_ <= 0.f) return RectF{};
    return RectF{kBannerX - 10.f, kBannerY, textW_ + 200.f, textH_ + 18.f + dangerH_};
}

void PlaceBanner::build(engine::Engine& engine, const ui::Theme& theme) {
    text_.reset();
    textW_ = 0.f;
    textH_ = 0.f;
    const engine::Point size = engine.measureText(title_, kBannerFont);
    if (size.x <= 0) return;
    // 四周各留出描边 2px 与投影 2px 的余地。
    const int w = size.x + 8;
    const int h = size.y + 8;
    const engine::TextureId previous = engine.renderTarget();
    text_ = engine::OwnedTexture(engine, engine.createRenderTarget(w, h, engine::ScaleMode::Linear));
    if (!text_.valid()) return;
    engine.setRenderTarget(text_.get());
    engine.clear(Color{0, 0, 0, 0});
    engine.drawText(title_, 3, 2, kBannerFont, theme.paper, theme.titleStyle);
    engine.setRenderTarget(previous);
    textW_ = static_cast<float>(w);
    textH_ = static_cast<float>(h);

    danger_.reset();
    dangerW_ = 0.f;
    dangerH_ = 0.f;
    if (dangerStars_ <= 0) return;
    const std::string line = dangerLine(dangerStars_);
    const engine::Point lineSize = engine.measureText(line, kDangerFont);
    danger_ = engine::OwnedTexture(
        engine, engine.createRenderTarget(lineSize.x + 8, lineSize.y + 8, engine::ScaleMode::Linear));
    if (!danger_.valid()) return;
    engine.setRenderTarget(danger_.get());
    engine.clear(Color{0, 0, 0, 0});
    engine.drawText(line, 3, 2, kDangerFont, theme.goldBright, theme.bodyStyle);
    engine.setRenderTarget(previous);
    dangerW_ = static_cast<float>(lineSize.x + 8);
    dangerH_ = static_cast<float>(lineSize.y + 8);
}

void PlaceBanner::draw(engine::Engine& engine, const ui::Theme& theme, Geometry& geometry, float alpha) {
    if (alpha <= 0.f || title_.empty()) return;
    if (dirty_) {
        build(engine, theme);
        dirty_ = false;
    }
    if (!text_.valid()) return;

    // 底：左浓右淡的一条墨色，托住字又不像一块面板那样把角落堵死。
    const float bandW = textW_ + 200.f;
    const float bandH = textH_ + 18.f + dangerH_;
    Color bandLeft = theme.inkDeep;
    bandLeft.a = 200;
    Color bandRight = theme.inkDeep;
    bandRight.a = 0;
    engine.drawGradientH({kBannerX - 10.f, kBannerY, bandW, bandH}, faded(bandLeft, alpha),
                         faded(bandRight, alpha), BlendMode::Alpha);

    engine::DrawOptions tint;
    tint.tint = faded(Color{255, 255, 255, 255}, alpha);
    engine.drawTexture(text_.get(), RectF{}, RectF{kBannerX + 18.f, kBannerY + 3.f, textW_, textH_}, tint);

    // 字下一道金线：左端一枚菱形，往右渐隐。
    const float lineY = kBannerY + textH_ + 6.f;
    Color goldOut = theme.gold;
    goldOut.a = 0;
    Color dimOut = theme.goldDim;
    dimOut.a = 0;
    engine.drawGradientH({kBannerX + 18.f, lineY, textW_ + 120.f, 1.f}, faded(theme.gold, alpha),
                         faded(goldOut, alpha), BlendMode::Alpha);
    engine.drawGradientH({kBannerX + 18.f, lineY + 3.f, textW_ + 60.f, 1.f}, faded(theme.goldDim, alpha),
                         faded(dimOut, alpha), BlendMode::Alpha);
    geometry.diamond(kBannerX + 8.f, lineY + 1.f, 5.f, 5.f, faded(theme.gold, alpha));
    geometry.diamond(kBannerX + 8.f, lineY + 1.f, 2.f, 2.f, faded(theme.goldBright, alpha));
    geometry.draw(engine, BlendMode::Alpha);
    // 危险度（有遭遇的图）：金线下面一行「凶险　★★☆☆☆」，与地名同淡同现。
    if (danger_.valid()) {
        engine.drawTexture(danger_.get(), RectF{}, RectF{kBannerX + 18.f, lineY + 4.f, dangerW_, dangerH_}, tint);
    }
}

}  // namespace fanren::game::hud
