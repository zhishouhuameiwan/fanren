#include "engine/Particles.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <numbers>
#include <span>
#include <string>

namespace fanren::engine {
namespace {

constexpr float kTwoPi = 2.f * std::numbers::pi_v<float>;

// prewarm 的步长。与帧率无关的固定步长，prewarm 出来的稳态因此是确定的。
constexpr float kPrewarmStep = 1.f / 30.f;

// 雨丝的长度 = 速度 × 这么多秒。约等于人眼在一帧的视觉暂留里看到的那一截。
constexpr float kRainStreakSeconds = 0.022f;

// 粒子从 area 外多远处进场、出场（除了粒子自身的大小与摆幅之外再留的余量）。
constexpr float kEdgeSlack = 8.f;

enum class Spawn { Inside, Top, Bottom };

// 每种粒子的手感。数值是照着「16px 像素世界 ×3、1280×720」这个画幅调的：
// 速度是屏幕像素/秒，大小是屏幕像素。
struct KindSpec {
    Spawn spawn;
    float lifeMin, lifeMax;          // 秒（Top 类按穿过 area 的路程另算）
    float vxMin, vxMax, vyMin, vyMax;
    float sizeMin, sizeMax;          // 像素贴图类只用来算进出场余量与裁剪
    float swayMin, swayMax;          // 左右摆动的幅度（像素）
    float swayFreqMin, swayFreqMax;  // 摆动频率（赫兹）
    float spinMin, spinMax;          // 旋转速度的大小（度/秒），方向随机
    float fadeIn, fadeOut;           // 生命期比例
    float alpha;                     // 基础不透明度
};

// 顺序必须与 ParticleKind 一致（下面 static_assert 管着个数，顺序靠这张表的注释对齐）。
constexpr std::array<KindSpec, 8> kSpecs{{
    // Dust：几乎不动，只是在光里慢慢浮着
    {Spawn::Inside, 6.f, 10.f, -6.f, 6.f, -6.f, 3.f, 5.f, 10.f, 5.f, 12.f, 0.10f, 0.30f,
     0.f, 0.f, 0.25f, 0.35f, 0.60f},
    // Firefly：绕着小圈游走
    {Spawn::Inside, 5.f, 9.f, -10.f, 10.f, -6.f, 6.f, 14.f, 22.f, 14.f, 26.f, 0.15f, 0.35f,
     0.f, 0.f, 0.20f, 0.30f, 0.95f},
    // Petal：轻，飘得慢、摆得多、转得快
    {Spawn::Top, 0.f, 0.f, 10.f, 26.f, 30.f, 48.f, 16.f, 16.f, 16.f, 28.f, 0.40f, 0.80f,
     60.f, 180.f, 0.03f, 0.08f, 0.95f},
    // Leaf：比花瓣重一点、大一点
    {Spawn::Top, 0.f, 0.f, 12.f, 30.f, 38.f, 64.f, 22.f, 22.f, 20.f, 36.f, 0.30f, 0.60f,
     50.f, 150.f, 0.03f, 0.08f, 0.95f},
    // Rain：又快又斜，不摆不转
    {Spawn::Top, 0.f, 0.f, -170.f, -130.f, 760.f, 900.f, 24.f, 24.f, 0.f, 0.f, 0.f, 0.f,
     0.f, 0.f, 0.f, 0.f, 1.f},
    // Snow：慢、轻摆
    {Spawn::Top, 0.f, 0.f, -6.f, 10.f, 24.f, 48.f, 4.f, 8.f, 8.f, 18.f, 0.20f, 0.50f,
     0.f, 0.f, 0.03f, 0.08f, 0.85f},
    // Ember：往上冒，一两秒就熄
    {Spawn::Bottom, 1.4f, 3.0f, -10.f, 10.f, -90.f, -40.f, 5.f, 9.f, 4.f, 10.f, 0.80f, 1.60f,
     0.f, 0.f, 0.05f, 0.50f, 0.95f},
    // Mist：大、淡、慢，横着挪。夜里还要被光照乘暗一截，再淡就看不出来了。
    {Spawn::Inside, 14.f, 24.f, 5.f, 14.f, -2.f, 2.f, 200.f, 360.f, 0.f, 0.f, 0.f, 0.f,
     0.f, 0.f, 0.30f, 0.35f, 0.20f},
}};
static_assert(kSpecs.size() == static_cast<std::size_t>(ParticleKind::Mist) + 1,
              "kSpecs 要与 ParticleKind 一一对应");

const KindSpec& specOf(ParticleKind kind) {
    return kSpecs[static_cast<std::size_t>(kind)];
}

// 配色。像素贴图是灰阶的，颜色全靠这里上。
constexpr std::array<Color, 2> kDustColors{{{255, 238, 200, 255}, {255, 226, 170, 255}}};
constexpr std::array<Color, 3> kFireflyColors{
    {{205, 255, 130, 255}, {180, 250, 110, 255}, {225, 255, 160, 255}}};
constexpr std::array<Color, 3> kPetalColors{
    {{255, 196, 214, 255}, {250, 178, 200, 255}, {255, 224, 232, 255}}};
constexpr std::array<Color, 4> kLeafColors{
    {{122, 168, 84, 255}, {164, 176, 70, 255}, {201, 162, 72, 255}, {190, 120, 60, 255}}};
constexpr std::array<Color, 1> kRainColors{{{178, 196, 228, 120}}};
constexpr std::array<Color, 1> kSnowColors{{{240, 244, 255, 255}}};
constexpr std::array<Color, 1> kEmberColors{{{255, 200, 100, 255}}};
constexpr std::array<Color, 1> kMistColors{{{214, 222, 236, 255}}};

// 火星熄灭时的颜色：从橙黄烧到暗红。
constexpr Color kEmberDying{220, 70, 30, 255};

std::span<const Color> paletteOf(ParticleKind kind) {
    switch (kind) {
        case ParticleKind::Dust: return kDustColors;
        case ParticleKind::Firefly: return kFireflyColors;
        case ParticleKind::Petal: return kPetalColors;
        case ParticleKind::Leaf: return kLeafColors;
        case ParticleKind::Rain: return kRainColors;
        case ParticleKind::Snow: return kSnowColors;
        case ParticleKind::Ember: return kEmberColors;
        case ParticleKind::Mist: return kMistColors;
    }
    return kDustColors;
}

// 像素小图：叶与花瓣。L 亮面、M 中间、D 暗面、S 叶柄、. 透明。
// 灰阶而不是成品色：同一张图靠 tint 就能是青叶、黄叶、红叶。
constexpr std::array<const char*, 5> kLeaf0{{
    "...LL..",
    ".LLLLM.",
    "SDDDDDM",
    ".MMMMD.",
    "...MM..",
}};
constexpr std::array<const char*, 6> kLeaf1{{
    "....LL",
    "..LLLM",
    ".LLLDM",
    ".LDMM.",
    ".DMM..",
    "S.....",
}};
constexpr std::array<const char*, 7> kLeaf2{{
    "..L..",
    ".LLM.",
    ".LDM.",
    ".LDM.",
    ".LDM.",
    "..M..",
    "..S..",
}};
constexpr std::array<const char*, 4> kPetal0{{
    ".LLL.",
    "LLLLM",
    ".LMM.",
    "..M..",
}};
constexpr std::array<const char*, 4> kPetal1{{
    ".LL.",
    "LLLM",
    "LLMM",
    ".MD.",
}};

// pixelSprite 按第一行的宽度去读每一行：哪一行少打一个点就读过了字符串的结尾。
// 放在编译期查，改图时当场报错，而不是运行时读出一片垃圾像素。
template <std::size_t N>
constexpr bool sameWidthRows(const std::array<const char*, N>& rows) {
    const auto width = [](const char* s) {
        std::size_t n = 0;
        while (s[n] != '\0') {
            ++n;
        }
        return n;
    };
    for (const char* row : rows) {
        if (width(row) != width(rows[0])) {
            return false;
        }
    }
    return true;
}
static_assert(sameWidthRows(kLeaf0) && sameWidthRows(kLeaf1) && sameWidthRows(kLeaf2) &&
                  sameWidthRows(kPetal0) && sameWidthRows(kPetal1),
              "像素小图的每一行必须一样宽");

std::uint8_t shadeOf(char c) {
    switch (c) {
        case 'L': return 255;
        case 'M': return 214;
        case 'D': return 168;
        case 'S': return 128;
        default: return 0;
    }
}

SpriteImage pixelSprite(std::span<const char* const> rows) {
    SpriteImage img;
    img.w = static_cast<int>(std::strlen(rows.front()));
    img.h = static_cast<int>(rows.size());
    img.scale = ScaleMode::Pixel;
    img.pixels.reserve(static_cast<std::size_t>(img.w) * static_cast<std::size_t>(img.h));
    for (const char* row : rows) {
        for (int x = 0; x < img.w; ++x) {
            const char c = row[x];
            const std::uint8_t v = shadeOf(c);
            img.pixels.push_back(c == '.' ? 0u : packRgba(Color{v, v, v, 255}));
        }
    }
    return img;
}

// 圆形的程序光斑：alpha = falloff(到中心的距离 / 半径)，白色。
template <typename Falloff>
SpriteImage roundSprite(int size, Falloff falloff) {
    SpriteImage img;
    img.w = size;
    img.h = size;
    img.scale = ScaleMode::Linear;
    img.pixels.reserve(static_cast<std::size_t>(size) * static_cast<std::size_t>(size));
    const float half = static_cast<float>(size) * 0.5f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float dx = (static_cast<float>(x) + 0.5f - half) / half;
            const float dy = (static_cast<float>(y) + 0.5f - half) / half;
            const float d = std::sqrt(dx * dx + dy * dy);
            const float a = d >= 1.f ? 0.f : std::clamp(falloff(d), 0.f, 1.f);
            img.pixels.push_back(
                packRgba(Color{255, 255, 255, static_cast<std::uint8_t>(a * 255.f + 0.5f)}));
        }
    }
    return img;
}

