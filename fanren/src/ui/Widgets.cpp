#include "ui/Widgets.h"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "engine/TextLayout.h"

namespace fanren::ui {
namespace {

// 角花菱形的半径。4 像素在 1280×720 上刚好读得出是个「结」，再大就抢了标题的戏。
constexpr float kCornerDiamond = 4.f;

// 内缩那道暗金线离外框几像素（施工图 1.6：内缩 3px）。
constexpr float kInnerFrameInset = 3.f;

// 选中行指针的半径。比角花大一圈：它是这一屏上唯一会动的那颗。
constexpr float kPointerDiamond = 5.f;

// 标题下那道金线从左端的多少透明度起渐隐到零。满不透明会在长面板里像一条分割线，
// 把标题和正文切成两块；渐隐的线读起来是「标题的下划」。
constexpr std::uint8_t kTitleRuleAlpha = 200;

// 数值条上下两端各往 paper / inkDeep 拉多少：上端提亮三成、下端压暗四分之一，
// 竖向一过就读得出「是一根圆柱」而不是一块平色。
constexpr float kGaugeLift = 0.30f;
constexpr float kGaugeSink = 0.25f;

[[nodiscard]] engine::RectF toRectF(const engine::Rect& r) {
    return engine::RectF{static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.w),
                         static_cast<float>(r.h)};
}

}  // namespace

engine::Color mixColor(const engine::Color& a, const engine::Color& b, float t) {
    const float k = std::clamp(t, 0.f, 1.f);
    const auto channel = [k](std::uint8_t from, std::uint8_t to) {
        const float lo = static_cast<float>(from);
        const float value = lo + (static_cast<float>(to) - lo) * k;
        return static_cast<std::uint8_t>(std::clamp(value + 0.5f, 0.f, 255.f));
    };
    return engine::Color{channel(a.r, b.r), channel(a.g, b.g), channel(a.b, b.b), a.a};
}

void drawFrame(engine::Engine& engine, const engine::RectF& r, const engine::Color& color) {
    if (r.w <= 0.f || r.h <= 0.f) return;
    engine.fillRect(engine::RectF{r.x, r.y, r.w, 1.f}, color, engine::BlendMode::Alpha);
    engine.fillRect(engine::RectF{r.x, r.y + r.h - 1.f, r.w, 1.f}, color, engine::BlendMode::Alpha);
    // 左右两竖让开上下两横已经占过的那一像素：半透明的框在四个角上叠两遍会亮出四个点。
    if (r.h > 2.f) {
        const float side = r.h - 2.f;
        engine.fillRect(engine::RectF{r.x, r.y + 1.f, 1.f, side}, color, engine::BlendMode::Alpha);
        engine.fillRect(engine::RectF{r.x + r.w - 1.f, r.y + 1.f, 1.f, side}, color,
                        engine::BlendMode::Alpha);
    }
}

void drawDiamond(engine::Engine& engine, float cx, float cy, float radius,
                 const engine::Color& color) {
    if (radius <= 0.f) return;
    // 四个顶点、两个三角形。按顶点序每三个一组（indices 留空），省一份下标表。
    const std::vector<engine::Vertex> vertices{
        {cx, cy - radius, color, 0.f, 0.f}, {cx + radius, cy, color, 0.f, 0.f},
        {cx, cy + radius, color, 0.f, 0.f}, {cx, cy - radius, color, 0.f, 0.f},
        {cx, cy + radius, color, 0.f, 0.f}, {cx - radius, cy, color, 0.f, 0.f},
    };
    engine.drawGeometry(engine::kInvalidTexture, vertices, {}, engine::BlendMode::Alpha);
}

void drawSelection(engine::Engine& engine, const engine::Rect& row, const Theme& theme) {
    if (row.w <= 0 || row.h <= 0) return;
    const engine::RectF bar = toRectF(row);
    engine.drawGradientH(bar, withAlpha(theme.gold, theme.cursorAlpha), withAlpha(theme.gold, 0),
                         engine::BlendMode::Alpha);
    drawDiamond(engine, bar.x, bar.y + bar.h / 2.f, kPointerDiamond, theme.goldBright);
}

