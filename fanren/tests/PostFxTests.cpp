// engine::PostFx 的单测。
//
// 三层：
//   1. 纯函数（景深带的 alpha 曲线、光源衰减、暗角、降采样尺寸、生成的两张图）——
//      不开窗口；每条都**扫一片**而不是钉一个点（docs/README.md「只钉一个数据点」那一条）。
//   2. 无头契约：整条管线是空操作，不切目标、不建纹理。
//   3. 软件渲染器上的像素：景深真的上下糊、中间清；光照真的压暗又被光源提亮；辉光真的
//      往外渗；暗角只压四角；调色先乘后加；UI 画在 endScene 之后不受任何影响。
//      验法与截图验收同一条路（captureFrame → PNG → 读回）。
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include "TempDir.h"
#include "engine/Engine.h"
#include "engine/PostFx.h"

namespace {

namespace fs = std::filesystem;

using fanren::engine::BlendMode;
using fanren::engine::Color;
using fanren::engine::DofRow;
using fanren::engine::DrawOptions;
using fanren::engine::Engine;
using fanren::engine::kInvalidTexture;
using fanren::engine::kLogicalHeight;
using fanren::engine::kLogicalWidth;
using fanren::engine::Light;
using fanren::engine::Point;
using fanren::engine::PostFx;
using fanren::engine::PostFxSettings;
using fanren::engine::RectF;
using fanren::engine::ScaleMode;
using fanren::engine::TextureId;

constexpr Color kWhite{255, 255, 255, 255};
constexpr Color kBlack{0, 0, 0, 255};

// 几组有代表性的景深参数：缺省、窄带、宽带、偏上、偏下、过渡很窄、两条过渡带挨着。
struct DofCase {
    float focus;
    float band;
    float ramp;
};
constexpr DofCase kDofCases[] = {
    {0.52f, 0.34f, 0.15f}, {0.5f, 0.1f, 0.1f},  {0.6f, 0.26f, 0.18f}, {0.3f, 0.2f, 0.12f},
    {0.75f, 0.15f, 0.2f},  {0.5f, 0.3f, 0.01f}, {0.5f, 0.05f, 0.1f},
};

float alphaAt(float y, const DofCase& c) {
    return fanren::engine::dofBlurAlpha(y, c.focus, c.band, c.ramp);
}

// 相邻两行之间线性插值：景深带的四边形就是这么把曲线画出来的。
float interpolateRows(const std::vector<DofRow>& rows, float y) {
    for (std::size_t i = 0; i + 1 < rows.size(); ++i) {
        if (y >= rows[i].y && y <= rows[i + 1].y) {
            const float span = rows[i + 1].y - rows[i].y;
            const float t = span > 0.f ? (y - rows[i].y) / span : 0.f;
            return rows[i].alpha + (rows[i + 1].alpha - rows[i].alpha) * t;
        }
    }
    return -1.f;   // 不在任何一段里：行没盖住 [0,1]
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. 纯函数
// ---------------------------------------------------------------------------

TEST(PostFxCurves, DofIsClearInsideTheBandBlurredOutsideAndHalfwayAtTheEdge) {
    for (const DofCase& c : kDofCases) {
        const float r = std::max(c.ramp, 1.f / 720.f);
        for (int i = 0; i <= 2000; ++i) {
            const float y = static_cast<float>(i) / 2000.f;
            const float d = std::fabs(y - c.focus);
            const float a = alphaAt(y, c);
            ASSERT_TRUE(std::isfinite(a)) << "y=" << y;
            ASSERT_GE(a, 0.f);
            ASSERT_LE(a, 1.f);
            if (d <= c.band - r * 0.5f - 1e-4f) {
                EXPECT_EQ(a, 0.f) << "带内应当全清：y=" << y << " focus=" << c.focus;
            }
            if (d >= c.band + r * 0.5f + 1e-4f) {
                EXPECT_EQ(a, 1.f) << "带外应当全糊：y=" << y << " focus=" << c.focus;
            }
        }
        // 清晰带的边（focus ± band）恰是过渡带正中：半糊。
        EXPECT_NEAR(alphaAt(c.focus - c.band, c), 0.5f, 1e-4f);
        EXPECT_NEAR(alphaAt(c.focus + c.band, c), 0.5f, 1e-4f);
        EXPECT_EQ(alphaAt(c.focus, c), c.band > r * 0.5f ? 0.f : alphaAt(c.focus, c));
    }
}

TEST(PostFxCurves, DofIsSymmetricAboutTheFocusAndNeverDecreasesAwayFromIt) {
    for (const DofCase& c : kDofCases) {
        float previous = 0.f;
        for (int i = 0; i <= 1000; ++i) {
            const float d = static_cast<float>(i) / 1000.f;
            const float above = fanren::engine::dofBlurAlpha(c.focus - d, c.focus, c.band, c.ramp);
            const float below = fanren::engine::dofBlurAlpha(c.focus + d, c.focus, c.band, c.ramp);
            EXPECT_NEAR(above, below, 1e-5f) << "上下不对称：d=" << d;
            EXPECT_GE(below + 1e-6f, previous) << "离焦点越远不许越清：d=" << d;
            previous = below;
        }
    }
}

TEST(PostFxCurves, DofRampStartsAndEndsFlat) {
    // smoothstep：过渡带两端的斜率为零，清晰带与全糊带接上去看不出折痕。
    const DofCase c{0.5f, 0.25f, 0.2f};
    const float start = c.focus + c.band - c.ramp * 0.5f;
    const float end = c.focus + c.band + c.ramp * 0.5f;
    const float step = c.ramp * 0.01f;
    EXPECT_LT(alphaAt(start + step, c), 0.001f);
    EXPECT_GT(alphaAt(end - step, c), 0.999f);
    // 中段是陡的：同样一步，中间变化远大于两端。
    const float mid = c.focus + c.band;
    EXPECT_GT(alphaAt(mid + step, c) - alphaAt(mid, c), 10.f * alphaAt(start + step, c));
}

TEST(PostFxCurves, DofWithAZeroRampIsAHardEdgeNotANaN) {
    const DofCase c{0.5f, 0.2f, 0.f};
    EXPECT_EQ(alphaAt(0.5f, c), 0.f);
    EXPECT_EQ(alphaAt(0.25f, c), 1.f);
    EXPECT_EQ(alphaAt(0.75f, c), 1.f);
    EXPECT_TRUE(std::isfinite(alphaAt(0.3f, c)));
}

TEST(PostFxCurves, DofRowsCoverTheScreenAndReproduceTheCurve) {
    std::vector<DofRow> rows;
    for (const DofCase& c : kDofCases) {
        fanren::engine::dofBandRows(c.focus, c.band, c.ramp, rows);
        ASSERT_GE(rows.size(), 2u);
        EXPECT_EQ(rows.front().y, 0.f);
        EXPECT_EQ(rows.back().y, 1.f);
        for (std::size_t i = 0; i + 1 < rows.size(); ++i) {
            EXPECT_LT(rows[i].y, rows[i + 1].y) << "行必须严格从上往下排";
        }
        // 真正要紧的性质：按行线性插值画出来的，与曲线本身差得不多。
        float worst = 0.f;
        for (int i = 0; i <= 4000; ++i) {
            const float y = static_cast<float>(i) / 4000.f;
            const float drawn = interpolateRows(rows, y);
            ASSERT_GE(drawn, 0.f) << "y=" << y << " 没有被任何一段盖住";
            worst = std::max(worst, std::fabs(drawn - alphaAt(y, c)));
        }
        EXPECT_LT(worst, 0.03f) << "focus=" << c.focus << " band=" << c.band << " ramp=" << c.ramp;
        // 也不白切：只在两条过渡带里细分，平的地方不多给行。
        EXPECT_LE(rows.size(), 2u + 2u * (fanren::engine::kDofRampSteps + 1u));
    }
}

TEST(PostFxCurves, LightFalloffIsFullAtTheCentreZeroAtTheRimAndMonotone) {
    EXPECT_EQ(fanren::engine::lightFalloff(0.f), 1.f);
    EXPECT_EQ(fanren::engine::lightFalloff(1.f), 0.f);
    EXPECT_EQ(fanren::engine::lightFalloff(1.5f), 0.f);
    float previous = 1.f;
    for (int i = 1; i <= 1000; ++i) {
        const float v = fanren::engine::lightFalloff(static_cast<float>(i) / 1000.f);
        EXPECT_LE(v, previous);
        previous = v;
    }
    // 外沿收得平：最后百分之一的半径里亮度已经几乎为零，看不出一道环。
    EXPECT_LT(fanren::engine::lightFalloff(0.99f), 0.001f);
    EXPECT_NEAR(fanren::engine::lightFalloff(0.5f), 0.5625f, 1e-6f);
}

TEST(PostFxCurves, VignetteLeavesTheMiddleAloneAndDarkensTowardTheCorners) {
    EXPECT_EQ(fanren::engine::vignetteAlpha(0.f, 0.f), 0.f);
    EXPECT_NEAR(fanren::engine::vignetteAlpha(1.f, 1.f), 1.f, 1e-5f);
    // 中间一大片完全不压。
    for (int i = 0; i <= 20; ++i) {
        const float t = static_cast<float>(i) / 20.f * 0.5f;
        EXPECT_EQ(fanren::engine::vignetteAlpha(t, t * 0.5f), 0.f) << "t=" << t;
    }
    // 沿对角线越往外越暗；四个象限、横竖两个方向对称。
    float previous = 0.f;
    for (int i = 0; i <= 100; ++i) {
        const float t = static_cast<float>(i) / 100.f;
        const float v = fanren::engine::vignetteAlpha(t, t);
        EXPECT_GE(v + 1e-6f, previous);
        previous = v;
        EXPECT_FLOAT_EQ(v, fanren::engine::vignetteAlpha(-t, t));
        EXPECT_FLOAT_EQ(v, fanren::engine::vignetteAlpha(-t, -t));
        EXPECT_FLOAT_EQ(fanren::engine::vignetteAlpha(t, 0.3f), fanren::engine::vignetteAlpha(0.3f, t));
    }
    // 边的正中压得比角轻。
    EXPECT_LT(fanren::engine::vignetteAlpha(1.f, 0.f), fanren::engine::vignetteAlpha(1.f, 1.f));
    EXPECT_GT(fanren::engine::vignetteAlpha(1.f, 0.f), 0.f);
}

TEST(PostFxCurves, DownsampleHalvesPerLevelAndNeverVanishes) {
    const Point full{1280, 720};
    const Point expected[] = {{1280, 720}, {640, 360}, {320, 180}, {160, 90}};
    for (int level = 0; level < 4; ++level) {
        const Point got = fanren::engine::downsampleSize(full, level);
        EXPECT_EQ(got.x, expected[level].x);
        EXPECT_EQ(got.y, expected[level].y);
    }
    const Point tiny = fanren::engine::downsampleSize(Point{3, 2}, 5);
    EXPECT_EQ(tiny.x, 1);
    EXPECT_EQ(tiny.y, 1);
}

TEST(PostFxCurves, GeneratedLightAndVignetteImagesHaveTheRightShape) {
    const int n = 64;
    const std::vector<std::uint32_t> light = fanren::engine::radialLightPixels(n);
    ASSERT_EQ(light.size(), static_cast<std::size_t>(n * n));
    const auto alpha = [](std::uint32_t p) { return static_cast<int>(p & 0xFFu); };
    const auto rgb = [](std::uint32_t p) { return p >> 8; };
    EXPECT_GE(alpha(light[static_cast<std::size_t>(n / 2 * n + n / 2)]), 250);
    EXPECT_EQ(alpha(light[0]), 0);
    EXPECT_EQ(alpha(light[static_cast<std::size_t>(n * n - 1)]), 0);
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            const std::uint32_t p = light[static_cast<std::size_t>(y * n + x)];
            EXPECT_EQ(rgb(p), 0xFFFFFFu);   // 白光，颜色靠绘制时上
            EXPECT_EQ(p, light[static_cast<std::size_t>(y * n + (n - 1 - x))]);   // 左右对称
            EXPECT_EQ(p, light[static_cast<std::size_t>((n - 1 - y) * n + x)]);   // 上下对称
        }
    }

