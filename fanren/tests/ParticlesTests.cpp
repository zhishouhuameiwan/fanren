// engine::ParticleSystem 的单测。
//
// 粒子本身不开窗口就能测（位置、速度、寿命都是 CPU 上的数）；最后一组在软件渲染器上
// 验「每一种都真的在屏幕上画出了东西」——程序贴图是正经的第二种画法，不能只在
// 美术到位的机器上才看得见。
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <type_traits>
#include <vector>

#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include "TempDir.h"
#include "engine/Engine.h"
#include "engine/Particles.h"

// 粒子系统持有自己的程序贴图：只可移动，且移动不抛——放进 std::vector 扩容时才是挪不是拷。
static_assert(!std::is_copy_constructible_v<fanren::engine::ParticleSystem>);
static_assert(std::is_nothrow_move_constructible_v<fanren::engine::ParticleSystem>);
static_assert(std::is_nothrow_move_assignable_v<fanren::engine::ParticleSystem>);

namespace {

namespace fs = std::filesystem;

using fanren::engine::BlendMode;
using fanren::engine::Color;
using fanren::engine::Engine;
using fanren::engine::Particle;
using fanren::engine::ParticleKind;
using fanren::engine::ParticleSpace;
using fanren::engine::ParticleSystem;
using fanren::engine::RectF;
using fanren::engine::ScaleMode;
using fanren::engine::SpriteImage;
using fanren::engine::Vec2;

constexpr RectF kScreen{0.f, 0.f, 1280.f, 720.f};

constexpr ParticleKind kAllKinds[] = {ParticleKind::Dust, ParticleKind::Firefly, ParticleKind::Petal,
                                      ParticleKind::Leaf, ParticleKind::Rain,    ParticleKind::Snow,
                                      ParticleKind::Ember, ParticleKind::Mist};

// 逐字段比较两颗粒子（浮点也按位相等：确定性就是「一模一样」，不是「差不多」）。
bool sameParticle(const Particle& a, const Particle& b) {
    return a.x == b.x && a.y == b.y && a.vx == b.vx && a.vy == b.vy && a.age == b.age &&
           a.life == b.life && a.size == b.size && a.angle == b.angle && a.spin == b.spin &&
           a.phase == b.phase && a.sway == b.sway && a.swayFreq == b.swayFreq &&
           a.color.r == b.color.r && a.color.g == b.color.g && a.color.b == b.color.b &&
           a.color.a == b.color.a && a.variant == b.variant;
}

void run(ParticleSystem& system, int frames, float dt = 1.f / 60.f) {
    for (int i = 0; i < frames; ++i) system.update(dt);
}

}  // namespace

TEST(Particles, TheSameSeedGivesTheSameParticlesEveryTime) {
    for (const ParticleKind kind : kAllKinds) {
        ParticleSystem a(42);
        ParticleSystem b(42);
        a.configure(kind, 30.f, kScreen);
        b.configure(kind, 30.f, kScreen);
        run(a, 150);
        run(b, 150);
        ASSERT_FALSE(a.particles().empty()) << "种类 " << static_cast<int>(kind);
        ASSERT_EQ(a.particles().size(), b.particles().size());
        for (std::size_t i = 0; i < a.particles().size(); ++i) {
            EXPECT_TRUE(sameParticle(a.particles()[i], b.particles()[i]))
                << "种类 " << static_cast<int>(kind) << " 第 " << i << " 颗";
        }
    }
}

TEST(Particles, DifferentSeedsGiveDifferentParticles) {
    ParticleSystem a(1);
    ParticleSystem b(2);
    a.configure(ParticleKind::Snow, 30.f, kScreen);
    b.configure(ParticleKind::Snow, 30.f, kScreen);
    run(a, 60);
    run(b, 60);
    ASSERT_FALSE(a.particles().empty());
    ASSERT_FALSE(b.particles().empty());
    EXPECT_NE(a.particles().front().x, b.particles().front().x);
}