void drawOrnament(engine::Engine& engine, float centerX, float y, float halfWidth,
                  const Theme& theme) {
    if (halfWidth <= 0.f) return;
    // 两段线从菱形两侧往外淡出；中间让出菱形的宽度，线不压在结上。
    const float gap = kPointerDiamond + 3.f;
    if (halfWidth > gap) {
        const float len = halfWidth - gap;
        engine.drawGradientH(engine::RectF{centerX - halfWidth, y, len, 1.f},
                             withAlpha(theme.gold, 0), theme.gold, engine::BlendMode::Alpha);
        engine.drawGradientH(engine::RectF{centerX + gap, y, len, 1.f}, theme.gold,
                             withAlpha(theme.gold, 0), engine::BlendMode::Alpha);
    }
    drawDiamond(engine, centerX, y + 0.5f, kPointerDiamond, theme.gold);
    drawDiamond(engine, centerX, y + 0.5f, kPointerDiamond - 2.f, theme.goldBright);
}

void drawPlate(engine::Engine& engine, const engine::Rect& area, const Theme& theme) {
    if (area.w <= 0 || area.h <= 0) return;
    const engine::RectF r = toRectF(area);
    engine.drawGradient(r, theme.inkDeep, mixColor(theme.inkDeep, theme.ink, 0.6f),
                        engine::BlendMode::Alpha);
    drawFrame(engine, r, theme.gold);
    // 两端的小菱形压在框的左右边正中：一眼看得出这是一块「牌」而不是又一个面板。
    drawDiamond(engine, r.x, r.y + r.h / 2.f, kCornerDiamond, theme.gold);
    drawDiamond(engine, r.x + r.w, r.y + r.h / 2.f, kCornerDiamond, theme.gold);
}

void drawScrim(engine::Engine& engine, const Theme& theme) {
    engine.fillRect(engine::RectF{0.f, 0.f, static_cast<float>(engine::kLogicalWidth),
                                  static_cast<float>(engine::kLogicalHeight)},
                    theme.scrim, engine::BlendMode::Alpha);
}

void drawRuleH(engine::Engine& engine, float left, float y, float width, const Theme& theme) {
    if (width <= 0.f) return;
    const float half = width / 2.f;
    engine.drawGradientH(engine::RectF{left, y, half, 1.f}, withAlpha(theme.goldDim, 0),
                         theme.goldDim, engine::BlendMode::Alpha);
    engine.drawGradientH(engine::RectF{left + half, y, width - half, 1.f}, theme.goldDim,
                         withAlpha(theme.goldDim, 0), engine::BlendMode::Alpha);
}

void drawRuleV(engine::Engine& engine, float x, float top, float height, const Theme& theme) {
    if (height <= 0.f) return;
    const float half = height / 2.f;
    engine.drawGradient(engine::RectF{x, top, 1.f, half}, withAlpha(theme.goldDim, 0),
                        theme.goldDim, engine::BlendMode::Alpha);
    engine.drawGradient(engine::RectF{x, top + half, 1.f, height - half}, theme.goldDim,
                        withAlpha(theme.goldDim, 0), engine::BlendMode::Alpha);
}

int spacedTextWidth(const engine::Engine& engine, const std::string& utf8, int size, int spacing) {
    const std::vector<std::string> glyphs = engine::splitGraphemes(utf8);
    int total = 0;
    for (const std::string& glyph : glyphs) total += engine.measureText(glyph, size).x;
    if (glyphs.size() > 1) total += spacing * static_cast<int>(glyphs.size() - 1);
    return total;
}

void drawSpacedText(engine::Engine& engine, const std::string& utf8, float centerX, int y,
                    int size, const engine::Color& color, const engine::TextStyle& style,
                    int spacing) {
    // 逐字画：每个字自己一张缓存纹理，拉开的字距才准。整行交给引擎画就只能是默认字距。
    const int total = spacedTextWidth(engine, utf8, size, spacing);
    int x = static_cast<int>(centerX) - total / 2;
    for (const std::string& glyph : engine::splitGraphemes(utf8)) {
        engine.drawText(glyph, x, y, size, color, style);
        x += engine.measureText(glyph, size).x + spacing;
    }
}