    const int w = 320;
    const int h = 180;
    const std::vector<std::uint32_t> vignette = fanren::engine::vignettePixels(w, h);
    ASSERT_EQ(vignette.size(), static_cast<std::size_t>(w * h));
    EXPECT_EQ(alpha(vignette[static_cast<std::size_t>(h / 2 * w + w / 2)]), 0);
    EXPECT_GE(alpha(vignette[0]), 240);
    EXPECT_GE(alpha(vignette[static_cast<std::size_t>(w * h - 1)]), 240);
    for (const std::uint32_t p : vignette) {
        EXPECT_EQ(rgb(p), 0u);   // 黑色
    }
}

// ---------------------------------------------------------------------------
// 2. 无头契约
// ---------------------------------------------------------------------------

TEST(PostFxHeadless, TheWholePipelineIsANoOpAndNeverSwitchesTargets) {
    Engine engine;
    ASSERT_TRUE(engine.init("fanren-tests", true).ok);
    {
        PostFx fx(engine);
        PostFxSettings everything;
        everything.ambient = Color{40, 40, 80, 255};
        everything.dof = 1.f;
        everything.bloom = 1.f;
        everything.vignette = 1.f;
        everything.gradeMul = Color{200, 200, 255, 255};
        everything.gradeAdd = Color{10, 0, 0, 255};
        for (int frame = 0; frame < 3; ++frame) {
            engine.beginFrame();
            fx.beginScene();
            EXPECT_EQ(engine.renderTarget(), kInvalidTexture);
            EXPECT_FALSE(fx.beginEmissive());
            fx.endEmissive();
            fx.addLight(Light{100.f, 100.f, 50.f, kWhite, 1.f});
            fx.endScene(everything);
            EXPECT_EQ(fx.snapshot(), kInvalidTexture);
            EXPECT_EQ(fx.snapshotTexture(), kInvalidTexture);
            EXPECT_EQ(fx.blurredSnapshot(), kInvalidTexture);
            engine.endFrame();
        }
        EXPECT_FALSE(fx.active());
        EXPECT_EQ(engine.renderTarget(), kInvalidTexture);
    }
    engine.shutdown();
}