TEST(Particles, PrewarmIsTheSameAsSteppingAtAThirtiethOfASecond) {
    ParticleSystem warmed(9);
    ParticleSystem stepped(9);
    warmed.configure(ParticleKind::Leaf, 12.f, kScreen);
    stepped.configure(ParticleKind::Leaf, 12.f, kScreen);
    warmed.prewarm(2.f);
    run(stepped, 60, 1.f / 30.f);
    ASSERT_EQ(warmed.particles().size(), stepped.particles().size());
    for (std::size_t i = 0; i < warmed.particles().size(); ++i) {
        EXPECT_TRUE(sameParticle(warmed.particles()[i], stepped.particles()[i]));
    }
}

TEST(Particles, TheSpawnCountFollowsTheRate) {
    // 尘埃寿命六秒起，头一秒一颗都不会死：生了几颗就是几颗。
    ParticleSystem dust(3);
    dust.configure(ParticleKind::Dust, 20.f, kScreen);
    run(dust, 60);
    EXPECT_GE(dust.particles().size(), 19u);
    EXPECT_LE(dust.particles().size(), 20u);
    run(dust, 60);
    EXPECT_GE(dust.particles().size(), 39u);
    EXPECT_LE(dust.particles().size(), 40u);
}

TEST(Particles, NegativeOrZeroRateSpawnsNothing) {
    ParticleSystem none(3);
    none.configure(ParticleKind::Rain, -50.f, kScreen);
    EXPECT_EQ(none.rate(), 0.f);
    run(none, 120);
    EXPECT_TRUE(none.particles().empty());
}

TEST(Particles, TheCapacityIsNeverExceeded) {
    // 发射率写错一个数量级（多打了几个零）：画面密一点，内存与帧率不许跟着涨。
    ParticleSystem flood(5);
    flood.configure(ParticleKind::Rain, 1.0e6f, kScreen);
    for (int i = 0; i < 30; ++i) {
        flood.update(1.f / 60.f);
        ASSERT_LE(flood.particles().size(), ParticleSystem::kDefaultCapacity);
    }
    EXPECT_EQ(flood.particles().size(), ParticleSystem::kDefaultCapacity);

    flood.setCapacity(100);
    EXPECT_EQ(flood.particles().size(), 100u);
    run(flood, 30);
    EXPECT_LE(flood.particles().size(), 100u);
}

TEST(Particles, ConfigureStartsAFreshEffect) {
    ParticleSystem system(5);
    system.configure(ParticleKind::Snow, 50.f, kScreen);
    run(system, 60);
    ASSERT_FALSE(system.particles().empty());
    system.configure(ParticleKind::Ember, 10.f, RectF{100.f, 500.f, 40.f, 10.f}, ParticleSpace::World);
    EXPECT_TRUE(system.particles().empty());
    EXPECT_EQ(system.kind(), ParticleKind::Ember);
    EXPECT_EQ(system.space(), ParticleSpace::World);
    EXPECT_EQ(system.area().x, 100.f);
}

