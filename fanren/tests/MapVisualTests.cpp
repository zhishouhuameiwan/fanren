// game/MapVisual：烘焙附带的 meta.json（A1 路 tools/artgen/maplights.py 写）与烘焙图的取舍判据。
// meta.json 由 io 层读（io::parseMapMeta / loadMapMeta），这里量「读 + 按时辰补缺省」合起来的结果。
//
// 缺省值的判据写成字面量（抄的是烘焙器 TIME_DEFAULTS 那张表），不从被测物推：
// 改了一边、另一边没跟，这里就红——两边的缺省一旦分家，画面与烘焙时的取景就对不上。
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "game/MapVisual.h"
#include "io/VisualLoader.h"
#include "TempDir.h"

namespace {

namespace fs = std::filesystem;
using fanren::engine::Color;
using fanren::engine::ParticleKind;
using fanren::game::MapVisual;
using fanren::game::TimeOfDay;

// io 层读 meta（io::parseMapMeta）+ game 层按时辰补缺省（mapVisualFrom）：两步合起来才是
// 世界画面拿到的样子，这里的判据量的就是这一整条。
fanren::core::Result<MapVisual> parseMapVisual(std::string_view json) {
    auto meta = fanren::io::parseMapMeta(json);
    if (!meta) return fanren::core::Result<MapVisual>::failure(meta.error);
    return fanren::core::Result<MapVisual>::success(fanren::game::mapVisualFrom(meta.value));
}

bool sameRgb(const Color& a, int r, int g, int b) {
    return a.r == r && a.g == g && a.b == b;
}

// 烘焙器写出来的样子（maplights.py 的 meta()，json.dumps(indent=1)）。
constexpr const char* kBakedMeta = R"({
 "map": "ch05_mofu",
 "tile": 16,
 "width": 48,
 "height": 36,
 "theme": "manor",
 "time": "night",
 "season": "autumn",
 "ambient": [84, 96, 142],
 "dof": 0.35,
 "bloom": 0.6,
 "vignette": 0.5,
 "particles": [{"kind": "firefly", "rate": 0.3}, {"kind": "leaf", "rate": 0.15}],
 "lights": [
  {"x": 23.05, "y": 13.9, "r": 3.0, "color": [201, 110, 64], "intensity": 0.8, "flicker": 0.15, "kind": "lantern"},
  {"x": 14.5, "y": 27.5, "r": 1.8, "color": [95, 174, 140], "intensity": 0.5, "flicker": 0.0, "kind": "save"}
 ],
 "water": [[3, 4], [4, 4]],
 "emitters": [{"kind": "ember", "x": 20.5, "y": 3.5}, {"kind": "mist", "x": 3.5, "y": 4.5}],
 "backdrop": "manor_night"
})";

TEST(MapVisual, ReadsEveryFieldTheBakerWrites) {
    auto parsed = parseMapVisual(kBakedMeta);
    ASSERT_TRUE(parsed.ok) << parsed.error;
    const MapVisual& v = parsed.value;
    EXPECT_EQ(v.theme, "manor");
    EXPECT_EQ(v.time, TimeOfDay::Night);
    EXPECT_TRUE(sameRgb(v.ambient, 84, 96, 142));
    EXPECT_FLOAT_EQ(v.dof, 0.35f);
    EXPECT_FLOAT_EQ(v.bloom, 0.6f);
    EXPECT_FLOAT_EQ(v.vignette, 0.5f);
    ASSERT_EQ(v.particles.size(), 2u);
    EXPECT_EQ(v.particles[0].kind, ParticleKind::Firefly);
    EXPECT_FLOAT_EQ(v.particles[0].density, 0.3f);
    EXPECT_EQ(v.particles[1].kind, ParticleKind::Leaf);
    ASSERT_EQ(v.lights.size(), 2u);
    EXPECT_FLOAT_EQ(v.lights[0].x, 23.05f);
    EXPECT_FLOAT_EQ(v.lights[0].radius, 3.f);
    EXPECT_TRUE(sameRgb(v.lights[0].color, 201, 110, 64));
    EXPECT_FLOAT_EQ(v.lights[0].flicker, 0.15f);
    EXPECT_EQ(v.lights[0].kind, "lantern");
    EXPECT_EQ(v.lights[1].kind, "save");
    ASSERT_EQ(v.water.size(), 2u);
    EXPECT_EQ(v.water[1].x, 4);
    EXPECT_EQ(v.water[1].y, 4);
    ASSERT_EQ(v.emitters.size(), 2u);
    EXPECT_EQ(v.emitters[0].kind, ParticleKind::Ember);
    EXPECT_FLOAT_EQ(v.emitters[1].y, 4.5f);
}