// ---------------------------------------------------------------------------
// 3. 软件渲染器上的像素
// ---------------------------------------------------------------------------

namespace {

struct Frame {
    int w = 0;
    int h = 0;
    std::vector<Color> pixels;
    [[nodiscard]] Color at(int x, int y) const {
        return pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                      static_cast<std::size_t>(x)];
    }
};

// 与 EngineTests 里那台同一个配法（dummy 视频驱动 + 软件渲染器），各测试文件各留一份，
// 免得为两个用例开一个共享头。
class SoftwarePostFx : public ::testing::Test {
protected:
    void SetUp() override {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        const auto result = engine.init("fanren-tests", false);
        ASSERT_TRUE(result.ok) << result.error;
        engine.beginFrame();
        fx = std::make_unique<PostFx>(engine);
    }
    void TearDown() override {
        fx.reset();
        engine.shutdown();
        SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
    }

    Frame grab() {
        Frame frame;
        const fs::path png = dir.path() / ("frame" + std::to_string(shots++) + ".png");
        const auto shot = engine.captureFrame(png.string());
        EXPECT_TRUE(shot.ok) << shot.error;
        if (!shot.ok) return frame;
        SDL_Surface* loaded = SDL_LoadPNG(png.string().c_str());
        EXPECT_NE(loaded, nullptr) << SDL_GetError();
        if (loaded == nullptr) return frame;
        SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(loaded);
        if (rgba == nullptr) return frame;
        frame.w = rgba->w;
        frame.h = rgba->h;
        for (int y = 0; y < frame.h; ++y) {
            const auto* row = static_cast<const std::uint8_t*>(rgba->pixels) + y * rgba->pitch;
            for (int x = 0; x < frame.w; ++x) {
                frame.pixels.push_back(Color{row[x * 4], row[x * 4 + 1], row[x * 4 + 2], row[x * 4 + 3]});
            }
        }
        SDL_DestroySurface(rgba);
        return frame;
    }