TEST(Particles, EachKindMovesTheWayItShould) {
    const auto warmed = [](ParticleKind kind, float rate) {
        ParticleSystem system(11);
        system.configure(kind, rate, kScreen);
        system.prewarm(4.f);
        return system;
    };
    const auto all = [](const ParticleSystem& s, auto pred) {
        return !s.particles().empty() &&
               std::all_of(s.particles().begin(), s.particles().end(), pred);
    };

    const ParticleSystem rain = warmed(ParticleKind::Rain, 200.f);
    EXPECT_TRUE(all(rain, [](const Particle& p) { return p.vy > 600.f && p.vx < 0.f; }))
        << "雨：又快又往左斜";
    const ParticleSystem snow = warmed(ParticleKind::Snow, 30.f);
    EXPECT_TRUE(all(snow, [](const Particle& p) { return p.vy > 0.f && p.vy < 60.f; }))
        << "雪：慢慢往下";
    for (const ParticleKind kind : {ParticleKind::Petal, ParticleKind::Leaf}) {
        const ParticleSystem falling = warmed(kind, 10.f);
        EXPECT_TRUE(all(falling, [](const Particle& p) {
            return p.vy > 0.f && p.spin != 0.f && p.sway > 0.f;
        })) << "花瓣与落叶：往下飘、会转、会摆";
    }
    const ParticleSystem embers = warmed(ParticleKind::Ember, 30.f);
    EXPECT_TRUE(all(embers, [](const Particle& p) { return p.vy < 0.f; })) << "火星往上冒";
    const ParticleSystem mist = warmed(ParticleKind::Mist, 2.f);
    EXPECT_TRUE(all(mist, [](const Particle& p) { return p.size >= 150.f && p.spin == 0.f; }))
        << "薄雾：大团、不转";
    const ParticleSystem dust = warmed(ParticleKind::Dust, 10.f);
    EXPECT_TRUE(all(dust, [](const Particle& p) {
        return std::fabs(p.vx) < 10.f && std::fabs(p.vy) < 10.f;
    })) << "尘埃：几乎不动";
    // 雨比雪快得多：拿速度比一比，别只看符号。
    EXPECT_GT(rain.particles().front().vy, 10.f * snow.particles().front().vy);
}

TEST(Particles, FallingKindsLeaveThroughTheBottomAndEmbersRiseFromTheBottomEdge) {
    ParticleSystem rain(21);
    rain.configure(ParticleKind::Rain, 300.f, kScreen);
    rain.prewarm(5.f);
    ASSERT_FALSE(rain.particles().empty());
    for (const Particle& p : rain.particles()) {
        EXPECT_LE(p.y, kScreen.h + 60.f) << "落出底边的雨丝应当被收走";
        EXPECT_GE(p.y, -60.f);
    }
    // 画面上下都有雨：稳态是铺满的，不是一截一截的。
    const auto [lowest, highest] = std::minmax_element(
        rain.particles().begin(), rain.particles().end(),
        [](const Particle& a, const Particle& b) { return a.y < b.y; });
    EXPECT_LT(lowest->y, 100.f);
    EXPECT_GT(highest->y, 620.f);

    const RectF bowl{100.f, 500.f, 40.f, 10.f};
    ParticleSystem embers(22);
    embers.configure(ParticleKind::Ember, 40.f, bowl, ParticleSpace::World);
    embers.prewarm(3.f);
    ASSERT_FALSE(embers.particles().empty());
    for (const Particle& p : embers.particles()) {
        EXPECT_LE(p.y, bowl.y + bowl.h + 1.f) << "火星从火盆口往上走，不会掉到下面去";
        EXPECT_GE(p.x, bowl.x - 30.f);
        EXPECT_LE(p.x, bowl.x + bowl.w + 30.f);
    }
    // area 是「从哪儿冒」，不是「只许待在哪儿」：冒出火盆口之后照样往上飞，直到寿命用完。
    // （曾经按 area 顶边收走，火星冒出火盆口二十来个像素就没了——截图上一颗都看不见。）
    const auto topmost = std::min_element(
        embers.particles().begin(), embers.particles().end(),
        [](const Particle& a, const Particle& b) { return a.y < b.y; });
    EXPECT_LT(topmost->y, bowl.y - 60.f);
}

TEST(Particles, WorldSpaceSubtractsTheCameraAndScreenSpaceIgnoresIt) {
    Particle p;
    p.x = 400.f;
    p.y = 300.f;
    const Vec2 screen = fanren::engine::particleScreenPosition(ParticleKind::Snow, p,
                                                               ParticleSpace::Screen, 123.f, 45.f);
    const Vec2 world = fanren::engine::particleScreenPosition(ParticleKind::Snow, p,
                                                              ParticleSpace::World, 123.f, 45.f);
    EXPECT_EQ(screen.x, 400.f);
    EXPECT_EQ(screen.y, 300.f);
    EXPECT_EQ(world.x, 277.f);
    EXPECT_EQ(world.y, 255.f);
}

