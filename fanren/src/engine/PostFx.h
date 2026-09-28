#pragma once
// HD-2D 后处理管线（docs/octopath-overhaul.md 1.4 节）。
//
//   场景 → sceneRT（1280×720）
//     ├─ 光照：lightRT 填环境色，点光源径向图 Add 叠上去，再以 Mod 乘回 sceneRT
//     ├─ 辉光：emissiveRT（只画发光体）→ 1/2 → 1/4 → … → 1/64 线性降采样 → 逐级 Add 回去
//     ├─ 景深：sceneRT → 降采样模糊图；上下两条渐变带（顶点 alpha）盖回去 = 移轴
//     └─ 暗角 + 调色（Mod / Add 各一层）
//   → 后缓冲 → UI（清晰，不进后处理）
//
// 辉光的链比施工图写的「降三级」深：线性降采样是 2×2 平均、线性放大只插到相邻的
// 那个像素中心，所以每一级只把光往外推「一个本级像素」。降到 1/8 为止，光晕只比
// 发光体大十几个像素，灯笼看起来是「亮」而不是「泛光」；降到 1/64，最外一级的一个
// 像素就是 64 个屏幕像素，光晕才铺得开。多出来的三级一共不到两万个像素，几乎不花钱。
//
// 本模块只用 Engine 的公开接口，一个 SDL 头都不 include：后处理是「画法」，
// 不是平台层。这样它不必挤进 Engine::Impl，将来换平台层也不用重写它。
//
// 用法（每帧）：
//   fx.beginScene();                 // 切到 sceneRT、清屏
//   ... 画世界 ...
//   if (fx.beginEmissive()) {        // 可选：发光体另画一份进 emissiveRT
//       ... 画灯笼的光晕、萤火 ...
//       fx.endEmissive();
//   }
//   fx.addLight({x, y, radius, color, intensity});   // 若干
//   fx.endScene(settings);           // 合成回 beginScene 时的目标（通常是后缓冲）
//   ... 画 UI ...
//
// 无头模式（或建不出渲染目标时）整条管线是空操作：beginScene 不切目标，世界照常
// 直接画在当前目标上，endScene 什么也不做——画面少了后处理，但一切照跑。
#include <array>
#include <cstdint>
#include <vector>

#include "engine/Engine.h"

namespace fanren::engine {

// 一帧后处理的全部参数。缺省值 = 什么都不做（白环境光、零强度、恒等调色）。
// 参数的出处是地图视觉预设（meta.json）与战斗场景各自的一套。
struct PostFxSettings {
    Color ambient{255, 255, 255};   // 环境光。白 = 不做光照；夜里压成暗蓝，光源再把局部提亮
    float dof = 0.f;                // 景深强度 [0,1]：带外那一截盖上多少模糊图
    float dofFocus = 0.52f;         // 清晰带中心（屏幕高度比例，0 顶 1 底）
    float dofBand = 0.34f;          // 清晰带半宽（量到过渡带正中，那里恰好半糊）
    float bloom = 0.f;              // 辉光强度，通常 [0,1]，大于 1 按 1 算
    float vignette = 0.f;           // 暗角强度 [0,1]：四角压暗的程度
    Color gradeMul{255, 255, 255};  // 调色：整屏乘这个颜色（Mod）
    Color gradeAdd{0, 0, 0};        // 调色：整屏加这个颜色（Add）
    // 以下为对契约的追加（追加在末尾，按字段顺序的聚合初始化不受影响）：
    // 过渡带全宽（屏幕高度比例）。清晰与全糊之间的那一段，越宽越柔。
    float dofRamp = 0.15f;
};

// 点光源。坐标是屏幕像素（与画世界用的是同一套坐标）。
struct Light {
    float x{}, y{};
    float radius{};                 // 光照所及的半径（像素），边缘衰减到零
    Color color{255, 255, 255};
    float intensity = 1.f;          // 1 = 圆心处加上一整份 color；大于 1 叠画多遍（上限 4）
};

// ---------------------------------------------------------------------------
// 纯函数：曲线与尺寸。抽出来是为了不开窗口就能单测——管线本身画在显存里，
// 看不见摸不着，而「景深带是不是上下对称、带内是不是一点都不糊」这种事，
// 错了只会在截图上隐约显出来。
// ---------------------------------------------------------------------------

// 景深的模糊程度 [0,1]（0 = 全清，1 = 全糊），y 是屏幕高度比例。
// 以 focus 为中心、离中心 band 处恰为 0.5，过渡带宽 ramp，带里 smoothstep 过渡：
//   |y-focus| ≤ band - ramp/2 → 0；  ≥ band + ramp/2 → 1。
// ramp 小于一个像素按一个像素算（硬边也要有个宽度，不然插值无从谈起）。
[[nodiscard]] float dofBlurAlpha(float y, float focus, float band, float ramp) noexcept;

// 景深带的几何行：若干条横线（y 为屏幕高度比例，alpha 为该处的模糊程度），
// 相邻两行之间画一条竖向渐变的四边形，就把 dofBlurAlpha 的曲线分段线性地画出来。
// 行从 y=0 排到 y=1；两条过渡带各切 kDofRampSteps 段，平的地方不多切。
struct DofRow {
    float y = 0.f;
    float alpha = 0.f;
};
inline constexpr int kDofRampSteps = 8;
void dofBandRows(float focus, float band, float ramp, std::vector<DofRow>& rows);

// 点光源的衰减：d 是到圆心的距离与半径之比。(1-d²)²：圆心 1、边缘 0，
// 且边缘处导数也是 0——光圈外沿不会出现一道看得见的环。
[[nodiscard]] float lightFalloff(float d) noexcept;

// 暗角的不透明度 [0,1]：nx、ny 是到屏幕中心的归一化偏移（左右边缘 ±1、上下边缘 ±1）。
// 中心一大片完全透明，往四角平滑地加深，四角为 1。按屏幕比例拉成椭圆，所以左右
// 两边与上下两边压暗的程度一样。
[[nodiscard]] float vignetteAlpha(float nx, float ny) noexcept;

// 第 level 级降采样的尺寸（每级减半，不小于 1×1）。level 0 是原尺寸。
[[nodiscard]] Point downsampleSize(Point full, int level) noexcept;

// 降采样链有几级：1/2、1/4 … 1/64。辉光用满，景深用前两级，快照模糊用前三级。
inline constexpr int kDownsampleLevels = 6;

// 运行时生成的两张图（packRgba 格式）：
//   光斑：size×size，白色，alpha = lightFalloff(到中心的距离 / 半径)；
//   暗角：w×h，黑色，alpha = vignetteAlpha。
[[nodiscard]] std::vector<std::uint32_t> radialLightPixels(int size);
[[nodiscard]] std::vector<std::uint32_t> vignettePixels(int w, int h);

// ---------------------------------------------------------------------------
// 管线本体
// ---------------------------------------------------------------------------
class PostFx {
public:
    // 渲染目标在第一次 beginScene（或 snapshot）时才建，之后一直复用；
    // 构造本身不碰显存，所以无头测试、还没 init 的引擎都可以先构造它。
    // 必须在 engine 之前析构（它持有的渲染目标要还给 engine）。
    explicit PostFx(Engine& engine);
    ~PostFx();
    PostFx(const PostFx&) = delete;
    PostFx& operator=(const PostFx&) = delete;
    PostFx(PostFx&&) = delete;
    PostFx& operator=(PostFx&&) = delete;