// 光点：亮芯 + 很快衰减的晕。(1-d)^2.2 在 d=0.3 处约 0.46、d=0.6 处约 0.13。
SpriteImage glowSprite() {
    return roundSprite(32, [](float d) { return std::pow(1.f - d, 2.2f); });
}

// 柔圆：雪片、雾团。(1-d²)²，中间饱满、边缘柔和地收成零。
SpriteImage softSprite(int size) {
    return roundSprite(size, [](float d) {
        const float q = 1.f - d * d;
        return q * q;
    });
}

// 每种粒子去美术目录里找哪些文件：按顺序试，先找到的那一组算数（同一组里有几张用几张）。
// 文件名与画法照 assets/art/fx 的实际产物（tools/artgen/fx.py，docs/art-sprites.md）：
//   * 16px 尺度的像素小图（dust / firefly / embers / spark / leaves / petals / snow / rain）
//     按帧原尺寸 ×3 画，Pixel 取样；
//   * 柔图（glow / softcircle）按粒子大小拉伸，Linear 取样；
//   * 灰度或白色出图的运行时乘种类配色；embers 自带火色，不乘；
//   * embers、firefly 是明灭帧，按 fps 轮播；叶形、花瓣形、雪片大小是变体，每颗固定一帧。
// fog.png 是四边无缝的平铺雾纹，当粒子画会露出方形的边，薄雾仍用柔圆。
struct ArtChoice {
    const char* pattern;
    bool pixelArt;
    bool tinted;
    float fps;
};
using ArtChoices = std::array<ArtChoice, 3>;
constexpr ArtChoice kNoMoreChoices{nullptr, false, true, 0.f};