    // 一整屏 1 像素宽的黑白竖条：清晰处只有 0 与 255，糊了就往 128 靠。
    void drawStripes() {
        for (int x = 0; x < kLogicalWidth; x += 2) {
            engine.fillRect({static_cast<float>(x), 0.f, 1.f, static_cast<float>(kLogicalHeight)},
                            kWhite, BlendMode::None);
        }
    }

    Engine engine;
    std::unique_ptr<PostFx> fx;
    fanren::test::TempDir dir{"fanren_postfx_sw"};
    int shots = 0;
};

// 一行里的像素离 128 有多远（平均）：竖条清晰时接近 127，糊透了接近 0。
double stripeContrast(const Frame& f, int y) {
    double sum = 0.0;
    int n = 0;
    for (int x = 100; x < 1180; ++x) {
        sum += std::abs(static_cast<int>(f.at(x, y).r) - 128);
        ++n;
    }
    return sum / n;
}

}  // namespace

TEST_F(SoftwarePostFx, DefaultSettingsReproduceTheSceneExactly) {
    // 参照：直接画在后缓冲上。
    const auto paint = [this] {
        engine.fillRect({100.f, 100.f, 300.f, 200.f}, Color{200, 60, 40, 255}, BlendMode::Alpha);
        engine.fillRect({500.f, 300.f, 200.f, 300.f}, Color{30, 160, 220, 255}, BlendMode::Alpha);
        engine.drawGradient({800.f, 0.f, 400.f, 720.f}, Color{0, 0, 0, 255}, kWhite, BlendMode::Alpha);
        drawStripes();
    };
    paint();
    const Frame direct = grab();

    engine.clear(kBlack);
    fx->beginScene();
    ASSERT_TRUE(fx->active());
    paint();
    fx->endScene(PostFxSettings{});
    const Frame piped = grab();

    ASSERT_EQ(direct.w, piped.w);
    int differing = 0;
    for (std::size_t i = 0; i < direct.pixels.size(); ++i) {
        const Color& a = direct.pixels[i];
        const Color& b = piped.pixels[i];
        differing += (a.r != b.r || a.g != b.g || a.b != b.b) ? 1 : 0;
    }
    EXPECT_EQ(differing, 0) << "缺省参数的后处理必须是恒等变换";
}