TEST(MapVisual, MissingFieldsTakeTheTimeOfDayDefaults) {
    // 与烘焙器 TIME_DEFAULTS 逐项相同（字面量抄自 tools/artgen/maplights.py）。
    struct Expect {
        const char* json;
        TimeOfDay time;
        int r, g, b;
        float dof, bloom, vignette;
    };
    const Expect cases[] = {
        {R"({"time": "day"})", TimeOfDay::Day, 236, 232, 222, 0.3f, 0.2f, 0.25f},
        {R"({"time": "dusk"})", TimeOfDay::Dusk, 218, 164, 132, 0.35f, 0.45f, 0.4f},
        {R"({"time": "night"})", TimeOfDay::Night, 84, 96, 142, 0.35f, 0.6f, 0.5f},
        {R"({"time": "indoor"})", TimeOfDay::Indoor, 132, 116, 98, 0.25f, 0.5f, 0.45f},
    };
    for (const Expect& c : cases) {
        auto parsed = parseMapVisual(c.json);
        ASSERT_TRUE(parsed.ok) << parsed.error;
        const MapVisual& v = parsed.value;
        EXPECT_EQ(v.time, c.time) << c.json;
        EXPECT_TRUE(sameRgb(v.ambient, c.r, c.g, c.b)) << c.json;
        EXPECT_FLOAT_EQ(v.dof, c.dof) << c.json;
        EXPECT_FLOAT_EQ(v.bloom, c.bloom) << c.json;
        EXPECT_FLOAT_EQ(v.vignette, c.vignette) << c.json;
        EXPECT_TRUE(v.particles.empty() && v.lights.empty() && v.water.empty() && v.emitters.empty())
            << "缺了的表就是空表，不替地图编灯与粒子";
    }
    // 写了的覆盖缺省，没写的仍按时辰。
    auto partial = parseMapVisual(R"({"time": "indoor", "ambient": [96, 84, 78]})");
    ASSERT_TRUE(partial.ok) << partial.error;
    EXPECT_TRUE(sameRgb(partial.value.ambient, 96, 84, 78));
    EXPECT_FLOAT_EQ(partial.value.bloom, 0.5f);
}

TEST(MapVisual, WithoutATimeTheThemeDecides) {
    // 烘焙器里三种室内主题一律取 indoor 时辰，其余的缺省是昼。
    for (const char* theme : {"indoor_wood", "indoor_stone", "cave"}) {
        auto parsed = parseMapVisual(std::string(R"({"theme": ")") + theme + "\"}");
        ASSERT_TRUE(parsed.ok) << parsed.error;
        EXPECT_EQ(parsed.value.time, TimeOfDay::Indoor) << theme;
    }
    auto village = parseMapVisual(R"({"theme": "village"})");
    ASSERT_TRUE(village.ok);
    EXPECT_EQ(village.value.time, TimeOfDay::Day);
    auto empty = parseMapVisual("{}");
    ASSERT_TRUE(empty.ok);
    EXPECT_EQ(empty.value.time, TimeOfDay::Day);
}

TEST(MapVisual, ANightMapWithoutMetaFallsBackByIndoorOrOutdoor) {
    // meta.json 整个没有（烘焙还没到这张图）：室外按昼、室内按室内，不点灯、不下粒子。
    const MapVisual outdoor = fanren::game::fallbackVisual(true);
    EXPECT_EQ(outdoor.time, TimeOfDay::Day);
    EXPECT_TRUE(outdoor.lights.empty() && outdoor.particles.empty());
    EXPECT_EQ(fanren::game::fallbackVisual(false).time, TimeOfDay::Indoor);
}