ArtChoices artChoicesOf(ParticleKind kind) {
    switch (kind) {
        case ParticleKind::Dust:
            return {{{"dust*.png", true, true, 0.f}, {"glow*.png", false, true, 0.f}, kNoMoreChoices}};
        case ParticleKind::Firefly:
            return {{{"firefly*.png", true, true, 3.f}, {"glow*.png", false, true, 0.f}, kNoMoreChoices}};
        case ParticleKind::Petal:
            return {{{"petal*.png", true, true, 0.f}, kNoMoreChoices, kNoMoreChoices}};
        case ParticleKind::Leaf:
            return {{{"leaves*.png", true, true, 0.f}, {"leaf*.png", true, true, 0.f}, kNoMoreChoices}};
        case ParticleKind::Rain:
            return {{{"rain*.png", true, true, 0.f}, kNoMoreChoices, kNoMoreChoices}};
        case ParticleKind::Snow:
            return {{{"snow*.png", true, true, 0.f}, {"softcircle*.png", false, true, 0.f}, kNoMoreChoices}};
        case ParticleKind::Ember:
            return {{{"ember*.png", true, false, 10.f}, {"spark*.png", true, true, 12.f},
                     {"glow*.png", false, true, 0.f}}};
        case ParticleKind::Mist:
            return {{{"mist*.png", false, true, 0.f}, {"softcircle*.png", false, true, 0.f}, kNoMoreChoices}};
    }
    return {{kNoMoreChoices, kNoMoreChoices, kNoMoreChoices}};
}