TEST_F(SoftwarePostFx, DepthOfFieldBlursTopAndBottomAndKeepsTheFocusSharp) {
    PostFxSettings s;
    s.dof = 1.f;
    s.dofFocus = 0.5f;
    s.dofBand = 0.25f;
    s.dofRamp = 0.1f;
    fx->beginScene();
    drawStripes();
    fx->endScene(s);
    const Frame f = grab();

    // 先验：焦点处仍是纯黑纯白的竖条（对比度满格）。
    EXPECT_GT(stripeContrast(f, 360), 120.0);
    // 上下两端（|y-0.5| ≥ band + ramp/2 = 0.3，即 y ≤ 144、y ≥ 576）糊透了。
    EXPECT_LT(stripeContrast(f, 20), 25.0);
    EXPECT_LT(stripeContrast(f, 100), 25.0);
    EXPECT_LT(stripeContrast(f, 620), 25.0);
    EXPECT_LT(stripeContrast(f, 700), 25.0);
    // 带内（|y-0.5| ≤ band - ramp/2 = 0.2，即 216..504）一点不糊。
    EXPECT_GT(stripeContrast(f, 230), 120.0);
    EXPECT_GT(stripeContrast(f, 490), 120.0);
    // 过渡带正中（y = 0.25、0.75）半糊，上下两条对称。
    const double top = stripeContrast(f, 180);
    const double bottom = stripeContrast(f, 540);
    EXPECT_GT(top, 30.0);
    EXPECT_LT(top, 100.0);
    EXPECT_NEAR(top, bottom, 12.0);
}

TEST_F(SoftwarePostFx, ZeroDofLeavesEveryRowSharp) {
    PostFxSettings s;
    s.dof = 0.f;
    fx->beginScene();
    drawStripes();
    fx->endScene(s);
    const Frame f = grab();
    for (const int y : {5, 100, 360, 620, 715}) {
        EXPECT_GT(stripeContrast(f, y), 120.0) << "y=" << y;
    }
}

TEST_F(SoftwarePostFx, AmbientDarkensAndALightBringsItsNeighbourhoodBack) {
    PostFxSettings s;
    s.ambient = Color{64, 64, 64, 255};
    fx->beginScene();
    engine.clear(kWhite);
    fx->addLight(Light{640.f, 360.f, 200.f, kWhite, 1.f});
    fx->endScene(s);
    const Frame f = grab();

    EXPECT_GE(f.at(640, 360).r, 245);                       // 光源正中：提回全亮
    EXPECT_NEAR(f.at(20, 20).r, 64, 3);                      // 光照不到的角落：环境光
    EXPECT_NEAR(f.at(1260, 700).r, 64, 3);
    const int halfway = f.at(740, 360).r;                    // 半径一半处：介于两者之间
    EXPECT_GT(halfway, 100);
    EXPECT_LT(halfway, 245);
    EXPECT_GT(f.at(700, 360).r, f.at(780, 360).r);           // 离得越远越暗
}