TEST(MapVisual, BrokenMetaIsAnErrorThatSaysWhere) {
    struct Case {
        const char* json;
        const char* mentions;   // 报错里必须点到的字段
    };
    const Case cases[] = {
        {R"({"time": "noon"})", "time"},
        {R"({"time": 3})", "time"},
        {R"({"ambient": [84, 96]})", "ambient"},
        {R"({"ambient": [84, 96, 300]})", "ambient"},
        {R"({"dof": -0.1})", "dof"},
        {R"({"bloom": "strong"})", "bloom"},
        {R"({"particles": [{"kind": "fireflies", "rate": 0.3}]})", "particles[0].kind"},
        {R"({"particles": [{"rate": 0.3}]})", "particles[0].kind"},
        {R"({"particles": {"kind": "dust"}})", "particles"},
        {R"({"lights": [{"y": 3, "r": 2}]})", "lights[0].x"},
        {R"({"lights": [{"x": 1, "y": 3, "color": [1, 2]}]})", "lights[0].color"},
        {R"({"water": [[1, 2, 3]]})", "water[0]"},
        {R"({"water": [[1.5, 2]]})", "water[0]"},
        {R"({"emitters": [{"kind": "smoke", "x": 1, "y": 2}]})", "emitters[0].kind"},
        {R"({"emitters": [{"kind": "ember", "x": 1}]})", "emitters[0].y"},
        {R"({"theme": ["cave"]})", "theme"},
        {R"([])", "顶层"},
    };
    for (const Case& c : cases) {
        auto parsed = parseMapVisual(c.json);
        EXPECT_FALSE(parsed.ok) << "竟然读成功了：" << c.json;
        EXPECT_NE(parsed.error.find(c.mentions), std::string::npos)
            << c.json << " 的报错没点到 " << c.mentions << "：" << parsed.error;
    }
    // 语法坏了也是报错，带行号。
    auto syntax = parseMapVisual("{\n \"time\": \"night\",\n}");
    EXPECT_FALSE(syntax.ok);
    EXPECT_NE(syntax.error.find("第 3 行"), std::string::npos) << syntax.error;
}

TEST(MapVisual, LoadingABadOrMissingFileIsAnErrorWithThePath) {
    const fanren::test::TempDir dir("fanren_mapvisual");
    const fs::path bad = dir.path() / "meta.json";
    {
        std::ofstream out(bad, std::ios::binary);
        out << "{\"time\": \"night\", \"lights\": [{\"x\": 1}]}";
    }
    auto loaded = fanren::io::loadMapMeta(bad.string());
    EXPECT_FALSE(loaded.ok);
    EXPECT_NE(loaded.error.find("meta.json"), std::string::npos) << loaded.error;
    EXPECT_NE(loaded.error.find("lights[0].y"), std::string::npos) << loaded.error;

    auto missing = fanren::io::loadMapMeta((dir.path() / "none.json").string());
    EXPECT_FALSE(missing.ok);
    EXPECT_NE(missing.error.find("none.json"), std::string::npos) << missing.error;
}

TEST(MapVisual, EveryBakedMetaInTheRepoParses) {
    // A1 的产物与本读取器的契约：仓库里已经烘焙出来的每一份 meta.json 都读得进来。
    // 烘焙还没到的图没有 meta（运行时按室内外缺省画），那不算错。
    fs::path root = ".";
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_hanjiacun.tmj")) {
            root = candidate;
            break;
        }
    }
    const fs::path maps = root / "assets" / "art" / "maps";
    int seen = 0;
    if (fs::exists(maps)) {
        for (const fs::directory_entry& entry : fs::directory_iterator(maps)) {
            const fs::path meta = entry.path() / "meta.json";
            if (!fs::exists(meta)) continue;
            ++seen;
            auto loaded = fanren::io::loadMapMeta(meta.string());
            EXPECT_TRUE(loaded.ok) << loaded.error;
        }
    }
    if (seen == 0) GTEST_SKIP() << "仓库里还没有烘焙好的 meta.json（" << maps.string() << "）";
}