// 程序贴图里哪几种是像素小图（按 ×3 画）。
bool isPixelKind(ParticleKind kind) {
    return kind == ParticleKind::Petal || kind == ParticleKind::Leaf;
}

float spawnMargin(const Particle& p) {
    return p.size * 0.5f + p.sway + kEdgeSlack;
}

std::uint8_t scaleAlpha(std::uint8_t a, float factor) {
    return static_cast<std::uint8_t>(std::clamp(static_cast<float>(a) * factor, 0.f, 255.f) + 0.5f);
}

Color mixColor(const Color& a, const Color& b, float t) {
    const auto mix = [t](std::uint8_t x, std::uint8_t y) {
        return static_cast<std::uint8_t>(static_cast<float>(x) +
                                         (static_cast<float>(y) - static_cast<float>(x)) * t + 0.5f);
    };
    return Color{mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a)};
}

// 包络之外，各种粒子自己的明灭。
float particleAlpha(ParticleKind kind, const Particle& p) {
    const KindSpec& spec = specOf(kind);
    float a = spec.alpha * lifeEnvelope(p.age, p.life, spec.fadeIn, spec.fadeOut);
    switch (kind) {
        case ParticleKind::Firefly: {
            // 一明一灭，灭的时候几乎看不见：萤火是一闪一闪的，不是一直亮着的灯。
            const float s = 0.5f + 0.5f * std::sin(p.phase * 2.3f + kTwoPi * 0.6f * p.age);
            a *= 0.15f + 0.85f * s * s;
            break;
        }
        case ParticleKind::Ember:
            a *= 0.75f + 0.25f * std::sin(p.phase + kTwoPi * 7.f * p.age);
            break;
        case ParticleKind::Dust:
            a *= 0.75f + 0.25f * std::sin(p.phase + kTwoPi * 0.8f * p.age);
            break;
        default:
            break;
    }
    return a;
}

std::uint32_t mixSeed(std::uint32_t seed) {
    // 种子先打散一遍：相邻的种子（1、2、3……）直接当 xorshift 的状态，
    // 头几个数会非常接近，几套相邻种子的粒子看起来就是同一套。
    std::uint32_t z = seed + 0x9E3779B9u;
    z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
    z = (z ^ (z >> 13)) * 0xC2B2AE35u;
    z ^= z >> 16;
    return z != 0 ? z : 0x6D2B79F5u;   // xorshift 的状态不能是 0（0 会一直是 0）
}

}  // namespace

float lifeEnvelope(float age, float life, float fadeIn, float fadeOut) noexcept {
    if (life <= 0.f) {
        return 0.f;
    }
    const float f = age / life;
    if (f < 0.f || f >= 1.f) {
        return 0.f;
    }
    float e = 1.f;
    if (fadeIn > 0.f && f < fadeIn) {
        e = f / fadeIn;
    }
    if (fadeOut > 0.f && f > 1.f - fadeOut) {
        e = std::min(e, (1.f - f) / fadeOut);
    }
    return e;
}

BlendMode particleBlend(ParticleKind kind) noexcept {
    switch (kind) {
        case ParticleKind::Dust:
        case ParticleKind::Firefly:
        case ParticleKind::Ember:
            return BlendMode::Add;
        default:
            return BlendMode::Alpha;
    }
}