TEST(Particles, SwayStaysWithinItsAmplitude) {
    Particle p;
    p.x = 500.f;
    p.y = 200.f;
    p.sway = 12.f;
    p.swayFreq = 0.7f;
    p.phase = 1.1f;
    float widest = 0.f;
    for (int i = 0; i < 2000; ++i) {
        p.age = static_cast<float>(i) * 0.01f;
        for (const ParticleKind kind : {ParticleKind::Leaf, ParticleKind::Firefly}) {
            const Vec2 at = fanren::engine::particleScreenPosition(kind, p, ParticleSpace::Screen, 0.f, 0.f);
            EXPECT_LE(std::fabs(at.x - p.x), p.sway + 1e-3f);
            EXPECT_LE(std::fabs(at.y - p.y), p.sway + 1e-3f);
            widest = std::max(widest, std::fabs(at.x - p.x));
        }
    }
    EXPECT_GT(widest, 0.9f * p.sway);   // 先验：真的在摆
}

TEST(Particles, LifeEnvelopeFadesInHoldsAndFadesOut) {
    using fanren::engine::lifeEnvelope;
    EXPECT_EQ(lifeEnvelope(0.f, 10.f, 0.2f, 0.3f), 0.f);
    EXPECT_NEAR(lifeEnvelope(1.f, 10.f, 0.2f, 0.3f), 0.5f, 1e-6f);
    EXPECT_EQ(lifeEnvelope(5.f, 10.f, 0.2f, 0.3f), 1.f);
    EXPECT_NEAR(lifeEnvelope(8.5f, 10.f, 0.2f, 0.3f), 0.5f, 1e-5f);
    EXPECT_EQ(lifeEnvelope(10.f, 10.f, 0.2f, 0.3f), 0.f);
    EXPECT_EQ(lifeEnvelope(3.f, 0.f, 0.2f, 0.3f), 0.f);
    EXPECT_EQ(lifeEnvelope(0.f, 10.f, 0.f, 0.f), 1.f);   // 不淡入淡出：生下来就是满的
    for (int i = 0; i <= 100; ++i) {
        const float v = lifeEnvelope(static_cast<float>(i) * 0.1f, 10.f, 0.2f, 0.3f);
        EXPECT_GE(v, 0.f);
        EXPECT_LE(v, 1.f);
    }
}

TEST(Particles, GlowKindsAddAndEverythingElseBlends) {
    EXPECT_EQ(fanren::engine::particleBlend(ParticleKind::Dust), BlendMode::Add);
    EXPECT_EQ(fanren::engine::particleBlend(ParticleKind::Firefly), BlendMode::Add);
    EXPECT_EQ(fanren::engine::particleBlend(ParticleKind::Ember), BlendMode::Add);
    for (const ParticleKind kind : {ParticleKind::Petal, ParticleKind::Leaf, ParticleKind::Rain,
                                    ParticleKind::Snow, ParticleKind::Mist}) {
        EXPECT_EQ(fanren::engine::particleBlend(kind), BlendMode::Alpha) << static_cast<int>(kind);
    }
}

