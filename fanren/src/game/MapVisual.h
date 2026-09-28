#pragma once
// 一张地图「长什么样」的那几个参数：烘焙附带的 meta.json（A1 路 tools/artgen/maplights.py 写），
// 以及烘焙图能不能用的判据。
//
// meta.json 的字段（坐标一律以格为单位，格中心 = x + 0.5）：
//   time      day | dusk | night | indoor          时辰预设
//   theme     village / town / cave / indoor_wood … 主题（只在 time 缺失时用来推时辰）
//   ambient   [r, g, b]                            环境光
//   dof / bloom / vignette                         景深、辉光、暗角的强度 [0,1]
//   particles [{kind, rate}]                       整张图的氛围粒子；rate 是 0..1 的浓淡
//   lights    [{x, y, r, color:[r,g,b], intensity, flicker, kind}]
//   water     [[x, y], …]                          水面格（运行时加波光）
//   emitters  [{kind, x, y}]                       定点发射：火盆的火星、水面的雾、水边的萤火
//   backdrop  "manor_night"                        这张图上开打时的战斗背景（世界画面不用，战斗画面取）
// 缺字段按时辰给缺省（时辰缺了按主题推），数都与烘焙器那张 TIME_DEFAULTS 同一套——
// 两边任何一边改了缺省，另一边的画面就对不上烘焙时的取景。
//
// 读在 io 层（io/VisualLoader.h 的 loadMapMeta）：认不出的时辰、认不出的粒子种类、形状不对的数组
// 一律报错——生成器只会写出那几种写法，写出别的就是有人手改坏了文件，悄悄按缺省画只会让
// 「夜景怎么成了白天」变成一个没人查得出的谜。读出来的 core::MapMeta 只有文件里写了的东西，
// 这里（mapVisualFrom）按时辰补缺省、换成引擎的颜色与粒子种类。
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/model/Visual.h"
#include "engine/Engine.h"
#include "engine/Particles.h"
#include "engine/PostFx.h"

namespace fanren::game {

// 时辰的枚举与 io 层读出来的是同一个。
using core::TimeOfDay;

struct MapLight {
    float x = 0.f;              // 格坐标
    float y = 0.f;
    float radius = 3.f;         // 光照半径（格）
    engine::Color color{255, 214, 160, 255};
    float intensity = 1.f;
    float flicker = 0.f;        // 0 = 稳定；0.4 = 火把那样跳
    std::string kind;           // lantern / window / furnace / torch / candle / sconce / save …
};

struct ParticleLayer {
    engine::ParticleKind kind = engine::ParticleKind::Dust;
    float density = 0.f;        // meta 里的 rate：0..1 的浓淡，不是每秒几颗（换算见 particleRate）
};

struct ParticleEmitter {
    engine::ParticleKind kind = engine::ParticleKind::Ember;
    float x = 0.f;              // 格坐标
    float y = 0.f;
};

struct MapVisual {
    std::string theme;
    TimeOfDay time = TimeOfDay::Day;
    engine::Color ambient{255, 255, 255, 255};
    float dof = 0.f;
    float bloom = 0.f;
    float vignette = 0.f;
    std::vector<ParticleLayer> particles;
    std::vector<MapLight> lights;
    std::vector<core::Point> water;
    std::vector<ParticleEmitter> emitters;
};

// 某个时辰的缺省：环境光、景深、辉光、暗角（与烘焙器 maplights.py 的 TIME_DEFAULTS 逐项相同），
// 不带灯、不带粒子。
[[nodiscard]] MapVisual visualDefaults(TimeOfDay time);

// meta.json 整个没有时（烘焙还没到这张图）：室外按昼、室内按室内。
[[nodiscard]] MapVisual fallbackVisual(bool outdoor);

// io 层读出来的 meta → 这一张图的画面参数：从 visualDefaults(meta.time) 起，文件里写了的覆盖上去。
[[nodiscard]] MapVisual mapVisualFrom(const core::MapMeta& meta);

// 这一帧的后处理参数：meta 给的环境光与三样强度 + 按时辰的调色 + 景深对焦在主角身上。
// focusY 是主角在屏幕上的高度比例（0 顶 1 底）。
[[nodiscard]] engine::PostFxSettings postFxFor(const MapVisual& visual, float focusY);

// 主角身上那团随行微光（施工图第 3 节：夜里、室内）。黄昏也点，小一些。
struct CarriedLight {
    bool on = false;
    float radiusCells = 0.f;
    engine::Color color{255, 255, 255, 255};
    float intensity = 0.f;
};
[[nodiscard]] CarriedLight carriedLightFor(TimeOfDay time);

// 烘焙图能不能用：载得进来（尺寸不是 0），而且恰好是「地图格数 × 16」。
//
// 尺寸对不上说明烘焙与地图脱了节（地图生成器改了图、烘焙没跟上）：贴上去就是整张图错位，
// 人站在屋顶上、墙画在路中间——那比退回旧画法难看得多，也难查得多。所以这一条也算「不能用」。
[[nodiscard]] bool bakedLayerUsable(engine::Point textureSize, int mapWidthCells, int mapHeightCells);

// 粒子浓淡 → 每秒生成数。areaScreens 是发射区的面积相当于几块屏幕（1280×720）：
// 同样的浓淡，铺满整张大图的尘埃要按面积多生，屏幕上看起来才一样密。
[[nodiscard]] float particleRate(engine::ParticleKind kind, float density, float areaScreens);

// 光源闪烁的乘数：以 1 为中心、幅度 flicker/2 平滑地跳。phase 让每盏灯各跳各的。
[[nodiscard]] float flickerFactor(float flicker, float seconds, float phase);

}  // namespace fanren::game