Vec2 particleScreenPosition(ParticleKind kind, const Particle& p, ParticleSpace space,
                            float cameraX, float cameraY) noexcept {
    Vec2 pos{p.x, p.y};
    if (p.sway > 0.f) {
        const float w = kTwoPi * p.swayFreq;
        pos.x += p.sway * std::sin(p.phase + w * p.age);
        if (kind == ParticleKind::Firefly) {
            // 纵向用另一个频率：两个方向的正弦拼成一条不闭合的李萨如曲线，
            // 萤火看起来是在「找路」，而不是在荡秋千。
            pos.y += 0.6f * p.sway * std::sin(p.phase * 1.7f + 1.3f * w * p.age);
        }
    }
    if (space == ParticleSpace::World) {
        pos.x -= cameraX;
        pos.y -= cameraY;
    }
    return pos;
}

int proceduralVariants(ParticleKind kind) noexcept {
    switch (kind) {
        case ParticleKind::Leaf: return 3;
        case ParticleKind::Petal: return 2;
        case ParticleKind::Rain: return 0;
        default: return 1;
    }
}

SpriteImage proceduralSprite(ParticleKind kind, int variant) {
    switch (kind) {
        case ParticleKind::Dust:
        case ParticleKind::Firefly:
        case ParticleKind::Ember:
            return glowSprite();
        case ParticleKind::Snow:
            return softSprite(32);
        case ParticleKind::Mist:
            return softSprite(64);
        case ParticleKind::Leaf:
            switch (variant % 3) {
                case 0: return pixelSprite(kLeaf0);
                case 1: return pixelSprite(kLeaf1);
                default: return pixelSprite(kLeaf2);
            }
        case ParticleKind::Petal:
            return variant % 2 == 0 ? pixelSprite(kPetal0) : pixelSprite(kPetal1);
        case ParticleKind::Rain:
            return SpriteImage{};
    }
    return SpriteImage{};
}

SheetLayout sheetLayout(Point textureSize) noexcept {
    if (textureSize.x <= 0 || textureSize.y <= 0) {
        return SheetLayout{1, std::max(textureSize.x, 0), std::max(textureSize.y, 0)};
    }
    const int frames = std::max(1, textureSize.x / textureSize.y);
    return SheetLayout{frames, textureSize.x / frames, textureSize.y};
}

int particleFrame(const Particle& p, int frames, float fps) noexcept {
    if (frames <= 1) {
        return 0;
    }
    // 起点按变体错开：一盆火星不会齐刷刷地同一帧亮、同一帧暗。
    const int step = fps > 0.f ? static_cast<int>(p.age * fps) : 0;
    return (static_cast<int>(p.variant) + step) % frames;
}

// ---------------------------------------------------------------------------

ParticleSystem::ParticleSystem(std::uint32_t seed) {
    rng_.state = mixSeed(seed);
}

void ParticleSystem::setArtDirectory(std::string dir) {
    artDir_ = std::move(dir);
    spritesEngine_ = nullptr;   // 下次 render 重新解析
}

void ParticleSystem::configure(ParticleKind kind, float rate, const RectF& area,
                               ParticleSpace space) {
    kind_ = kind;
    rate_ = std::max(rate, 0.f);
    area_ = area;
    space_ = space;
    clear();
}

void ParticleSystem::setCapacity(std::size_t capacity) {
    capacity_ = capacity;
    if (particles_.size() > capacity_) {
        // 最老的在最前面（只往后追加、删除保序）。
        particles_.erase(particles_.begin(),
                         particles_.begin() + static_cast<std::ptrdiff_t>(particles_.size() - capacity_));
    }
}

void ParticleSystem::clear() {
    particles_.clear();
    spawnDebt_ = 0.f;
}