TEST(Particles, ProceduralSpritesAreRealImages) {
    for (const ParticleKind kind : kAllKinds) {
        const int variants = fanren::engine::proceduralVariants(kind);
        if (kind == ParticleKind::Rain) {
            EXPECT_EQ(variants, 0) << "雨画成线，不要贴图";
            continue;
        }
        ASSERT_GE(variants, 1);
        for (int v = 0; v < variants; ++v) {
            const SpriteImage img = fanren::engine::proceduralSprite(kind, v);
            ASSERT_GT(img.w, 0);
            ASSERT_GT(img.h, 0);
            ASSERT_EQ(img.pixels.size(), static_cast<std::size_t>(img.w * img.h));
            const auto alpha = [](std::uint32_t p) { return p & 0xFFu; };
            const auto opaque = std::count_if(img.pixels.begin(), img.pixels.end(),
                                              [&](std::uint32_t p) { return alpha(p) > 128u; });
            const auto clear = std::count_if(img.pixels.begin(), img.pixels.end(),
                                             [&](std::uint32_t p) { return alpha(p) == 0u; });
            EXPECT_GT(opaque, 0) << "种类 " << static_cast<int>(kind) << " 变体 " << v;
            EXPECT_GT(clear, 0) << "四周要是透明的，不能是一块实心方砖";
            EXPECT_EQ(alpha(img.pixels.front()), 0u) << "左上角透明";
        }
        // 叶与花瓣是像素小图（按 ×3 画），光点与柔圆是线性取样的。
        const bool pixelArt = kind == ParticleKind::Leaf || kind == ParticleKind::Petal;
        EXPECT_EQ(fanren::engine::proceduralSprite(kind, 0).scale,
                  pixelArt ? ScaleMode::Pixel : ScaleMode::Linear);
    }
    // 三种叶形、两种花瓣：变体之间真的不一样。
    EXPECT_NE(fanren::engine::proceduralSprite(ParticleKind::Leaf, 0).pixels,
              fanren::engine::proceduralSprite(ParticleKind::Leaf, 1).pixels);
    EXPECT_NE(fanren::engine::proceduralSprite(ParticleKind::Petal, 0).pixels,
              fanren::engine::proceduralSprite(ParticleKind::Petal, 1).pixels);
}

TEST(Particles, SheetLayoutSplitsASingleRowOfSquareFrames) {
    // assets/art/fx 的约定：单行横排、方形帧。这几组尺寸就是那一路的实际产物。
    const struct {
        fanren::engine::Point size;
        int frames, frameW, frameH;
    } kCases[] = {
        {{32, 8}, 4, 8, 8},    // leaves / petals / embers
        {{24, 8}, 3, 8, 8},    // snow / spark
        {{16, 8}, 2, 8, 8},    // firefly
        {{4, 16}, 1, 4, 16},   // rain：比高还窄，就是一帧
        {{64, 64}, 1, 64, 64}, // glow
        {{4, 4}, 1, 4, 4},     // dust
        {{0, 0}, 1, 0, 0},
    };
    for (const auto& c : kCases) {
        const fanren::engine::SheetLayout got = fanren::engine::sheetLayout(c.size);
        EXPECT_EQ(got.frames, c.frames) << c.size.x << "x" << c.size.y;
        EXPECT_EQ(got.frameW, c.frameW) << c.size.x << "x" << c.size.y;
        EXPECT_EQ(got.frameH, c.frameH) << c.size.x << "x" << c.size.y;
    }
}

TEST(Particles, ParticleFrameCyclesWithFpsOrStaysFixed) {
    Particle p;
    p.variant = 6;
    // fps = 0：每颗粒子一辈子就是那一帧（叶形不会一边飘一边变形）。
    for (int i = 0; i < 50; ++i) {
        p.age = static_cast<float>(i) * 0.137f;
        EXPECT_EQ(fanren::engine::particleFrame(p, 4, 0.f), 6 % 4);
    }
    // fps = 10：每 0.1 秒进一帧，四帧一轮；帧号始终在范围内。
    std::vector<int> seen;
    for (int i = 0; i < 8; ++i) {
        p.age = 0.05f + static_cast<float>(i) * 0.1f;
        const int frame = fanren::engine::particleFrame(p, 4, 10.f);
        ASSERT_GE(frame, 0);
        ASSERT_LT(frame, 4);
        seen.push_back(frame);
    }
    for (std::size_t i = 1; i < seen.size(); ++i) {
        EXPECT_EQ(seen[i], (seen[i - 1] + 1) % 4);
    }
    EXPECT_EQ(fanren::engine::particleFrame(p, 1, 10.f), 0);
}