TEST(MapVisual, BakedLayerIsUsableOnlyWhenItLoadedAndMatchesTheMap) {
    // 「缺烘焙图退回旧画法」的判据：载不进来（尺寸 0）或与地图格数 ×16 对不上，都退回。
    using fanren::game::bakedLayerUsable;
    EXPECT_TRUE(bakedLayerUsable({640, 480}, 40, 30));
    EXPECT_FALSE(bakedLayerUsable({0, 0}, 40, 30)) << "没载进来（缺图、坏图、无头）";
    EXPECT_FALSE(bakedLayerUsable({656, 480}, 40, 30)) << "宽多一格：烘焙与地图脱节";
    EXPECT_FALSE(bakedLayerUsable({640, 464}, 40, 30)) << "高少一格";
    EXPECT_FALSE(bakedLayerUsable({480, 640}, 40, 30)) << "宽高对调";
    EXPECT_FALSE(bakedLayerUsable({1280, 960}, 40, 30)) << "按 32px 一格出的图不是 16px 美术";
}

TEST(MapVisual, PostFxFocusesTheTiltShiftOnTheHero) {
    auto parsed = parseMapVisual(kBakedMeta);
    ASSERT_TRUE(parsed.ok);
    const auto s = fanren::game::postFxFor(parsed.value, 0.5f);
    EXPECT_TRUE(sameRgb(s.ambient, 84, 96, 142));
    EXPECT_FLOAT_EQ(s.dof, 0.35f);
    EXPECT_FLOAT_EQ(s.bloom, 0.6f);
    EXPECT_FLOAT_EQ(s.vignette, 0.5f);
    EXPECT_FLOAT_EQ(s.dofFocus, 0.5f);
    // 主角贴着屏幕边时清晰带不整条滑出画面。
    EXPECT_FLOAT_EQ(fanren::game::postFxFor(parsed.value, 0.05f).dofFocus, 0.3f);
    EXPECT_FLOAT_EQ(fanren::game::postFxFor(parsed.value, 0.95f).dofFocus, 0.7f);
    // 夜里调色偏青：蓝通道乘得比红通道多。
    EXPECT_GT(s.gradeMul.b, s.gradeMul.r);
    const auto dusk = fanren::game::postFxFor(fanren::game::visualDefaults(TimeOfDay::Dusk), 0.5f);
    EXPECT_GT(dusk.gradeMul.r, dusk.gradeMul.b) << "黄昏偏暖";
}

TEST(MapVisual, TheHeroCarriesALightAtNightIndoorsAndAtDuskButNotByDay) {
    using fanren::game::carriedLightFor;
    EXPECT_FALSE(carriedLightFor(TimeOfDay::Day).on);
    EXPECT_TRUE(carriedLightFor(TimeOfDay::Night).on);
    EXPECT_TRUE(carriedLightFor(TimeOfDay::Indoor).on);
    EXPECT_TRUE(carriedLightFor(TimeOfDay::Dusk).on);
    EXPECT_GT(carriedLightFor(TimeOfDay::Night).radiusCells, carriedLightFor(TimeOfDay::Dusk).radiusCells)
        << "黄昏那团光小一些";
}

TEST(MapVisual, ParticleRatesScaleWithDensityAndArea) {
    using fanren::game::particleRate;
    const float one = particleRate(ParticleKind::Dust, 0.25f, 1.f);
    EXPECT_GT(one, 0.f);
    EXPECT_FLOAT_EQ(particleRate(ParticleKind::Dust, 0.5f, 1.f), one * 2.f) << "浓一倍，生一倍";
    EXPECT_FLOAT_EQ(particleRate(ParticleKind::Dust, 0.25f, 3.f), one * 3.f)
        << "铺在三块屏幕那么大的图上，屏幕上才看起来一样密";
    EXPECT_FLOAT_EQ(particleRate(ParticleKind::Dust, 0.f, 3.f), 0.f);
    EXPECT_FLOAT_EQ(particleRate(ParticleKind::Dust, -1.f, 3.f), 0.f);
}

TEST(MapVisual, FlickerStaysAroundOneAndSteadyLightsDoNotFlicker) {
    using fanren::game::flickerFactor;
    for (float t = 0.f; t < 5.f; t += 0.07f) {
        EXPECT_FLOAT_EQ(flickerFactor(0.f, t, 1.3f), 1.f);
        const float f = flickerFactor(0.4f, t, 1.3f);
        EXPECT_GE(f, 0.8f - 1e-5f);
        EXPECT_LE(f, 1.2f + 1e-5f);
    }
}

}  // namespace