TEST_F(SoftwarePostFx, LightsOnlyMatterWhenTheAmbientIsNotWhite) {
    // 环境光纯白：光照是恒等的，场景原样（这条是 applyLighting 早退的依据）。
    fx->beginScene();
    engine.clear(Color{120, 120, 120, 255});
    fx->addLight(Light{640.f, 360.f, 300.f, kWhite, 4.f});
    fx->endScene(PostFxSettings{});
    const Frame f = grab();
    EXPECT_EQ(f.at(640, 360).r, 120);
    EXPECT_EQ(f.at(10, 10).r, 120);
}

TEST_F(SoftwarePostFx, BloomSpreadsLightAroundEmissiveShapesButOnlyInThatFrame) {
    PostFxSettings s;
    s.bloom = 1.f;
    fx->beginScene();
    ASSERT_TRUE(fx->beginEmissive());
    engine.fillRect({620.f, 340.f, 40.f, 40.f}, kWhite, BlendMode::Alpha);
    fx->endEmissive();
    fx->endScene(s);
    const Frame lit = grab();
    // 场景本身全黑：出现亮光全是辉光的功劳。发光体上最亮，光晕至少铺出发光体半个
    // 身位（40 像素见方的块，外沿再往外 20 像素还看得见），远处一点不亮。
    EXPECT_GT(lit.at(640, 360).r, 150);
    EXPECT_GT(lit.at(680, 360).r, 8);
    EXPECT_GT(lit.at(640, 400).r, 8);
    EXPECT_GT(lit.at(660, 360).r, lit.at(680, 360).r);   // 由内往外变淡
    EXPECT_EQ(lit.at(100, 100).r, 0);

    // 下一帧没画发光体：不许把上一帧的辉光再加一遍。
    engine.endFrame();
    engine.beginFrame();
    fx->beginScene();
    fx->endScene(s);
    const Frame next = grab();
    EXPECT_EQ(next.at(640, 360).r, 0);
    EXPECT_EQ(next.at(680, 360).r, 0);
}

// 系统设置的「画面缩放：铺满」（docs/settings.md 4.2）：窗口不是 1280×720 的整数倍时，主菜单的模糊底图
// （PostFx::snapshot → blurredSnapshot，整屏铺回去）要落回原处，不许错位。截图验收那台机器是 4K 屏，
// 全屏铺满恰好 3 倍、试不出非整数倍，所以这里用 dummy 驱动把窗口改成 1500×1000 实测：
// 铺满 = 按宽放大 1.171875 倍，内容区 1500×843.75，上下各留约 78 像素黑边。
TEST_F(SoftwarePostFx, TheMenuBackdropStaysPutWhenFitScalesByANonIntegerFactor) {
    int count = 0;
    SDL_Window** windows = SDL_GetWindows(&count);
    ASSERT_NE(windows, nullptr) << SDL_GetError();
    SDL_Window* window = count == 1 ? windows[0] : nullptr;
    SDL_free(windows);
    ASSERT_NE(window, nullptr) << "先验：引擎开了恰好一个窗口";

    engine.endFrame();
    ASSERT_TRUE(SDL_SetWindowSize(window, 1500, 1000)) << SDL_GetError();
    engine.setIntegerScale(false);
    engine.pollEvents();   // 让渲染器收到窗口尺寸的变化
    engine.beginFrame();

    // 左半红、右半蓝（逻辑坐标），拍快照——主菜单打开那一帧做的就是这件事。
    constexpr Color kRed{220, 30, 30, 255};
    constexpr Color kBlue{30, 30, 220, 255};
    engine.fillRect({0.f, 0.f, 640.f, 720.f}, kRed, BlendMode::None);
    engine.fillRect({640.f, 0.f, 640.f, 720.f}, kBlue, BlendMode::None);
    ASSERT_NE(fx->snapshot(), kInvalidTexture);
    const Point shot = engine.textureSize(fx->snapshotTexture());
    EXPECT_EQ(shot.x, 1500) << "快照是内容区，不是整个窗口";
    EXPECT_NEAR(shot.y, 844, 1);

    // 模糊底图整屏铺回去（MenuScene::drawBackdrop 的画法），红蓝分界应仍在逻辑 x = 640。
    // 抓帧（SDL_RenderReadPixels 读整个视口）拿到的正是内容区：1500×843，不含上下黑边。
    engine.clear(kBlack);
    engine.drawTexture(fx->blurredSnapshot(), RectF{}, RectF{0.f, 0.f, 1280.f, 720.f}, DrawOptions{});
    const Frame f = grab();
    ASSERT_EQ(f.w, 1500);
    ASSERT_NEAR(f.h, 844, 1);
    constexpr double kScale = 1500.0 / 1280.0;
    const auto at = [&](double lx, double ly) {
        return f.at(static_cast<int>(lx * kScale), static_cast<int>(ly * kScale));
    };
    EXPECT_GT(at(320, 360).r, 180) << "左半还是红的";
    EXPECT_GT(at(960, 360).b, 180) << "右半还是蓝的";
    // 贴着上下沿也是底图（没有竖向错位把黑边带进来，也没有被挤出去一截）。
    EXPECT_GT(at(320, 1).r, 150) << "上沿";
    EXPECT_GT(at(960, 1).b, 150) << "上沿";
    EXPECT_GT(at(320, 718).r, 150) << "下沿";
    EXPECT_GT(at(960, 718).b, 150) << "下沿";
    // 分界：沿中线找红开始不占上风的那一列，换回逻辑坐标应在 640 附近（模糊有宽度，留几个像素的余地）。
    const int y = static_cast<int>(360.0 * kScale);
    int cross = -1;
    for (int x = 1; x < f.w; ++x) {
        if (f.at(x - 1, y).r > f.at(x - 1, y).b && f.at(x, y).r <= f.at(x, y).b) {
            cross = x;
            break;
        }
    }
    EXPECT_NEAR(cross / kScale, 640.0, 6.0) << "红蓝分界跑到了窗口 x = " << cross;
}

