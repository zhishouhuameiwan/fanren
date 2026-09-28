#include "engine/PostFx.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace fanren::engine {
namespace {

// 过渡带最窄一个像素：再窄就是除以零，也画不出插值。
constexpr float kDofMinRamp = 1.f / static_cast<float>(kLogicalHeight);

// 光斑纹理的边长。它会被拉伸到几十到几百像素，线性取样下 128 已经看不出台阶。
constexpr int kLightSpriteSize = 128;

// 一盏灯最多叠画几遍（intensity 的上限）。再亮就只剩一片死白，没有意义。
constexpr int kMaxLightPasses = 4;

// 暗角从离中心多远开始（对角线长度的比例）。中间这一片完全不压，
// 人物与对白框所在的区域不该被暗角蚕食。
constexpr float kVignetteInner = 0.42f;

// 辉光逐级往上叠时每一级的权重。小于 1：越糊的那一级越淡，光晕由内向外衰减，
// 而不是在最外圈堆出一道亮环。
constexpr float kBloomUpWeight = 0.85f;

float clamp01(float v) noexcept {
    return std::clamp(v, 0.f, 1.f);
}

float smoothstep01(float t) noexcept {
    const float x = clamp01(t);
    return x * x * (3.f - 2.f * x);
}

std::uint8_t toByte(float v01) noexcept {
    return static_cast<std::uint8_t>(clamp01(v01) * 255.f + 0.5f);
}

bool isWhite(const Color& c) noexcept {
    return c.r == 255 && c.g == 255 && c.b == 255;
}

bool isBlack(const Color& c) noexcept {
    return c.r == 0 && c.g == 0 && c.b == 0;
}

Color opaque(const Color& c) noexcept {
    return Color{c.r, c.g, c.b, 255};
}

RectF fullScreen() noexcept {
    return RectF{0.f, 0.f, static_cast<float>(kLogicalWidth), static_cast<float>(kLogicalHeight)};
}

RectF whole() noexcept {
    return RectF{};   // 宽高为 0 = 整张纹理
}

}  // namespace

float dofBlurAlpha(float y, float focus, float band, float ramp) noexcept {
    const float r = std::max(ramp, kDofMinRamp);
    const float distance = std::fabs(y - focus);
    return smoothstep01((distance - (band - r * 0.5f)) / r);
}

void dofBandRows(float focus, float band, float ramp, std::vector<DofRow>& rows) {
    constexpr float kSameRow = 1e-6f;
    rows.clear();
    const float r = std::max(ramp, kDofMinRamp);
    const auto row = [&](float y) { return DofRow{y, dofBlurAlpha(y, focus, band, ramp)}; };
    // 两端各一行，恰好是 0 与 1：最上与最下的四边形必须贴到屏幕边。
    // 过渡带的切点由浮点算出来（0.8 + 0.2 会是 0.99999994），贴着两端的那几个
    // 一律并进端点，不然去重时留下的是「差一点点到底」的那一行。
    rows.push_back(row(0.f));
    rows.push_back(row(1.f));
    // 只在两条过渡带里细切；带内（全清）与带外（全糊）是平的，两端各一行就够。
    for (const float edge : {focus - band, focus + band}) {
        for (int i = 0; i <= kDofRampSteps; ++i) {
            const float y =
                edge - r * 0.5f + r * static_cast<float>(i) / static_cast<float>(kDofRampSteps);
            if (y > kSameRow && y < 1.f - kSameRow) {
                rows.push_back(row(y));
            }
        }
    }
    std::sort(rows.begin(), rows.end(),
              [](const DofRow& a, const DofRow& b) { return a.y < b.y; });
    // 两条过渡带挨得很近时会切出重合的行，零高度的四边形只是白画。
    rows.erase(std::unique(rows.begin(), rows.end(),
                           [](const DofRow& a, const DofRow& b) {
                               return std::fabs(a.y - b.y) < kSameRow;
                           }),
               rows.end());
}

float lightFalloff(float d) noexcept {
    if (d >= 1.f) {
        return 0.f;
    }
    const float q = 1.f - d * d;
    return q * q;
}

float vignetteAlpha(float nx, float ny) noexcept {
    // 除以 √2：四角（±1, ±1）处半径恰为 1。
    const float radius = std::sqrt(nx * nx + ny * ny) / std::sqrt(2.f);
    return smoothstep01((radius - kVignetteInner) / (1.f - kVignetteInner));
}