void drawPanel(engine::Engine& engine, const engine::Rect& area, const std::string& title,
               const Theme& theme) {
    // 退化矩形直接不画：调用方在做屏幕分割时算出零宽零高是常事，
    // 与其在每个面板里各判一次，不如在这里兜住。
    if (area.w <= 0 || area.h <= 0) return;

    const engine::RectF r = toRectF(area);
    // 上深下浅：底部更透，地图从面板下沿透上来一点，面板读起来是「浮」在场景上的。
    engine.drawGradient(r, theme.ink, theme.inkLow, engine::BlendMode::Alpha);
    if (r.w > kInnerFrameInset * 2.f + 2.f && r.h > kInnerFrameInset * 2.f + 2.f) {
        drawFrame(engine,
                  engine::RectF{r.x + kInnerFrameInset, r.y + kInnerFrameInset,
                                r.w - kInnerFrameInset * 2.f, r.h - kInnerFrameInset * 2.f},
                  theme.goldDim);
    }
    drawFrame(engine, r, theme.gold);
    drawDiamond(engine, r.x, r.y, kCornerDiamond, theme.gold);
    drawDiamond(engine, r.x + r.w, r.y, kCornerDiamond, theme.gold);
    drawDiamond(engine, r.x, r.y + r.h, kCornerDiamond, theme.gold);
    drawDiamond(engine, r.x + r.w, r.y + r.h, kCornerDiamond, theme.gold);
    if (title.empty()) return;

    const int x = area.x + theme.padding;
    const int y = area.y + theme.lineSpacing;
    const int bottom = area.y + area.h;
    // 面板比标题带还矮时宁可不画标题：把字渗到边框外面，比少一行标题难看得多。
    if (y + theme.titleFontSize > bottom) return;
    engine.drawText(title, x, y, theme.titleFontSize, theme.goldBright, theme.titleStyle);

    // 标题下的金线：落点与改造前那道分隔线同一行（标题带高度因此一个像素没变）。
    const int ruleY = y + theme.titleFontSize + theme.lineSpacing / 2;
    const int ruleW = area.w - theme.padding * 2;
    if (ruleW > 0 && ruleY < bottom) {
        engine.drawGradientH(engine::RectF{static_cast<float>(x), static_cast<float>(ruleY),
                                           static_cast<float>(ruleW), 1.f},
                             withAlpha(theme.gold, kTitleRuleAlpha), withAlpha(theme.gold, 0),
                             engine::BlendMode::Alpha);
        drawDiamond(engine, static_cast<float>(x), static_cast<float>(ruleY) + 0.5f,
                    kCornerDiamond - 1.f, theme.goldBright);
    }
}

int listRowHeight(const Theme& theme) {
    // 行高为 0 会让「能画几行」变成除零。字号与行距都是皮肤给的数，
    // 皮肤把它们配成 0 是配置错误，但不该表现为崩溃。
    return std::max(1, theme.bodyFontSize + theme.lineSpacing);
}

int listAreaHeight(int rows, const Theme& theme) {
    if (rows <= 0) return 0;
    // rows 多半直接来自条目数，而条目数是数据决定的。抬到 64 位再夹，
    // 免得「背包里几千件东西」这种输入让 int 乘法溢出成负高度。
    const long long height = static_cast<long long>(rows) * listRowHeight(theme) +
                             static_cast<long long>(theme.padding) * 2;
    return static_cast<int>(std::min<long long>(height, std::numeric_limits<int>::max()));
}

int listRowsThatFit(int areaHeight, const Theme& theme) {
    const int innerH = areaHeight - theme.padding * 2;
    if (innerH <= 0) return 0;
    // 至少一行：区域比一行还矮时画一行被截的总比一片空白强，
    // 玩家至少看得出这里有东西。ListView::render 也是这么办的。
    return std::max(1, innerH / listRowHeight(theme));
}