    // 切到 sceneRT 并清成不透明黑。记下当时绑着的目标，endScene 合成回那里。
    void beginScene();

    // 之后的绘制进 emissiveRT（辉光的素材），直到 endEmissive 切回 sceneRT。
    // 一帧里可以进出多次，第一次进来时清屏。返回 false 表示本帧没有辉光通道
    // （无头、没有 beginScene、建不出渲染目标、画面特效是精简档）——调用方这时**别画发光体**，
    // 否则它们会直接画在场景上。精简档下 endScene 也跳过辉光与景深（Engine::effectsLevel）。
    [[nodiscard]] bool beginEmissive();
    void endEmissive();

    // 登记一盏本帧的点光源。只在 ambient 不是纯白时起作用（纯白 = 本来就全亮）。
    void addLight(const Light& light);

    // 做完全部后处理，把结果合成回 beginScene 时的目标，并切到那里。
    // 之后画的东西（UI）不受后处理影响。没有 beginScene 的帧里调它是空操作。
    void endScene(const PostFxSettings& settings);

    // 把后缓冲当前的画面拷一份（开战碎屏转场、菜单背景模糊用），同时备好一张模糊版。
    // 必须在 endFrame 之前调。返回的纹理归 PostFx 所有：下一次 snapshot 会顶掉它，
    // PostFx 析构时释放。无头模式返回 kInvalidTexture。
    [[nodiscard]] TextureId snapshot();
    [[nodiscard]] TextureId snapshotTexture() const { return snapshot_.get(); }
    // 最近一次 snapshot 的模糊版（1/4 尺寸、线性取样，整屏拉伸着画就是一张柔焦的底）。
    [[nodiscard]] TextureId blurredSnapshot() const;

    // 渲染目标建成了没有。第一次 beginScene / snapshot 之后才有答案；无头模式恒为 false。
    [[nodiscard]] bool active() const { return ready_; }

private:
    bool ensureResources();
    void resample(TextureId src, TextureId dst);
    void accumulate(TextureId src, TextureId dst, float weight);
    void applyLighting(const PostFxSettings& settings);
    void applyBloom(float strength);
    void prepareDofBlur();
    void drawDofBands(const PostFxSettings& settings);

    Engine& engine_;
    bool tried_ = false;
    bool ready_ = false;
    bool inScene_ = false;
    bool emissiveDrawn_ = false;
    TextureId output_ = kInvalidTexture;

    OwnedTexture scene_;         // 1/1：世界画在这里，后处理也在这里做
    OwnedTexture emissive_;      // 1/1：只画发光体
    OwnedTexture blur_;          // 1/1：景深用的模糊图（放大回原尺寸，景深带按 1:1 取样）
    OwnedTexture light_;         // 1/2：光照图（光照本来就是低频的，半尺寸看不出差别）
    // 降采样链 1/2 … 1/64。辉光、景深、快照模糊轮流用（同一帧里先后用，互不重叠）。
    std::array<OwnedTexture, kDownsampleLevels> chain_;
    OwnedTexture snapshotBlur_;  // 1/4：快照的模糊版
    OwnedTexture lightSprite_;   // 光斑（径向渐变）
    OwnedTexture vignette_;      // 暗角
    OwnedTexture snapshot_;

    std::vector<Light> lights_;
    // 景深几何的缓冲，每帧复用，不重新分配。
    std::vector<DofRow> dofRows_;
    std::vector<Vertex> dofVertices_;
    std::vector<int> dofIndices_;
};

}  // namespace fanren::engine