Point downsampleSize(Point full, int level) noexcept {
    return Point{std::max(1, full.x >> level), std::max(1, full.y >> level)};
}

std::vector<std::uint32_t> radialLightPixels(int size) {
    std::vector<std::uint32_t> pixels(static_cast<std::size_t>(size) * static_cast<std::size_t>(size));
    const float half = static_cast<float>(size) * 0.5f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            // 取像素中心：左右、上下两半严格对称。
            const float dx = (static_cast<float>(x) + 0.5f - half) / half;
            const float dy = (static_cast<float>(y) + 0.5f - half) / half;
            const float a = lightFalloff(std::sqrt(dx * dx + dy * dy));
            pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(size) +
                   static_cast<std::size_t>(x)] = packRgba(Color{255, 255, 255, toByte(a)});
        }
    }
    return pixels;
}

std::vector<std::uint32_t> vignettePixels(int w, int h) {
    std::vector<std::uint32_t> pixels(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    const float hw = static_cast<float>(w) * 0.5f;
    const float hh = static_cast<float>(h) * 0.5f;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float nx = (static_cast<float>(x) + 0.5f - hw) / hw;
            const float ny = (static_cast<float>(y) + 0.5f - hh) / hh;
            pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                   static_cast<std::size_t>(x)] = packRgba(Color{0, 0, 0, toByte(vignetteAlpha(nx, ny))});
        }
    }
    return pixels;
}

// ---------------------------------------------------------------------------

PostFx::PostFx(Engine& engine) : engine_(engine) {}

PostFx::~PostFx() = default;

bool PostFx::ensureResources() {
    if (tried_) {
        return ready_;
    }
    tried_ = true;
    const Point full{kLogicalWidth, kLogicalHeight};
    const auto target = [this](Point size) {
        return OwnedTexture(engine_, engine_.createRenderTarget(size.x, size.y, ScaleMode::Linear));
    };
    scene_ = target(full);
    if (!scene_.valid()) {
        return false;   // 无头模式在这里就停：后面那些图连像素都不必算
    }
    emissive_ = target(full);
    blur_ = target(full);
    light_ = target(downsampleSize(full, 1));
    bool chainReady = true;
    for (std::size_t i = 0; i < chain_.size(); ++i) {
        chain_[i] = target(downsampleSize(full, static_cast<int>(i) + 1));
        chainReady = chainReady && chain_[i].valid();
    }
    snapshotBlur_ = target(downsampleSize(full, 2));
    lightSprite_ = OwnedTexture(
        engine_, engine_.createTexture(kLightSpriteSize, kLightSpriteSize,
                                       radialLightPixels(kLightSpriteSize), ScaleMode::Linear));
    const Point vignetteSize = downsampleSize(full, 2);
    vignette_ = OwnedTexture(
        engine_, engine_.createTexture(vignetteSize.x, vignetteSize.y,
                                       vignettePixels(vignetteSize.x, vignetteSize.y),
                                       ScaleMode::Linear));
    ready_ = emissive_.valid() && blur_.valid() && light_.valid() && chainReady &&
             snapshotBlur_.valid() && lightSprite_.valid() && vignette_.valid();
    return ready_;
}

void PostFx::beginScene() {
    lights_.clear();
    emissiveDrawn_ = false;
    if (!ensureResources()) {
        inScene_ = false;
        return;
    }
    // 没有 endScene 就又 beginScene（上一段提前 return 了之类）：这时绑着的是 sceneRT 自己，
    // 记成输出目标就会把场景合成回它自己。保留第一次记下的那个。
    if (!inScene_) {
        output_ = engine_.renderTarget();
    }
    engine_.setRenderTarget(scene_.get());
    engine_.clear(Color{0, 0, 0, 255});
    inScene_ = true;
}

bool PostFx::beginEmissive() {
    // 精简档没有辉光（docs/settings.md 4.3）：照「没有辉光通道」答，各场景本来就按 false 不画发光体。
    if (!inScene_ || engine_.effectsLevel() == EffectsLevel::Lite) {
        return false;
    }
    engine_.setRenderTarget(emissive_.get());
    if (!emissiveDrawn_) {
        // 清成**不透明**黑：发光体按 Alpha 画进来后 alpha 仍是满的，
        // 之后按 Add 叠回场景时颜色只乘一次透明度（清成全透明会乘两次，光晕边缘发灰）。
        engine_.clear(Color{0, 0, 0, 255});
        emissiveDrawn_ = true;
    }
    return true;
}

