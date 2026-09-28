#include "game/FxDemoScene.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <utility>

#include "game/Application.h"

namespace fanren::game {

using engine::BlendMode;
using engine::Color;
using engine::Engine;
using engine::ParticleKind;
using engine::ParticleSpace;
using engine::PostFxSettings;
using engine::RectF;
using engine::TextStyle;
using engine::Vertex;

// 一张预设 = 天色 + 布景开关 + 后处理参数 + 至多三层粒子。
// 下游地图预设（data/visual/maps.json → meta.json）将来要装的就是这几样东西。
struct FxDemoScene::Preset {
    struct LayerSpec {
        ParticleKind kind = ParticleKind::Dust;
        float rate = 0.f;            // 0 = 这一层不用
        RectF area{};
        ParticleSpace space = ParticleSpace::Screen;
        bool emissive = false;       // 再画一份进辉光通道
        bool front = true;           // 画在亭子与人物之前（false：画在远山之后、亭子之前）
    };

    const char* name;
    const char* title;
    const char* lighting;
    const char* effects;
    const char* particles;
    Color skyTop;
    Color skyBottom;
    bool stars;
    bool moon;
    bool lanternsLit;
    bool sunShafts;
    PostFxSettings post;
    std::array<LayerSpec, 3> layers;
};

namespace {

constexpr float kW = static_cast<float>(engine::kLogicalWidth);
constexpr float kH = static_cast<float>(engine::kLogicalHeight);
constexpr float kTwoPi = 2.f * std::numbers::pi_v<float>;

// 这一幕没有镜头移动；世界空间的粒子照样传镜头，演示两种坐标怎么接。
constexpr float kCameraX = 0.f;
constexpr float kCameraY = 0.f;

// 开场先让粒子空跑一阵：截图的第一帧就是「雪已经下了一会儿」的样子。
// 要够最慢的那种从顶上落到底：雪每秒二三十像素，穿过 720 像素要二三十秒。
constexpr float kPrewarmSeconds = 30.f;

// 墨金主题的几个色（施工图 1.6 节）。界面路落地 ui::Theme 之后这里改用它。
constexpr Color kInk{14, 17, 25, 219};
constexpr Color kInkLow{14, 17, 25, 184};
constexpr Color kInkDeep{7, 9, 14, 230};
constexpr Color kGold{201, 164, 92, 255};
constexpr Color kGoldDim{120, 98, 56, 255};
constexpr Color kGoldBright{242, 217, 139, 255};
constexpr Color kPaper{241, 234, 216, 255};
constexpr Color kPaperDim{156, 148, 128, 255};

// 布景的位置。
constexpr float kHorizonY = 440.f;
constexpr std::array<float, 5> kPillarX{170.f, 410.f, 650.f, 890.f, 1110.f};
constexpr std::array<float, 4> kLanternX{290.f, 530.f, 770.f, 1000.f};
constexpr float kLanternY = 252.f;
constexpr float kBrazierX = 300.f;
constexpr float kBrazierTop = 552.f;
constexpr float kMoonX = 1040.f;
constexpr float kMoonY = 96.f;
constexpr float kFeetY = 552.f;
constexpr std::array<float, 2> kFigureX{580.f, 722.f};

const std::array<FxDemoScene::Preset, 4> kPresets{{
    {
        .name = "night",
        .title = "后处理自检 · 夜",
        .lighting = "夜间环境光　灯笼 ×4　火盆　月光　人物微光",
        .effects = "辉光　景深（移轴）　暗角　调色",
        .particles = "粒子：薄雾　萤火　火星",
        .skyTop = {10, 14, 34, 255},
        .skyBottom = {46, 46, 84, 255},
        .stars = true,
        .moon = true,
        .lanternsLit = true,
        .sunShafts = false,
        .post = {.ambient = {58, 66, 112, 255},
                 .dof = 1.f,
                 .dofFocus = 0.60f,
                 .dofBand = 0.26f,
                 .bloom = 0.75f,
                 .vignette = 0.6f,
                 .gradeMul = {255, 244, 236, 255},
                 .gradeAdd = {4, 2, 10, 255},
                 .dofRamp = 0.18f},
        .layers = {{
            {ParticleKind::Mist, 0.8f, {-200.f, 330.f, 1680.f, 170.f}, ParticleSpace::Screen, false, false},
            {ParticleKind::Firefly, 5.f, {420.f, 380.f, 760.f, 260.f}, ParticleSpace::World, true, true},
            // 火星不进辉光通道：它本身就是 Add 的亮点，再叠一遍光晕会烧成一根白柱子。
            {ParticleKind::Ember, 14.f, {kBrazierX - 22.f, 540.f, 44.f, 12.f}, ParticleSpace::World, false, true},
        }},
    },
    {
        .name = "day",
        .title = "后处理自检 · 昼",
        .lighting = "日光（环境光纯白，不做光照）　光柱",
        .effects = "辉光　景深（移轴）　暗角　调色",
        .particles = "粒子：尘埃光点　花瓣　落叶",
        .skyTop = {104, 156, 212, 255},
        .skyBottom = {204, 222, 230, 255},
        .stars = false,
        .moon = false,
        .lanternsLit = false,
        .sunShafts = true,
        .post = {.ambient = {255, 255, 255, 255},
                 .dof = 0.9f,
                 .dofFocus = 0.60f,
                 .dofBand = 0.26f,
                 .bloom = 0.35f,
                 .vignette = 0.35f,
                 .gradeMul = {255, 248, 232, 255},
                 .gradeAdd = {6, 4, 0, 255},
                 .dofRamp = 0.18f},
        .layers = {{
            {ParticleKind::Dust, 10.f, {260.f, 160.f, 760.f, 420.f}, ParticleSpace::Screen, true, true},
            {ParticleKind::Petal, 5.f, {0.f, 0.f, kW, kH}, ParticleSpace::Screen, false, true},
            {ParticleKind::Leaf, 2.f, {0.f, 0.f, kW, kH}, ParticleSpace::Screen, false, true},
        }},
    },
    {
        .name = "rain",
        .title = "后处理自检 · 雨",
        .lighting = "阴天环境光　灯笼 ×4　火盆",
        .effects = "辉光　景深（移轴）　暗角　调色",
        .particles = "粒子：雨　薄雾",
        .skyTop = {40, 46, 60, 255},
        .skyBottom = {84, 92, 106, 255},
        .stars = false,
        .moon = false,
        .lanternsLit = true,
        .sunShafts = false,
        .post = {.ambient = {118, 124, 150, 255},
                 .dof = 0.9f,
                 .dofFocus = 0.60f,
                 .dofBand = 0.26f,
                 .bloom = 0.6f,
                 .vignette = 0.5f,
                 .gradeMul = {222, 230, 255, 255},
                 .gradeAdd = {0, 0, 0, 255},
                 .dofRamp = 0.18f},
        .layers = {{
            {ParticleKind::Mist, 0.5f, {-200.f, 330.f, 1680.f, 170.f}, ParticleSpace::Screen, false, false},
            {ParticleKind::Rain, 420.f, {0.f, 0.f, kW, kH}, ParticleSpace::Screen, false, true},
            {},
        }},
    },
    {
        .name = "snow",
        .title = "后处理自检 · 雪",
        .lighting = "雪天环境光　灯笼 ×4　火盆",
        .effects = "辉光　景深（移轴）　暗角　调色",
        .particles = "粒子：雪　薄雾",
        .skyTop = {92, 104, 132, 255},
        .skyBottom = {170, 180, 200, 255},
        .stars = false,
        .moon = false,
        .lanternsLit = true,
        .sunShafts = false,
        .post = {.ambient = {172, 180, 212, 255},
                 .dof = 0.9f,
                 .dofFocus = 0.60f,
                 .dofBand = 0.26f,
                 .bloom = 0.5f,
                 .vignette = 0.45f,
                 .gradeMul = {236, 242, 255, 255},
                 .gradeAdd = {0, 0, 0, 255},
                 .dofRamp = 0.18f},
        .layers = {{
            {ParticleKind::Mist, 0.4f, {-200.f, 330.f, 1680.f, 170.f}, ParticleSpace::Screen, false, false},
            {ParticleKind::Snow, 36.f, {0.f, 0.f, kW, kH}, ParticleSpace::Screen, false, true},
            {},
        }},
    },
}};

const FxDemoScene::Preset* findPreset(const std::string& name) {
    const std::string wanted = name.empty() ? std::string("night") : name;
    for (const FxDemoScene::Preset& preset : kPresets) {
        if (wanted == preset.name) {
            return &preset;
        }
    }
    return nullptr;
}

// 人物：16×24 的程序像素图（施工图 1.1 节：行走人物 16×24，×3 画成 48×72）。
// K 发、S 肤、E 眼、W 领、R 袍、D 袍暗面、G 腰带、H 剑鞘、B 靴、. 透明。
// 剑鞘只挂在右边：左右不对称，翻转（flipX）才看得出来。
constexpr std::array<const char*, 24> kFigureRows{{
    "......KKKK......",
    ".....KKKKKK.....",
    "......KKKK......",
    "....KKKKKKKK....",
    "...KKKKKKKKKK...",
    "...KSSSSSSSSK...",
    "...SSESSSSESS...",
    "...SSSSSSSSSS...",
    "....SSSSSSSS....",
    ".....SSSSSS.....",
    "....RRWWWWRR....",
    "...RRRRWWRRRRG..",
    "..RRRRRRRRRRRRH.",
    "..RRRRRRRRRRRRH.",
    ".SRRRRRRRRRRRRSH",
    ".SRRGGGGGGGGRRSH",
    "..RRRRRRRRRRRR.H",
    "..RRRRRRRRRRRR.H",
    "..DRRRRRRRRRRD.H",
    "..DRRRRRRRRRRD..",
    "...DRRRRRRRRD...",
    "....DDDDDDDD....",
    "....BB....BB....",
    "...BBB....BBB...",
}};

constexpr int kFigureW = 16;
constexpr int kFigureH = static_cast<int>(kFigureRows.size());

// 每一行都得恰好 kFigureW 个字符：下面按这个宽度逐字读，短一截就读过了字符串结尾。
constexpr bool figureRowsAreFullWidth() {
    for (const char* row : kFigureRows) {
        int n = 0;
        while (row[n] != '\0') {
            ++n;
        }
        if (n != kFigureW) {
            return false;
        }
    }
    return true;
}
static_assert(figureRowsAreFullWidth(), "人物像素图每一行必须恰好 16 个字符");

std::vector<std::uint32_t> figurePixels(const Color& robe, const Color& robeDark) {
    std::vector<std::uint32_t> pixels;
    pixels.reserve(static_cast<std::size_t>(kFigureW * kFigureH));
    for (const char* row : kFigureRows) {
        for (int x = 0; x < kFigureW; ++x) {
            Color c{0, 0, 0, 0};
            switch (row[x]) {
                case 'K': c = {28, 26, 34, 255}; break;
                case 'S': c = {232, 196, 158, 255}; break;
                case 'E': c = {34, 28, 34, 255}; break;
                case 'W': c = {232, 228, 214, 255}; break;
                case 'R': c = robe; break;
                case 'D': c = robeDark; break;
                case 'G': c = {201, 164, 92, 255}; break;
                case 'H': c = {72, 44, 30, 255}; break;
                case 'B': c = {44, 36, 32, 255}; break;
                default: break;
            }
            pixels.push_back(engine::packRgba(c));
        }
    }
    return pixels;
}

// 星星的位置：确定性的散列，同一幕每次都是同一片星空。
float hash01(std::uint32_t n) {
    n = (n ^ 61u) ^ (n >> 16);
    n *= 9u;
    n ^= n >> 4;
    n *= 0x27D4EB2Du;
    n ^= n >> 15;
    return static_cast<float>(n & 0xFFFFFFu) / 16777216.f;
}

float farRidge(float x) {
    return 330.f - 46.f * std::sin(x * 0.0042f + 0.8f) - 22.f * std::sin(x * 0.011f + 2.1f) -
           9.f * std::sin(x * 0.031f);
}

float nearRidge(float x) {
    return 384.f - 28.f * std::sin(x * 0.0061f + 2.4f) - 13.f * std::sin(x * 0.019f + 0.3f);
}

Vertex vtx(float x, float y, const Color& c) {
    return Vertex{x, y, c, 0.f, 0.f};
}

void frame(Engine& e, const RectF& r, const Color& c) {
    e.fillRect({r.x, r.y, r.w, 1.f}, c, BlendMode::Alpha);
    e.fillRect({r.x, r.y + r.h - 1.f, r.w, 1.f}, c, BlendMode::Alpha);
    e.fillRect({r.x, r.y, 1.f, r.h}, c, BlendMode::Alpha);
    e.fillRect({r.x + r.w - 1.f, r.y, 1.f, r.h}, c, BlendMode::Alpha);
}

}  // namespace

// ---------------------------------------------------------------------------
// Mesh
// ---------------------------------------------------------------------------

void FxDemoScene::Mesh::quad(float x0, float y0, float x1, float y1, const Color& top,
                             const Color& bottom) {
    polygon4(vtx(x0, y0, top), vtx(x1, y0, top), vtx(x1, y1, bottom), vtx(x0, y1, bottom));
}

void FxDemoScene::Mesh::polygon4(const Vertex& a, const Vertex& b, const Vertex& c,
                                 const Vertex& d) {
    const int base = static_cast<int>(vertices_.size());
    vertices_.insert(vertices_.end(), {a, b, c, d});
    indices_.insert(indices_.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

void FxDemoScene::Mesh::triangle(const Vertex& a, const Vertex& b, const Vertex& c) {
    const int base = static_cast<int>(vertices_.size());
    vertices_.insert(vertices_.end(), {a, b, c});
    indices_.insert(indices_.end(), {base, base + 1, base + 2});
}

void FxDemoScene::Mesh::fan(float cx, float cy, float rx, float ry, const Color& center,
                            const Color& rim, int segments) {
    const int base = static_cast<int>(vertices_.size());
    vertices_.push_back(vtx(cx, cy, center));
    for (int i = 0; i < segments; ++i) {
        const float a = kTwoPi * static_cast<float>(i) / static_cast<float>(segments);
        vertices_.push_back(vtx(cx + rx * std::cos(a), cy + ry * std::sin(a), rim));
    }
    for (int i = 0; i < segments; ++i) {
        indices_.insert(indices_.end(), {base, base + 1 + i, base + 1 + (i + 1) % segments});
    }
}

void FxDemoScene::Mesh::draw(Engine& engine, BlendMode blend, engine::TextureId texture) {
    if (!vertices_.empty()) {
        engine.drawGeometry(texture, vertices_, indices_, blend);
    }
    vertices_.clear();
    indices_.clear();
}

// ---------------------------------------------------------------------------
// FxDemoScene
// ---------------------------------------------------------------------------

FxDemoScene::FxDemoScene(std::string preset) : presetName_(std::move(preset)) {
    // 认不出的名字 main 已在压栈前拦下（knowsPreset），这里按缺省的夜景画。
    preset_ = findPreset(presetName_);
    if (preset_ == nullptr) {
        preset_ = &kPresets.front();
    }
}

FxDemoScene::~FxDemoScene() = default;

bool FxDemoScene::knowsPreset(const std::string& preset) {
    return findPreset(preset) != nullptr;
}

void FxDemoScene::onEnter(Application& app) {
    Engine& e = app.engine();
    fx_ = std::make_unique<engine::PostFx>(e);

    // 每层一个种子：同一幕每次进来粒子都长一个样，截图可以前后对比。
    std::uint32_t seed = 7;
    layers_.reserve(preset_->layers.size());
    for (const Preset::LayerSpec& spec : preset_->layers) {
        if (spec.rate <= 0.f) {
            continue;
        }
        Layer layer{engine::ParticleSystem(seed++), spec.emissive, spec.front};
        layer.system.configure(spec.kind, spec.rate, spec.area, spec.space);
        layer.system.prewarm(kPrewarmSeconds);
        layers_.push_back(std::move(layer));
    }

    figures_[0] = engine::OwnedTexture(
        e, e.createTexture(kFigureW, kFigureH, figurePixels({74, 116, 124, 255}, {48, 78, 86, 255}),
                           engine::ScaleMode::Pixel));
    figures_[1] = engine::OwnedTexture(
        e, e.createTexture(kFigureW, kFigureH, figurePixels({152, 108, 64, 255}, {104, 72, 44, 255}),
                           engine::ScaleMode::Pixel));
}

bool FxDemoScene::update(Application& app, double deltaSeconds) {
    const float dt = static_cast<float>(deltaSeconds);
    time_ += dt;
    for (Layer& layer : layers_) {
        layer.system.update(dt);
    }
    // 这一幕只供自检，Esc 是唯一的出口。
    return !app.engine().keyPressed(Engine::Key::Cancel);
}

void FxDemoScene::render(Application& app) {
    Engine& e = app.engine();

    // ① 世界画进场景图。beginScene 之后的一切都要经过后处理。
    fx_->beginScene();
    drawBackdrop(e);
    for (Layer& layer : layers_) {
        if (!layer.front) {
            layer.system.render(e, kCameraX, kCameraY);
        }
    }
    drawGround(e);
    drawPavilion(e);
    drawLanterns(e);
    drawBrazier(e);
    drawFigures(e);
    for (Layer& layer : layers_) {
        if (layer.front) {
            layer.system.render(e, kCameraX, kCameraY);
        }
    }
    drawShafts(e);

    // ② 发光体另画一份进辉光通道。返回 false（无头、建不出目标）时一笔都不许画，
    //    否则这些光晕会直接糊在场景上。
    if (fx_->beginEmissive()) {
        drawEmissive(e);
        fx_->endEmissive();
    }

    // ③ 光源：屏幕像素坐标，与画世界用的是同一套。
    addLights();

    // ④ 合成：光照 → 辉光 → 景深 → 暗角 → 调色，结果落回后缓冲。
    fx_->endScene(preset_->post);

    // ⑤ UI 在后缓冲上直接画：不糊、不压暗、不被调色。
    drawOverlay(e);
}

void FxDemoScene::drawBackdrop(Engine& e) {
    e.drawGradient({0.f, 0.f, kW, kHorizonY}, preset_->skyTop, preset_->skyBottom,
                   BlendMode::Alpha);

    if (preset_->stars) {
        for (std::uint32_t i = 0; i < 46; ++i) {
            const float x = hash01(i * 2u + 1u) * kW;
            const float y = hash01(i * 2u + 2u) * 300.f;
            const float twinkle = 0.6f + 0.4f * std::sin(time_ * 1.3f + static_cast<float>(i));
            const auto a = static_cast<std::uint8_t>(90.f + 130.f * twinkle);
            e.fillRect({x, y, 2.f, 2.f}, {220, 226, 255, a}, BlendMode::Alpha);
        }
    }
    if (preset_->moon) {
        mesh_.fan(kMoonX, kMoonY, 110.f, 110.f, {140, 150, 200, 70}, {140, 150, 200, 0});
        mesh_.draw(e, BlendMode::Add);
        mesh_.fan(kMoonX, kMoonY, 30.f, 30.f, {252, 250, 236, 255}, {218, 214, 196, 255});
        mesh_.draw(e, BlendMode::Alpha);
    }
    if (preset_->sunShafts) {
        // 太阳在画框左上角外面：只露出一团暖光。
        mesh_.fan(120.f, 20.f, 260.f, 200.f, {255, 244, 214, 150}, {255, 244, 214, 0});
        mesh_.draw(e, BlendMode::Add);
    }

    // 两重远山：顶点色从山脊到山脚渐暗，远的那重偏蓝偏淡（空气透视）。
    const auto ridge = [&](float (*height)(float), const Color& top, const Color& bottom) {
        for (float x = 0.f; x < kW; x += 32.f) {
            mesh_.polygon4(vtx(x, height(x), top), vtx(x + 32.f, height(x + 32.f), top),
                           vtx(x + 32.f, kHorizonY, bottom), vtx(x, kHorizonY, bottom));
        }
        mesh_.draw(e, BlendMode::Alpha);
    };
    const bool dark = preset_->stars || !preset_->sunShafts;
    ridge(farRidge, dark ? Color{40, 46, 76, 255} : Color{132, 156, 186, 255},
          dark ? Color{28, 32, 54, 255} : Color{160, 180, 200, 255});
    ridge(nearRidge, dark ? Color{26, 34, 48, 255} : Color{92, 122, 120, 255},
          dark ? Color{20, 26, 36, 255} : Color{112, 140, 128, 255});
}

void FxDemoScene::drawGround(Engine& e) {
    // 远处草地 → 石板院子 → 近处草地 → 前景灌木。
    e.drawGradient({0.f, 430.f, kW, 44.f}, {40, 60, 50, 255}, {36, 56, 46, 255}, BlendMode::Alpha);
    e.drawGradient({0.f, 474.f, kW, 130.f}, {86, 82, 76, 255}, {104, 100, 92, 255},
                   BlendMode::Alpha);
    // 石板缝：错缝砌，横缝每 26 像素一道。
    const Color seam{58, 54, 50, 170};
    for (int row = 0; row < 5; ++row) {
        const float y0 = 474.f + 26.f * static_cast<float>(row);
        e.drawLine(0.f, y0, kW, y0, seam);
        const float offset = row % 2 == 0 ? 0.f : 32.f;
        for (float x = offset; x < kW; x += 64.f) {
            e.drawLine(x, y0, x, y0 + 26.f, seam);
        }
    }
    e.drawGradient({0.f, 604.f, kW, 48.f}, {36, 62, 46, 255}, {28, 50, 38, 255}, BlendMode::Alpha);
    e.drawGradient({0.f, 652.f, kW, 68.f}, {22, 38, 30, 255}, {14, 24, 20, 255}, BlendMode::Alpha);

    // 前景灌木：离镜头最近，景深会把它们整个糊掉——移轴的「近」就是靠它读出来的。
    const std::array<std::array<float, 4>, 6> bushes{{
        {40.f, 700.f, 110.f, 70.f},
        {210.f, 714.f, 120.f, 64.f},
        {400.f, 726.f, 140.f, 60.f},
        {880.f, 724.f, 130.f, 62.f},
        {1070.f, 708.f, 120.f, 70.f},
        {1240.f, 700.f, 110.f, 76.f},
    }};
    for (const auto& b : bushes) {
        mesh_.fan(b[0], b[1], b[2], b[3], {26, 46, 34, 255}, {12, 24, 18, 255});
    }
    mesh_.draw(e, BlendMode::Alpha);
}

void FxDemoScene::drawPavilion(Engine& e) {
    // 屋顶：梯形 + 两端起翘的檐角。
    const Color roofTop{34, 30, 42, 255};
    const Color roofBottom{58, 50, 62, 255};
    mesh_.polygon4(vtx(200.f, 92.f, roofTop), vtx(1080.f, 92.f, roofTop),
                   vtx(1170.f, 150.f, roofBottom), vtx(110.f, 150.f, roofBottom));
    mesh_.triangle(vtx(110.f, 150.f, roofBottom), vtx(72.f, 124.f, roofBottom),
                   vtx(150.f, 138.f, roofBottom));
    mesh_.triangle(vtx(1170.f, 150.f, roofBottom), vtx(1208.f, 124.f, roofBottom),
                   vtx(1130.f, 138.f, roofBottom));
    mesh_.draw(e, BlendMode::Alpha);
    for (int i = 0; i <= 34; ++i) {
        const float t = static_cast<float>(i) / 34.f;
        e.drawLine(200.f + 880.f * t, 94.f, 110.f + 1060.f * t, 148.f, {24, 20, 30, 200});
    }
    e.fillRect({190.f, 86.f, 900.f, 8.f}, {46, 40, 54, 255}, BlendMode::Alpha);
    e.fillRect({110.f, 147.f, 1060.f, 3.f}, {170, 136, 76, 255}, BlendMode::Alpha);

    // 额枋。
    e.drawGradient({120.f, 151.f, 1040.f, 25.f}, {128, 70, 44, 255}, {86, 46, 30, 255},
                   BlendMode::Alpha);
    e.fillRect({120.f, 151.f, 1040.f, 2.f}, {190, 150, 84, 255}, BlendMode::Alpha);
    e.fillRect({120.f, 174.f, 1040.f, 2.f}, {60, 34, 24, 255}, BlendMode::Alpha);

    // 朱红柱子：左右两半各一道横向渐变，读出圆柱的体积。
    for (const float cx : kPillarX) {
        e.drawGradientH({cx - 17.f, 176.f, 17.f, 368.f}, {92, 28, 22, 255}, {172, 62, 46, 255},
                        BlendMode::Alpha);
        e.drawGradientH({cx, 176.f, 17.f, 368.f}, {172, 62, 46, 255}, {104, 32, 24, 255},
                        BlendMode::Alpha);
        e.fillRect({cx - 21.f, 176.f, 42.f, 8.f}, {60, 36, 26, 255}, BlendMode::Alpha);
        e.drawGradient({cx - 26.f, 540.f, 52.f, 22.f}, {124, 118, 108, 255}, {80, 76, 70, 255},
                       BlendMode::Alpha);
    }
}

void FxDemoScene::drawLanterns(Engine& e) {
    const bool lit = preset_->lanternsLit;
    const Color gold{176, 138, 70, 255};
    for (const float x : kLanternX) {
        e.drawLine(x, 176.f, x, 214.f, {40, 26, 20, 255});
        mesh_.fan(x, kLanternY, 24.f, 32.f, lit ? Color{250, 132, 84, 255} : Color{170, 54, 40, 255},
                  lit ? Color{178, 50, 36, 255} : Color{104, 30, 24, 255});
    }
    mesh_.draw(e, BlendMode::Alpha);
    for (const float x : kLanternX) {
        for (const float dx : {-12.f, 0.f, 12.f}) {
            const float half = 32.f * std::sqrt(1.f - (dx / 24.f) * (dx / 24.f));
            e.drawLine(x + dx, kLanternY - half, x + dx, kLanternY + half, {120, 34, 26, 150});
        }
        e.fillRect({x - 13.f, 214.f, 26.f, 6.f}, gold, BlendMode::Alpha);
        e.fillRect({x - 11.f, 283.f, 22.f, 6.f}, gold, BlendMode::Alpha);
        e.drawLine(x, 289.f, x, 310.f, {200, 60, 40, 255});
        e.fillRect({x - 2.f, 306.f, 4.f, 10.f}, {190, 52, 36, 255}, BlendMode::Alpha);
    }
}

void FxDemoScene::drawBrazier(Engine& e) {
    const Color iron{48, 44, 48, 255};
    for (const float lx : {kBrazierX - 20.f, kBrazierX - 2.f, kBrazierX + 16.f}) {
        e.fillRect({lx, kBrazierTop + 16.f, 4.f, 30.f}, iron, BlendMode::Alpha);
    }
    mesh_.polygon4(vtx(kBrazierX - 30.f, kBrazierTop, {84, 78, 80, 255}),
                   vtx(kBrazierX + 30.f, kBrazierTop, {84, 78, 80, 255}),
                   vtx(kBrazierX + 20.f, kBrazierTop + 20.f, {40, 36, 40, 255}),
                   vtx(kBrazierX - 20.f, kBrazierTop + 20.f, {40, 36, 40, 255}));
    mesh_.draw(e, BlendMode::Alpha);
    e.fillRect({kBrazierX - 34.f, kBrazierTop - 3.f, 68.f, 5.f}, {110, 102, 100, 255},
               BlendMode::Alpha);

    // 火舌：三角形，底部实色、尖端全透明；尖端随时间左右晃、上下窜。
    for (int i = 0; i < 5; ++i) {
        const float fi = static_cast<float>(i);
        const float bx = kBrazierX - 22.f + fi * 11.f;
        const float tipX = bx + 4.f * std::sin(time_ * 5.f + fi * 1.7f);
        const float tipY = kBrazierTop - 26.f - 14.f * std::fabs(std::sin(time_ * 3.1f + fi));
        mesh_.triangle(vtx(bx - 8.f, kBrazierTop, {255, 130, 40, 230}),
                       vtx(bx + 8.f, kBrazierTop, {255, 130, 40, 230}),
                       vtx(tipX, tipY, {255, 220, 120, 0}));
        mesh_.triangle(vtx(bx - 4.f, kBrazierTop, {255, 226, 150, 240}),
                       vtx(bx + 4.f, kBrazierTop, {255, 226, 150, 240}),
                       vtx(tipX, tipY + 12.f, {255, 250, 220, 0}));
    }
    mesh_.draw(e, BlendMode::Alpha);
}

void FxDemoScene::drawFigures(Engine& e) {
    // 脚下柔影：中心半透明黑、边缘透明的扁椭圆。
    for (const float x : kFigureX) {
        mesh_.fan(x, kFeetY - 2.f, 22.f, 7.f, {0, 0, 0, 120}, {0, 0, 0, 0});
    }
    mesh_.draw(e, BlendMode::Alpha);

    // 16×24 像素图按 ×3 画；第二个人水平翻转，两人相对而立。
    for (std::size_t i = 0; i < kFigureX.size(); ++i) {
        engine::DrawOptions options;
        options.flipX = i == 1;
        e.drawTexture(figures_[i].get(), RectF{},
                      RectF{kFigureX[i] - 24.f, kFeetY - 72.f, 48.f, 72.f}, options);
    }
}

void FxDemoScene::drawShafts(Engine& e) {
    // 光柱（体积光）：斜着的长条，顶端淡淡一层、到地面淡到零，Add 叠上去。
    if (preset_->moon) {
        for (const float x0 : {960.f, 1070.f, 1170.f}) {
            mesh_.polygon4(vtx(x0, 0.f, {120, 140, 210, 70}), vtx(x0 + 70.f, 0.f, {120, 140, 210, 70}),
                           vtx(x0 - 110.f, kH, {120, 140, 210, 0}),
                           vtx(x0 - 260.f, kH, {120, 140, 210, 0}));
        }
    }
    if (preset_->sunShafts) {
        for (const float x0 : {60.f, 230.f, 420.f}) {
            mesh_.polygon4(vtx(x0, 0.f, {255, 238, 196, 64}), vtx(x0 + 90.f, 0.f, {255, 238, 196, 64}),
                           vtx(x0 + 480.f, kH, {255, 238, 196, 0}),
                           vtx(x0 + 300.f, kH, {255, 238, 196, 0}));
        }
    }
    mesh_.draw(e, BlendMode::Add);
}

void FxDemoScene::drawEmissive(Engine& e) {
    // 辉光通道里只画「会发光的那一块」，而且画得比场景里更亮更饱满：
    // 它经过降采样与放大之后会化成光晕加回去，这里画的就是光晕的形状与颜色。
    if (preset_->moon) {
        mesh_.fan(kMoonX, kMoonY, 34.f, 34.f, {255, 252, 236, 255}, {240, 236, 210, 255});
    }
    if (preset_->lanternsLit) {
        // 纸灯笼透出来的是橙红的光：光晕不要白芯，否则辉光一加灯笼就烧成一团白。
        for (const float x : kLanternX) {
            mesh_.fan(x, kLanternY, 42.f, 52.f, {240, 124, 60, 255}, {210, 70, 30, 0});
        }
    }
    mesh_.fan(kBrazierX, kBrazierTop - 12.f, 40.f, 34.f, {255, 190, 90, 255}, {255, 90, 20, 0});
    mesh_.draw(e, BlendMode::Alpha);
    for (Layer& layer : layers_) {
        if (layer.emissive) {
            layer.system.render(e, kCameraX, kCameraY);
        }
    }
}

void FxDemoScene::addLights() {
    if (preset_->lanternsLit) {
        for (const float x : kLanternX) {
            fx_->addLight({x, kLanternY, 250.f, {255, 170, 96, 255}, 1.2f});
        }
    }
    fx_->addLight({kBrazierX, kBrazierTop - 6.f, 300.f, {255, 136, 56, 255}, 1.3f});
    if (preset_->moon) {
        fx_->addLight({kMoonX, kMoonY, 700.f, {80, 90, 130, 255}, 1.f});
        // 主角身上一团微光（施工图第 3 节）：夜里人物不至于沉进黑里。
        fx_->addLight({kFigureX[0], kFeetY - 40.f, 120.f, {120, 130, 150, 255}, 0.6f});
    }
}

void FxDemoScene::drawOverlay(Engine& e) {
    // 墨金面板：竖向渐变底 + 1px 金线外框 + 内缩 3px 的暗金线 + 四角小菱形。
    const RectF panel{24.f, 24.f, 470.f, 152.f};
    e.drawGradient(panel, kInk, kInkLow, BlendMode::Alpha);
    frame(e, panel, kGold);
    frame(e, {panel.x + 3.f, panel.y + 3.f, panel.w - 6.f, panel.h - 6.f}, kGoldDim);
    const auto diamond = [&](float cx, float cy, float r, const Color& c) {
        mesh_.polygon4(vtx(cx, cy - r, c), vtx(cx + r, cy, c), vtx(cx, cy + r, c), vtx(cx - r, cy, c));
    };
    diamond(panel.x, panel.y, 4.f, kGold);
    diamond(panel.x + panel.w, panel.y, 4.f, kGold);
    diamond(panel.x + panel.w, panel.y + panel.h, 4.f, kGold);
    diamond(panel.x, panel.y + panel.h, 4.f, kGold);
    diamond(40.f, 115.f, 5.f, kGoldBright);   // 选中行的指针
    mesh_.draw(e, BlendMode::Alpha);

    // 选中行的渐隐金条。故意画得比面板宽，由裁剪收在面板里——演示 setClipRect。
    const engine::Rect inside{static_cast<int>(panel.x) + 4, static_cast<int>(panel.y) + 4,
                              static_cast<int>(panel.w) - 8, static_cast<int>(panel.h) - 8};
    e.setClipRect(&inside);
    e.drawGradientH({28.f, 101.f, 560.f, 28.f}, {201, 164, 92, 110}, {201, 164, 92, 0},
                    BlendMode::Alpha);
    e.setClipRect(nullptr);

    TextStyle title;
    title.outline = true;
    title.outlineWidth = 2;
    title.outlineColor = kInkDeep;
    title.shadow = true;
    e.drawText(preset_->title, 44, 34, 26, kGoldBright, title);
    TextStyle body;
    body.shadow = true;
    e.drawText(preset_->lighting, 52, 76, 19, kPaper, body);
    e.drawText(preset_->effects, 52, 103, 19, kGoldBright, body);
    e.drawText(preset_->particles, 52, 130, 19, kPaper, body);

    const std::string caption = "UI 画在 endScene 之后：不糊、不压暗、不被调色";
    const engine::Point size = e.measureText(caption, 18);
    e.drawText(caption, engine::kLogicalWidth - 28 - size.x, engine::kLogicalHeight - 24 - size.y,
               18, kPaperDim, body);

    // 粒子用的是哪一种画法：截图上一眼看得出来，不必再去翻日志。
    const auto withArt = std::count_if(layers_.begin(), layers_.end(),
                                       [](const Layer& layer) { return layer.system.usesArt(); });
    const std::string source =
        withArt == 0 ? "粒子贴图：程序生成（assets/art/fx 里没有对应的图）"
        : withArt == static_cast<std::ptrdiff_t>(layers_.size())
            ? "粒子贴图：美术产物（assets/art/fx）"
            : "粒子贴图：部分美术产物、部分程序生成";
    const engine::Point sourceSize = e.measureText(source, 18);
    e.drawText(source, engine::kLogicalWidth - 28 - sourceSize.x,
               engine::kLogicalHeight - 24 - size.y - 6 - sourceSize.y, 18, kPaperDim, body);
}

}  // namespace fanren::game
