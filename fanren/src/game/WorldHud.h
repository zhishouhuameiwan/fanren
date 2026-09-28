#pragma once
// 世界画面上压在后处理之外的那一层：地名横幅、目标框、目标菱形与边缘箭头、名牌、存盘提示。
//
// 这些都画在 PostFx::endScene 之后（施工图 1.4：UI 不糊、不压暗、不被调色）。
// 文字一律按 1280×720 原生绘制，不像素化——「像素世界 + 高清字」本身就是 HD-2D 的一部分（施工图 1.1）。
// 颜色与字样一律取 ui::Theme 的墨金 token（app.theme()），这里不另起字面量。
#include <string>
#include <vector>

#include "engine/Engine.h"
#include "ui/Widgets.h"

namespace fanren::game::hud {


// 攒一批三角形一次画掉。缓冲每帧复用（每画一次新建两个 vector 是最容易照抄走的坏习惯）。
class Geometry {
public:
    void quad(float x0, float y0, float x1, float y1, const engine::Color& top,
              const engine::Color& bottom);
    void polygon4(const engine::Vertex& a, const engine::Vertex& b, const engine::Vertex& c,
                  const engine::Vertex& d);
    void triangle(const engine::Vertex& a, const engine::Vertex& b, const engine::Vertex& c);
    // 实心扇形椭圆：中心 center 色、边缘 rim 色（柔影、光圈）。
    void fan(float cx, float cy, float rx, float ry, const engine::Color& center,
             const engine::Color& rim, int segments = 24);
    void diamond(float cx, float cy, float rx, float ry, const engine::Color& color);
    void draw(engine::Engine& engine, engine::BlendMode blend,
              engine::TextureId texture = engine::kInvalidTexture);

    // 直接往缓冲里加顶点（带纹理的网格）；下标相对这一批的起点。
    [[nodiscard]] int vertexCount() const { return static_cast<int>(vertices_.size()); }
    void vertex(const engine::Vertex& v) { vertices_.push_back(v); }
    void index(int i) { indices_.push_back(i); }

private:
    std::vector<engine::Vertex> vertices_;
    std::vector<int> indices_;
};

// 颜色的透明度乘一个系数（淡入淡出）。
[[nodiscard]] engine::Color faded(engine::Color color, float alpha);

// 墨金面板：竖向渐变底 + 1px 金线外框 + 内缩 3px 的暗金线 + 四角小菱形。alpha 整体淡入淡出。
void drawPanel(engine::Engine& engine, const ui::Theme& theme, Geometry& geometry,
               const engine::RectF& rect, float alpha);

// 名牌占的矩形（排版避让用）；无头或空串时宽高为 0。
[[nodiscard]] engine::RectF namePlateRect(engine::Engine& engine, const std::string& label, float centerX,
                                          float bottomY);

// 名牌：描边字，不要底条（施工图：描边文字，不要底条也读得清）。centerX 对中，bottomY 是字的底边。
void drawNamePlate(engine::Engine& engine, const ui::Theme& theme, const std::string& label,
                   float centerX, float bottomY);

// 左上角的目标框：一行目标、一行「前往　X」（可以为空）。alpha 整体淡（盖住主角时）。
// objectiveBoxRect 是它会占的矩形（没有目标、无头量不出字时宽高为 0）。
[[nodiscard]] engine::RectF objectiveBoxRect(engine::Engine& engine, float x, float y, const std::string& text,
                                             const std::string& where);
void drawObjectiveBox(engine::Engine& engine, const ui::Theme& theme, Geometry& geometry, float x, float y,
                      const std::string& text, const std::string& where, float alpha);

// 右上角的存盘提示。
void drawSaveNotice(engine::Engine& engine, const ui::Theme& theme, Geometry& geometry,
                    const std::string& text);

// 目标菱形：悬在目标头顶。glow 是柔光贴图（可为无效句柄，只是少一圈光）。
void drawObjectiveDiamond(engine::Engine& engine, const ui::Theme& theme, Geometry& geometry,
                          engine::TextureId glow, float cx, float cy);

// 屏幕边缘的柔光箭头：(x, y) 是箭头尖，(dx, dy) 是朝向（不必归一），label 写在箭头内侧。
void drawEdgeArrow(engine::Engine& engine, const ui::Theme& theme, Geometry& geometry,
                   engine::TextureId glow, float x, float y, float dx, float dy,
                   const std::string& label);

// 地名横幅。字画进一张离屏画布、只在地名变了时重画一次：淡入淡出要逐帧改透明度，
// 而逐帧改文字颜色会逐帧新建文字纹理（docs/interfaces-octo-engine.md 2.5）。
class PlaceBanner {
public:
    // 地名或危险星级变了才会重画。dangerStars > 0 时地名下多一行「凶险　★★☆☆☆」
    //（docs/interfaces-octo-encounters.md 第 6 节）；0 = 这里没有遭遇，那一行不画。
    void set(std::string title, int dangerStars);
    // 危险度那一行打头的界面词（data/text/ui.json 的 ui.world.danger）。横幅拿不到文案表，
    // 由 Application::init 读完文案后交进来——与 WorldScene::setPathActionHooks 同一个做法。
    static void setDangerLabel(std::string label);
    // 危险度那一行的正文：界面词、全角空格、实星 stars 颗、空星补足 kMaxStars 颗。
    static constexpr int kMaxStars = 5;
    [[nodiscard]] static std::string dangerLine(int stars);
    // alpha 为 0 时什么也不画。
    void draw(engine::Engine& engine, const ui::Theme& theme, Geometry& geometry, float alpha);
    // 横幅占到屏幕多低（目标框往下排）。
    [[nodiscard]] static float bottom();
    // 横幅这一刻占的矩形（字还没排出来时宽高为 0）。
    [[nodiscard]] engine::RectF area() const;

private:
    void build(engine::Engine& engine, const ui::Theme& theme);

    std::string title_;
    int dangerStars_ = 0;
    bool dirty_ = true;
    engine::OwnedTexture text_;
    float textW_ = 0.f;
    float textH_ = 0.f;
    // 危险度那一行，与地名同一个理由画进离屏画布（逐帧改透明度不逐帧新建文字纹理）。
    engine::OwnedTexture danger_;
    float dangerW_ = 0.f;
    float dangerH_ = 0.f;
};

}  // namespace fanren::game::hud