void PostFx::endEmissive() {
    if (!inScene_) {
        return;
    }
    engine_.setRenderTarget(scene_.get());
}

void PostFx::addLight(const Light& light) {
    lights_.push_back(light);
}

void PostFx::resample(TextureId src, TextureId dst) {
    const Point size = engine_.textureSize(dst);
    engine_.setRenderTarget(dst);
    DrawOptions copy;
    copy.blend = BlendMode::None;
    engine_.drawTexture(src, whole(),
                        RectF{0.f, 0.f, static_cast<float>(size.x), static_cast<float>(size.y)},
                        copy);
}

void PostFx::accumulate(TextureId src, TextureId dst, float weight) {
    const Point size = engine_.textureSize(dst);
    engine_.setRenderTarget(dst);
    DrawOptions add;
    add.blend = BlendMode::Add;
    add.tint.a = toByte(weight);
    engine_.drawTexture(src, whole(),
                        RectF{0.f, 0.f, static_cast<float>(size.x), static_cast<float>(size.y)},
                        add);
}

void PostFx::applyLighting(const PostFxSettings& settings) {
    // 环境光纯白时光照图处处 ≥ 1，乘回去是恒等变换——光源再亮也没有可提亮的余地。
    if (isWhite(settings.ambient)) {
        return;
    }
    const Point lightSize = engine_.textureSize(light_.get());
    const float sx = static_cast<float>(lightSize.x) / static_cast<float>(kLogicalWidth);
    const float sy = static_cast<float>(lightSize.y) / static_cast<float>(kLogicalHeight);

    engine_.setRenderTarget(light_.get());
    engine_.clear(opaque(settings.ambient));
    for (const Light& light : lights_) {
        if (light.radius <= 0.f || light.intensity <= 0.f) {
            continue;
        }
        const RectF dst{(light.x - light.radius) * sx, (light.y - light.radius) * sy,
                        2.f * light.radius * sx, 2.f * light.radius * sy};
        DrawOptions glow;
        glow.blend = BlendMode::Add;
        float remaining = std::min(light.intensity, static_cast<float>(kMaxLightPasses));
        while (remaining > 0.f) {
            glow.tint = Color{light.color.r, light.color.g, light.color.b,
                              toByte(std::min(remaining, 1.f))};
            engine_.drawTexture(lightSprite_.get(), whole(), dst, glow);
            remaining -= 1.f;
        }
    }

    engine_.setRenderTarget(scene_.get());
    DrawOptions modulate;
    modulate.blend = BlendMode::Mod;
    engine_.drawTexture(light_.get(), whole(), fullScreen(), modulate);
}

void PostFx::applyBloom(float strength) {
    resample(emissive_.get(), chain_.front().get());
    for (std::size_t i = 1; i < chain_.size(); ++i) {
        resample(chain_[i - 1].get(), chain_[i].get());
    }
    // 由糊到清逐级放大叠回上一级：1/64 → 1/32 → … → 1/2，最后一次性加回场景。
    // 与「每一级各自放大到全屏再加一遍」贡献的是同一组光晕，但放大是一级一级的
    // （每次两倍，线性插值看不出台阶），全屏的那一遍也只剩一次。
    for (std::size_t i = chain_.size() - 1; i > 0; --i) {
        accumulate(chain_[i].get(), chain_[i - 1].get(), kBloomUpWeight);
    }
    engine_.setRenderTarget(scene_.get());
    DrawOptions add;
    add.blend = BlendMode::Add;
    add.tint.a = toByte(strength);
    engine_.drawTexture(chain_.front().get(), whole(), fullScreen(), add);
}

void PostFx::prepareDofBlur() {
    // 降到 1/4 再一级一级放大回原尺寸。直接从 1/4 拉回全屏会看出双线性插值的菱形格；
    // 放大回原尺寸的另一个原因是景深带按 1:1 取样——软件渲染器画带顶点色的
    // 三角形时只做最近邻取样，拿 1/4 的图去画会是一块块马赛克（截图验收用的正是它）。
    const TextureId half = chain_[0].get();
    const TextureId quarter = chain_[1].get();
    engine_.setTextureScaleMode(scene_.get(), ScaleMode::Linear);
    resample(scene_.get(), half);
    resample(half, quarter);
    resample(quarter, half);
    resample(half, blur_.get());
}