// 系统设置的「画面特效：精简」（docs/settings.md 4.3）：档位放在 Engine 上，PostFx 单点读——
// 没有辉光通道（beginEmissive 答 false）、endScene 跳过景深；光照照做。
TEST_F(SoftwarePostFx, LiteEffectsDropBloomAndDepthOfFieldButKeepTheLighting) {
    PostFxSettings s;
    s.dof = 1.f;
    s.dofFocus = 0.5f;
    s.dofBand = 0.25f;
    s.dofRamp = 0.1f;
    s.bloom = 1.f;

    engine.setEffectsLevel(fanren::engine::EffectsLevel::Lite);
    fx->beginScene();
    EXPECT_FALSE(fx->beginEmissive()) << "精简档没有辉光通道：调用方照「别画发光体」处理";
    drawStripes();
    fx->endScene(s);
    const Frame lite = grab();
    for (const int y : {20, 100, 620, 700}) {
        EXPECT_GT(stripeContrast(lite, y), 120.0) << "精简档不做景深：y=" << y;
    }

    // 光照、暗角这些便宜的照做：夜景的昼夜感靠它。
    engine.endFrame();
    engine.beginFrame();
    PostFxSettings night;
    night.ambient = Color{64, 64, 64, 255};
    fx->beginScene();
    engine.clear(kWhite);
    fx->endScene(night);
    EXPECT_NEAR(grab().at(20, 20).r, 64, 3) << "精简档照样压暗";

    // [配对的负向] 同一组参数回到完整档：上下两端又糊了，辉光通道也回来了。
    engine.endFrame();
    engine.beginFrame();
    engine.setEffectsLevel(fanren::engine::EffectsLevel::Full);
    fx->beginScene();
    EXPECT_TRUE(fx->beginEmissive());
    fx->endEmissive();
    drawStripes();
    fx->endScene(s);
    const Frame full = grab();
    EXPECT_LT(stripeContrast(full, 20), 25.0);
    EXPECT_LT(stripeContrast(full, 700), 25.0);
}

TEST_F(SoftwarePostFx, VignetteDarkensTheCornersAndSparesTheMiddle) {
    PostFxSettings s;
    s.vignette = 1.f;
    fx->beginScene();
    engine.clear(kWhite);
    fx->endScene(s);
    const Frame f = grab();
    EXPECT_GE(f.at(640, 360).r, 252);
    EXPECT_GE(f.at(640, 200).r, 252);
    EXPECT_LE(f.at(2, 2).r, 40);
    EXPECT_LE(f.at(1277, 717).r, 40);
    const int edge = f.at(2, 360).r;       // 左边正中：压了，但比角轻
    EXPECT_LT(edge, 230);
    EXPECT_GT(edge, f.at(2, 2).r + 40);
}