void ParticleSystem::spawn(float dt) {
    const KindSpec& spec = specOf(kind_);
    Particle p;
    p.vx = rng_.range(spec.vxMin, spec.vxMax);
    p.vy = rng_.range(spec.vyMin, spec.vyMax);
    p.size = rng_.range(spec.sizeMin, spec.sizeMax);
    p.sway = rng_.range(spec.swayMin, spec.swayMax);
    p.swayFreq = rng_.range(spec.swayFreqMin, spec.swayFreqMax);
    const float spin = rng_.range(spec.spinMin, spec.spinMax);
    p.spin = (rng_.next() & 1u) != 0 ? spin : -spin;
    p.angle = spin > 0.f ? rng_.range(0.f, 360.f) : 0.f;
    p.phase = rng_.range(0.f, kTwoPi);
    const std::span<const Color> palette = paletteOf(kind_);
    p.color = palette[rng_.next() % palette.size()];
    p.variant = static_cast<std::uint8_t>(rng_.next() & 0xFFu);

    const float margin = spawnMargin(p);
    switch (spec.spawn) {
        case Spawn::Inside:
            p.x = rng_.range(area_.x, area_.x + area_.w);
            p.y = rng_.range(area_.y, area_.y + area_.h);
            p.life = rng_.range(spec.lifeMin, spec.lifeMax);
            break;
        case Spawn::Top: {
            // 斜着落的东西要从 area 外侧进场，否则迎风那一侧的边上永远是空的。
            const float travel = area_.h + 2.f * margin;
            const float drift = p.vx * travel / p.vy;
            p.x = rng_.range(area_.x - std::max(drift, 0.f), area_.x + area_.w - std::min(drift, 0.f));
            p.y = area_.y - margin;
            // 寿命只是个兜底：正常是落出 area 底边时被收走。
            p.life = travel / p.vy * 1.5f + 1.f;
            break;
        }
        case Spawn::Bottom:
            p.x = rng_.range(area_.x, area_.x + area_.w);
            p.y = area_.y + area_.h;
            p.life = rng_.range(spec.lifeMin, spec.lifeMax);
            break;
    }
    // 同一帧生出的几颗各自往前走一小截：不然它们会排成一条横线同时进场，
    // 雨大的时候一眼就看得出一排一排的。
    const float head = rng_.unit() * dt;
    p.x += p.vx * head;
    p.y += p.vy * head;
    p.angle += p.spin * head;
    p.age = head;
    particles_.push_back(p);
}

bool ParticleSystem::expired(const Particle& p) const {
    if (p.age >= p.life) {
        return true;
    }
    // 落下来的出了底边就收；往上冒的（火星）只看寿命——它的 area 是「从哪儿冒」
    // （火盆口那一窄条），冒出 area 顶边正是它该干的事，不是该死的时候。
    switch (specOf(kind_).spawn) {
        case Spawn::Top: return p.y > area_.y + area_.h + spawnMargin(p) + 1.f;
        case Spawn::Bottom:
        case Spawn::Inside: return false;
    }
    return false;
}

void ParticleSystem::update(float dt) {
    if (dt <= 0.f) {
        return;
    }
    for (Particle& p : particles_) {
        p.age += dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.angle += p.spin * dt;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                    [this](const Particle& p) { return expired(p); }),
                     particles_.end());

    spawnDebt_ += rate_ * dt;
    while (spawnDebt_ >= 1.f) {
        if (particles_.size() >= capacity_) {
            // 满了：这一帧剩下的全部作废，不攒到下一帧——攒着的话一腾出位置就是一阵爆发。
            spawnDebt_ = 0.f;
            break;
        }
        spawnDebt_ -= 1.f;
        spawn(dt);
    }
}

void ParticleSystem::prewarm(float seconds) {
    const int steps = static_cast<int>(std::ceil(seconds / kPrewarmStep));
    for (int i = 0; i < steps; ++i) {
        update(kPrewarmStep);
    }
}

