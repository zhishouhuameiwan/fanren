#include "game/MapVisual.h"

#include <algorithm>
#include <cmath>

namespace fanren::game {
namespace {

engine::Color colorOf(const core::Rgb& rgb) {
    return engine::Color{rgb.r, rgb.g, rgb.b, 255};
}

// meta 的粒子种类 → 引擎的。两边一一对应；core 那一份多了一种而这里没接上，编译器的 switch 警告会响。
engine::ParticleKind particleKindOf(core::AmbientParticle kind) {
    using engine::ParticleKind;
    switch (kind) {
        case core::AmbientParticle::Dust: return ParticleKind::Dust;
        case core::AmbientParticle::Firefly: return ParticleKind::Firefly;
        case core::AmbientParticle::Petal: return ParticleKind::Petal;
        case core::AmbientParticle::Leaf: return ParticleKind::Leaf;
        case core::AmbientParticle::Rain: return ParticleKind::Rain;
        case core::AmbientParticle::Snow: return ParticleKind::Snow;
        case core::AmbientParticle::Ember: return ParticleKind::Ember;
        case core::AmbientParticle::Mist: return ParticleKind::Mist;
    }
    return ParticleKind::Dust;   // 枚举之外的值只可能来自内存损坏；给一个画得出来的
}

}  // namespace

MapVisual visualDefaults(TimeOfDay time) {
    MapVisual v;
    v.time = time;
    // 与 tools/artgen/maplights.py 的 TIME_DEFAULTS 逐项相同。
    switch (time) {
        case TimeOfDay::Day:
            v.ambient = engine::Color{236, 232, 222, 255};
            v.dof = 0.3f;
            v.bloom = 0.2f;
            v.vignette = 0.25f;
            break;
        case TimeOfDay::Dusk:
            v.ambient = engine::Color{218, 164, 132, 255};
            v.dof = 0.35f;
            v.bloom = 0.45f;
            v.vignette = 0.4f;
            break;
        case TimeOfDay::Night:
            v.ambient = engine::Color{84, 96, 142, 255};
            v.dof = 0.35f;
            v.bloom = 0.6f;
            v.vignette = 0.5f;
            break;
        case TimeOfDay::Indoor:
            v.ambient = engine::Color{132, 116, 98, 255};
            v.dof = 0.25f;
            v.bloom = 0.5f;
            v.vignette = 0.45f;
            break;
    }
    return v;
}

MapVisual fallbackVisual(bool outdoor) {
    return visualDefaults(outdoor ? TimeOfDay::Day : TimeOfDay::Indoor);
}

MapVisual mapVisualFrom(const core::MapMeta& meta) {
    MapVisual out = visualDefaults(meta.time);
    out.theme = meta.theme;
    if (meta.ambient) out.ambient = colorOf(*meta.ambient);
    if (meta.dof) out.dof = *meta.dof;
    if (meta.bloom) out.bloom = *meta.bloom;
    if (meta.vignette) out.vignette = *meta.vignette;
    for (const core::MetaParticleLayer& layer : meta.particles) {
        out.particles.push_back(ParticleLayer{particleKindOf(layer.kind), layer.density});
    }
    for (const core::MetaLight& light : meta.lights) {
        out.lights.push_back(MapLight{light.x, light.y, light.radius, colorOf(light.color), light.intensity,
                                      light.flicker, light.kind});
    }
    out.water = meta.water;
    for (const core::MetaEmitter& emitter : meta.emitters) {
        out.emitters.push_back(ParticleEmitter{particleKindOf(emitter.kind), emitter.x, emitter.y});
    }
    return out;
}

engine::PostFxSettings postFxFor(const MapVisual& visual, float focusY) {
    engine::PostFxSettings s;
    s.ambient = visual.ambient;
    s.dof = std::clamp(visual.dof, 0.f, 1.f);
    // 移轴：清晰带跟着主角走（镜头在地图边上夹住时主角不在正中，焦点仍在他身上）。
    // 夹在 0.3–0.7：主角贴着屏幕边站时，不让清晰带整条滑出画面。
    s.dofFocus = std::clamp(focusY, 0.3f, 0.7f);
    s.dofBand = 0.30f;
    s.dofRamp = 0.22f;
    s.bloom = visual.bloom;
    s.vignette = std::clamp(visual.vignette, 0.f, 1.f);
    // 调色按时辰：meta 没有这一项（烘焙图是中性的，冷暖交给运行时），这里给一套偏移很小的数，
    // 只为让昼暖、暮橙、夜青、室内烛黄在同一套美术上分得开。
    switch (visual.time) {
        case TimeOfDay::Day:
            s.gradeMul = engine::Color{255, 250, 242, 255};
            s.gradeAdd = engine::Color{3, 2, 0, 255};
            break;
        case TimeOfDay::Dusk:
            s.gradeMul = engine::Color{255, 230, 206, 255};
            s.gradeAdd = engine::Color{10, 4, 0, 255};
            break;
        case TimeOfDay::Night:
            s.gradeMul = engine::Color{212, 222, 255, 255};
            s.gradeAdd = engine::Color{0, 2, 8, 255};
            break;
        case TimeOfDay::Indoor:
            s.gradeMul = engine::Color{255, 238, 214, 255};
            s.gradeAdd = engine::Color{5, 3, 0, 255};
            break;
    }
    return s;
}

CarriedLight carriedLightFor(TimeOfDay time) {
    switch (time) {
        case TimeOfDay::Day: return CarriedLight{};
        case TimeOfDay::Dusk: return CarriedLight{true, 2.6f, engine::Color{255, 226, 190, 255}, 0.5f};
        case TimeOfDay::Night: return CarriedLight{true, 3.4f, engine::Color{196, 208, 240, 255}, 0.8f};
        case TimeOfDay::Indoor: return CarriedLight{true, 3.2f, engine::Color{255, 222, 176, 255}, 0.7f};
    }
    return CarriedLight{};
}

bool bakedLayerUsable(engine::Point textureSize, int mapWidthCells, int mapHeightCells) {
    constexpr int kArtCell = 16;   // 烘焙图一格 16 像素（施工图 1.1）
    return textureSize.x > 0 && textureSize.y > 0 && textureSize.x == mapWidthCells * kArtCell &&
           textureSize.y == mapHeightCells * kArtCell;
}

float particleRate(engine::ParticleKind kind, float density, float areaScreens) {
    // 浓淡为 1 时，一块屏幕那么大的地方每秒生几颗。乘上寿命（Particles.cpp 的 kSpecs）就是
    // 屏幕上同时有几颗：尘埃 24×8 秒 ≈ 190、萤火 10×7 ≈ 70、雾团 1.6×19 ≈ 30。
    // 落下来的活得久（从屏幕顶飘到底要二三十秒）：花瓣 2.4×29 ≈ 70、落叶 2×22 ≈ 44、雪 12×30 ≈ 360。
    // 头一版按「每秒几片」拍了个 14，韩家村 0.5 的浓淡满屏两百片花瓣，像下了一场粉色的雪。
    // meta 里写的浓淡在 0.12–0.5 之间，于是屏幕上是几十颗尘埃、二三十片花瓣、十几团雾——够「有」，不至于「脏」。
    float full = 0.f;
    switch (kind) {
        case engine::ParticleKind::Dust: full = 24.f; break;
        case engine::ParticleKind::Firefly: full = 10.f; break;
        case engine::ParticleKind::Petal: full = 2.4f; break;
        case engine::ParticleKind::Leaf: full = 2.f; break;
        case engine::ParticleKind::Rain: full = 700.f; break;
        case engine::ParticleKind::Snow: full = 12.f; break;
        case engine::ParticleKind::Ember: full = 12.f; break;
        case engine::ParticleKind::Mist: full = 1.6f; break;
    }
    return full * std::max(0.f, density) * std::max(0.f, areaScreens);
}

float flickerFactor(float flicker, float seconds, float phase) {
    const float wobble = 0.6f * std::sin(seconds * 7.3f + phase) +
                         0.4f * std::sin(seconds * 12.7f + phase * 1.9f);
    return 1.f + 0.5f * flicker * wobble;
}

}  // namespace fanren::game