TEST_F(SoftwarePostFx, GradeMultipliesThenAdds) {
    PostFxSettings s;
    s.gradeMul = Color{255, 128, 0, 255};
    s.gradeAdd = Color{0, 0, 50, 255};
    fx->beginScene();
    engine.clear(Color{128, 128, 128, 255});
    fx->endScene(s);
    const Color c = grab().at(640, 360);
    EXPECT_NEAR(c.r, 128, 2);
    EXPECT_NEAR(c.g, 64, 2);
    EXPECT_NEAR(c.b, 50, 2);
}

TEST_F(SoftwarePostFx, UiDrawnAfterEndSceneIsUntouched) {
    PostFxSettings s;
    s.ambient = Color{40, 40, 40, 255};
    s.vignette = 1.f;
    s.dof = 1.f;
    s.gradeMul = Color{100, 100, 255, 255};
    fx->beginScene();
    engine.clear(kWhite);
    fx->endScene(s);
    EXPECT_EQ(engine.renderTarget(), kInvalidTexture);     // 回到了后缓冲
    engine.fillRect({0.f, 0.f, 40.f, 40.f}, Color{250, 240, 10, 255}, BlendMode::Alpha);
    const Color corner = grab().at(5, 5);                  // 暗角最重、景深全糊、调色压蓝的那个角
    EXPECT_EQ(corner.r, 250);
    EXPECT_EQ(corner.g, 240);
    EXPECT_EQ(corner.b, 10);
}

TEST_F(SoftwarePostFx, EndSceneComposesIntoTheTargetThatWasBoundAtBeginScene) {
    const TextureId canvas = engine.createRenderTarget(kLogicalWidth, kLogicalHeight, ScaleMode::Pixel);
    ASSERT_NE(canvas, kInvalidTexture);
    engine.setRenderTarget(canvas);
    fx->beginScene();
    engine.clear(Color{0, 200, 0, 255});
    fx->endScene(PostFxSettings{});
    EXPECT_EQ(engine.renderTarget(), canvas);

    // 后缓冲上什么都没有；把那张画布贴上来才看得见场景。
    engine.setRenderTarget(kInvalidTexture);
    EXPECT_EQ(grab().at(640, 360).g, 0);
    DrawOptions copy;
    copy.blend = BlendMode::None;
    engine.drawTexture(canvas, RectF{}, RectF{0.f, 0.f, 1280.f, 720.f}, copy);
    EXPECT_EQ(grab().at(640, 360).g, 200);
}

TEST_F(SoftwarePostFx, SnapshotCopiesTheFrameAndPreparesABlurredTwin) {
    engine.fillRect({0.f, 0.f, 640.f, 720.f}, Color{255, 0, 0, 255}, BlendMode::Alpha);
    engine.fillRect({640.f, 0.f, 640.f, 720.f}, Color{0, 0, 255, 255}, BlendMode::Alpha);
    const TextureId shot = fx->snapshot();
    ASSERT_NE(shot, kInvalidTexture);
    EXPECT_EQ(fx->snapshotTexture(), shot);
    const TextureId blurred = fx->blurredSnapshot();
    ASSERT_NE(blurred, kInvalidTexture);
    EXPECT_EQ(engine.textureSize(blurred).x, kLogicalWidth / 4);
    EXPECT_EQ(engine.textureSize(blurred).y, kLogicalHeight / 4);
    EXPECT_EQ(engine.renderTarget(), kInvalidTexture);   // 做完模糊版切回原目标

    engine.clear(kBlack);
    DrawOptions copy;
    copy.blend = BlendMode::None;
    engine.drawTexture(shot, RectF{}, RectF{0.f, 0.f, 640.f, 360.f}, copy);
    engine.drawTexture(blurred, RectF{}, RectF{0.f, 360.f, 1280.f, 360.f}, copy);
    const Frame f = grab();
    EXPECT_EQ(f.at(100, 100).r, 255);                     // 快照：原样（缩到左上四分之一）
    EXPECT_EQ(f.at(500, 100).b, 255);
    const Color seam = f.at(640, 540);                    // 模糊版：红蓝交界处混成紫
    EXPECT_GT(seam.r, 40);
    EXPECT_GT(seam.b, 40);
    EXPECT_GT(f.at(100, 540).r, 200);                     // 离交界远的地方仍是原色

    // 第二次快照顶掉第一次：旧句柄随之失效，不漏显存。
    const TextureId again = fx->snapshot();
    ASSERT_NE(again, kInvalidTexture);
    EXPECT_EQ(engine.textureSize(shot).x, 0);
}