TEST(Particles, RenderingHeadlessIsANoOp) {
    Engine engine;
    ASSERT_TRUE(engine.init("fanren-tests", true).ok);
    {
        ParticleSystem system(4);
        system.configure(ParticleKind::Leaf, 20.f, kScreen);
        system.prewarm(2.f);
        const std::size_t before = system.particles().size();
        engine.beginFrame();
        system.render(engine, 10.f, 20.f);
        engine.endFrame();
        EXPECT_EQ(system.particles().size(), before);   // render 不推进模拟
    }
    engine.shutdown();
}

namespace {

// 软件渲染器上把每一种粒子画一遍，数屏幕上亮起来的像素。
// artDir 为空用缺省目录（有美术就用美术）；给一个不存在的目录则一律程序贴图。
// 返回每一种用没用上美术贴图。
class SoftwareParticles : public ::testing::Test {
protected:
    void SetUp() override {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        const auto ready = engine.init("fanren-tests", false);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override {
        engine.shutdown();
        SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
    }

    std::vector<bool> renderEveryKind(const std::string& artDir) {
        std::vector<bool> usedArt;
        for (const ParticleKind kind : kAllKinds) {
            ParticleSystem system(31);
            if (!artDir.empty()) system.setArtDirectory(artDir);
            // 薄雾一秒一团也够铺，其余给足量，保证画面上一定有几颗在飘。
            system.configure(kind, kind == ParticleKind::Mist ? 3.f : 60.f, kScreen);
            system.prewarm(6.f);
            EXPECT_FALSE(system.particles().empty());

            engine.beginFrame();
            system.render(engine, 0.f, 0.f);
            const fs::path png = dir.path() / ("kind" + std::to_string(shots++) + ".png");
            EXPECT_TRUE(engine.captureFrame(png.string()).ok);
            engine.endFrame();
            usedArt.push_back(system.usesArt());

            SDL_Surface* loaded = SDL_LoadPNG(png.string().c_str());
            EXPECT_NE(loaded, nullptr) << SDL_GetError();
            if (loaded == nullptr) continue;
            SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(loaded);
            if (rgba == nullptr) continue;
            int lit = 0;
            for (int y = 0; y < rgba->h; ++y) {
                const auto* row = static_cast<const std::uint8_t*>(rgba->pixels) + y * rgba->pitch;
                for (int x = 0; x < rgba->w; ++x) {
                    lit += (row[x * 4] + row[x * 4 + 1] + row[x * 4 + 2]) > 30 ? 1 : 0;
                }
            }
            SDL_DestroySurface(rgba);
            EXPECT_GT(lit, 200) << "种类 " << static_cast<int>(kind) << " 在屏幕上几乎没画出东西";
        }
        return usedArt;
    }

    Engine engine;
    fanren::test::TempDir dir{"fanren_particles_sw"};
    int shots = 0;
};

}  // namespace

TEST_F(SoftwareParticles, EveryKindPutsPixelsOnScreenWithProceduralSprites) {
    // 指一个没有图的目录：不管这棵树里美术到没到，验的都是程序贴图这一种画法。
    for (const bool art : renderEveryKind("__no_such_art_dir__")) {
        EXPECT_FALSE(art);
    }
}

TEST_F(SoftwareParticles, EveryKindPutsPixelsOnScreenWithTheArtRoutesSprites) {
    if (engine.findAssets(fanren::engine::kParticleArtDir, "*.png").empty()) {
        GTEST_SKIP() << "assets/art/fx 里还没有美术贴图，这一种画法此处验不到";
    }
    const std::vector<bool> usedArt = renderEveryKind(std::string{});
    // 先验：美术那条路真的走到了（不是悄悄全退回了程序贴图）。
    EXPECT_TRUE(std::any_of(usedArt.begin(), usedArt.end(), [](bool b) { return b; }));
}