int panelTitleBandHeight(bool hasTitle, const Theme& theme) {
    // 标题上方的行距 + 字号 + 分隔线下的呼吸位。
    if (!hasTitle) return 0;
    return theme.lineSpacing + theme.titleFontSize + theme.lineSpacing;
}

engine::Rect panelContentArea(const engine::Rect& area, const std::string& title,
                              const Theme& theme) {
    const int titleBand = panelTitleBandHeight(!title.empty(), theme);
    engine::Rect inner{area.x, area.y + titleBand, area.w, area.h - titleBand};
    // 标题比面板还高时正文区会变成负高度，直接压到 0：
    // 调用方拿到的矩形必须是能安全传给任何绘制函数的。
    inner.w = std::max(0, inner.w);
    inner.h = std::max(0, inner.h);
    return inner;
}

void drawTextBlock(engine::Engine& engine, const std::string& utf8, const engine::Rect& area,
                   int fontSize, const Theme& theme) {
    if (utf8.empty() || fontSize <= 0) return;

    const int innerW = area.w - theme.padding * 2;
    const int innerH = area.h - theme.padding * 2;
    if (innerW <= 0 || innerH <= 0) return;

    // 断行与中文标点禁则一律交给 engine::layoutText。全项目只许有一套断行算法：
    // 各面板各写一份必然走样，P1 的对话框已经证明过这条。
    const auto measure = [&engine, fontSize](const std::string& sample) {
        return engine.measureText(sample, fontSize).x;
    };
    const std::vector<engine::LayoutLine> lines = engine::layoutText(utf8, innerW, measure);

    const int rowHeight = fontSize + theme.lineSpacing;
    const int bottom = area.y + area.h - theme.padding;
    int y = area.y + theme.padding;
    for (const engine::LayoutLine& line : lines) {
        // 画不下的行直接截断，不许溢出面板：宁可少一行，也不能让文字压到边框外。
        if (y + fontSize > bottom) break;
        if (!line.text.empty()) {
            engine.drawText(line.text, area.x + theme.padding, y, fontSize, theme.paper,
                            theme.bodyStyle);
        }
        y += rowHeight;
    }
}

int gaugeFillWidth(int width, int current, int maximum) {
    // 上限为 0 的条只能是空条。先挡在这里，后面的除法才不必担心除零。
    if (width <= 0 || maximum <= 0) return 0;
    // 负血量（掉到 0 以下的那一帧）按空条画，不要反着画到框外去。
    if (current <= 0) return 0;
    // 超上限（吃了增益药、护盾溢出）按满条画，不要画出边框。
    if (current >= maximum) return width;

    // 用 64 位中转：气血上限日后完全可能上万，int 乘法先溢出再除会得到负宽度，
    // 而负宽度传给渲染器就是一条向左长出去的条。
    const long long scaled = static_cast<long long>(width) * static_cast<long long>(current) /
                             static_cast<long long>(maximum);
    return static_cast<int>(std::clamp<long long>(scaled, 0, static_cast<long long>(width)));
}

void drawGauge(engine::Engine& engine, const engine::Rect& area, int current, int maximum,
               const engine::Color& fill, const Theme& theme) {
    if (area.w <= 0 || area.h <= 0) return;

    const engine::RectF slot = toRectF(area);
    engine.fillRect(slot, theme.inkDeep, engine::BlendMode::Alpha);   // 底槽
    const int filled = gaugeFillWidth(area.w, current, maximum);
    if (filled > 0) {
        const engine::RectF bar{slot.x, slot.y, static_cast<float>(filled), slot.h};
        engine.drawGradient(bar, mixColor(fill, theme.paper, kGaugeLift),
                            mixColor(fill, theme.inkDeep, kGaugeSink), engine::BlendMode::Alpha);
        // 高光只占顶上四分之一（至少一像素）：一道窄光比整条提亮更像「有厚度」。
        const float sheenH = std::max(1.f, slot.h / 4.f);
        engine.fillRect(engine::RectF{bar.x, bar.y, bar.w, sheenH}, theme.sheen,
                        engine::BlendMode::Alpha);
    }
    drawFrame(engine, slot, theme.goldDim);   // 边框压在最上层，盖住填充的边缘
}

}  // namespace fanren::ui