void ParticleSystem::ensureSprites(Engine& engine) {
    if (spritesEngine_ == &engine && spritesFor_ == kind_) {
        return;
    }
    sprites_.clear();
    ownedSprites_.clear();
    spritesEngine_ = &engine;
    spritesFor_ = kind_;
    usesArt_ = false;

    for (const ArtChoice& choice : artChoicesOf(kind_)) {
        if (choice.pattern == nullptr) {
            break;
        }
        const ScaleMode scale = choice.pixelArt ? ScaleMode::Pixel : ScaleMode::Linear;
        for (const std::string& file : engine.findAssets(artDir_, choice.pattern)) {
            const TextureId id = engine.loadTexture(file, scale);
            if (id != kInvalidTexture) {
                sprites_.push_back(Sprite{id, sheetLayout(engine.textureSize(id))});
            }
        }
        if (!sprites_.empty()) {
            usesArt_ = true;
            pixelArt_ = choice.pixelArt;
            tinted_ = choice.tinted;
            fps_ = choice.fps;
            return;
        }
    }
    // 美术还没到（或指定的目录里没有）：程序贴图。雨没有程序贴图，画成线。
    pixelArt_ = isPixelKind(kind_);
    tinted_ = true;
    fps_ = 0.f;
    for (int v = 0; v < proceduralVariants(kind_); ++v) {
        const SpriteImage img = proceduralSprite(kind_, v);
        OwnedTexture texture(engine, engine.createTexture(img.w, img.h, img.pixels, img.scale));
        if (texture.valid()) {
            sprites_.push_back(Sprite{texture.get(), SheetLayout{1, img.w, img.h}});
            ownedSprites_.push_back(std::move(texture));
        }
    }
}

void ParticleSystem::render(Engine& engine, float cameraX, float cameraY) {
    if (particles_.empty()) {
        return;
    }
    ensureSprites(engine);
    const BlendMode blend = particleBlend(kind_);
    const float screenW = static_cast<float>(kLogicalWidth);
    const float screenH = static_cast<float>(kLogicalHeight);

    for (const Particle& p : particles_) {
        const float alpha = particleAlpha(kind_, p);
        if (alpha <= 0.f) {
            continue;
        }
        const Vec2 pos = particleScreenPosition(kind_, p, space_, cameraX, cameraY);
        Color color = p.color;
        float size = p.size;
        if (kind_ == ParticleKind::Ember) {
            // 火星越飞越红；柔图还越飞越小（像素美术不缩——缩了就不是 ×3 的像素了，只靠淡出）。
            const float t = std::clamp(p.age / p.life, 0.f, 1.f);
            color = mixColor(p.color, kEmberDying, t);
            if (!pixelArt_) {
                size *= 1.f - 0.6f * t;
            }
        }
        // 屏幕外的不画。余量要盖住雨丝的长度与像素贴图的实际大小。
        const float reach = size + std::fabs(p.vy) * kRainStreakSeconds + 24.f;
        if (pos.x < -reach || pos.x > screenW + reach || pos.y < -reach || pos.y > screenH + reach) {
            continue;
        }
        color.a = scaleAlpha(color.a, alpha);

        if (kind_ == ParticleKind::Rain && sprites_.empty()) {
            engine.drawLine(pos.x, pos.y, pos.x - p.vx * kRainStreakSeconds,
                            pos.y - p.vy * kRainStreakSeconds, color);
            continue;
        }
        if (sprites_.empty()) {
            continue;   // 无头模式：一张贴图都建不出来
        }
        const Sprite& sprite = sprites_[p.variant % sprites_.size()];
        const SheetLayout& sheet = sprite.layout;
        const int frame = particleFrame(p, sheet.frames, fps_);
        const RectF src{static_cast<float>(frame * sheet.frameW), 0.f,
                        static_cast<float>(sheet.frameW), static_cast<float>(sheet.frameH)};
        float w = size;
        float h = size;
        DrawOptions options;
        options.blend = blend;
        options.angle = p.angle;
        options.tint = tinted_ ? color : Color{255, 255, 255, color.a};
        if (pixelArt_) {
            w = static_cast<float>(sheet.frameW * kParticlePixelScale);
            h = static_cast<float>(sheet.frameH * kParticlePixelScale);
        }
        if (kind_ == ParticleKind::Rain) {
            // 雨丝贴图是竖的：按速度方向转过去（亮的那头朝前）。柔图就按画线时的长度拉。
            options.angle = std::atan2(-p.vx, p.vy) * 180.f / std::numbers::pi_v<float>;
            if (!pixelArt_) {
                w = 2.f;
                h = std::sqrt(p.vx * p.vx + p.vy * p.vy) * kRainStreakSeconds;
            }
        }
        engine.drawTexture(sprite.id, src, RectF{pos.x - w * 0.5f, pos.y - h * 0.5f, w, h},
                           options);
    }
}

}  // namespace fanren::engine