void PostFx::drawDofBands(const PostFxSettings& settings) {
    dofBandRows(settings.dofFocus, settings.dofBand, settings.dofRamp, dofRows_);
    dofVertices_.clear();
    dofIndices_.clear();
    const float w = static_cast<float>(kLogicalWidth);
    const float h = static_cast<float>(kLogicalHeight);
    for (std::size_t i = 0; i + 1 < dofRows_.size(); ++i) {
        const DofRow& top = dofRows_[i];
        const DofRow& bottom = dofRows_[i + 1];
        if (top.alpha <= 0.f && bottom.alpha <= 0.f) {
            continue;   // 清晰带：一个像素都不盖
        }
        const Color ct{255, 255, 255, toByte(top.alpha * settings.dof)};
        const Color cb{255, 255, 255, toByte(bottom.alpha * settings.dof)};
        const int base = static_cast<int>(dofVertices_.size());
        dofVertices_.push_back(Vertex{0.f, top.y * h, ct, 0.f, top.y});
        dofVertices_.push_back(Vertex{w, top.y * h, ct, 1.f, top.y});
        dofVertices_.push_back(Vertex{w, bottom.y * h, cb, 1.f, bottom.y});
        dofVertices_.push_back(Vertex{0.f, bottom.y * h, cb, 0.f, bottom.y});
        for (const int k : {0, 1, 2, 0, 2, 3}) {
            dofIndices_.push_back(base + k);
        }
    }
    if (!dofVertices_.empty()) {
        engine_.drawGeometry(blur_.get(), dofVertices_, dofIndices_, BlendMode::Alpha);
    }
}

void PostFx::endScene(const PostFxSettings& settings) {
    if (!inScene_) {
        return;
    }
    inScene_ = false;

    // 画面特效档位只在这里（与 beginEmissive）读，全工程单点（docs/settings.md 4.3）：精简 = 跳过辉光与景深；
    // 光照、暗角、调色照做——便宜，而且夜景的昼夜感靠光照。辉光不必在这里再判一次：精简档下 beginEmissive
    // 答 false，emissiveDrawn_ 就不会是真。
    applyLighting(settings);
    if (settings.bloom > 0.f && emissiveDrawn_) {
        applyBloom(settings.bloom);
    }
    const bool dof = engine_.effectsLevel() == EffectsLevel::Full && settings.dof > 0.f;
    if (dof) {
        prepareDofBlur();
    }

    engine_.setRenderTarget(output_);
    // 场景图贴回去用像素取样：窗口按整数倍放大时（2560×1440 之类）像素美术
    // 仍是锐利的方块，而不是被双线性糊成一片。
    engine_.setTextureScaleMode(scene_.get(), ScaleMode::Pixel);
    DrawOptions copy;
    copy.blend = BlendMode::None;
    engine_.drawTexture(scene_.get(), whole(), fullScreen(), copy);

    if (dof) {
        drawDofBands(settings);
    }
    if (settings.vignette > 0.f) {
        DrawOptions shade;
        shade.tint.a = toByte(settings.vignette);
        engine_.drawTexture(vignette_.get(), whole(), fullScreen(), shade);
    }
    if (!isWhite(settings.gradeMul)) {
        engine_.fillRect(fullScreen(), opaque(settings.gradeMul), BlendMode::Mod);
    }
    if (!isBlack(settings.gradeAdd)) {
        engine_.fillRect(fullScreen(), opaque(settings.gradeAdd), BlendMode::Add);
    }
    lights_.clear();
    emissiveDrawn_ = false;
}

TextureId PostFx::snapshot() {
    snapshot_ = OwnedTexture(engine_, engine_.snapshotBackbuffer());
    if (!snapshot_.valid() || !ensureResources()) {
        return snapshot_.get();
    }
    // 模糊版走与景深同一条降采样链，只是停在 1/4 那一级：它要被整屏拉伸着当底图，
    // 糊一点正好。链上的图在 endScene 里已经用完，这里借来用不冲突。
    const TextureId previous = engine_.renderTarget();
    resample(snapshot_.get(), chain_[0].get());
    resample(chain_[0].get(), chain_[1].get());
    resample(chain_[1].get(), chain_[2].get());
    resample(chain_[2].get(), snapshotBlur_.get());
    engine_.setRenderTarget(previous);
    return snapshot_.get();
}

TextureId PostFx::blurredSnapshot() const {
    return snapshot_.valid() && ready_ ? snapshotBlur_.get() : kInvalidTexture;
}

}  // namespace fanren::engine
