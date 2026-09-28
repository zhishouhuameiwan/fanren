#pragma once
// 粒子（docs/octopath-overhaul.md 第 3 节：按地图主题选尘埃光点、萤火、落叶、花瓣、雨、
// 火星、薄雾）。
//
// 三条设计取舍：
//   * **确定性**：同一个种子、同一串 update(dt)，粒子一个不差地落在同一处。
//     随机数自己算（xorshift），不用 <random> 的分布——那些分布的算法由标准库实现
//     自定，换个编译器同一颗种子就是另一场雨，截图对比与单测都会跟着漂。
//   * **有上限**：每个系统最多 capacity 颗，满了新的就不生。发射率写错一个数量级
//     （数据里多打一个零）只会让画面稀一点密一点，不会把内存与帧率吃光。
//   * **两种画法都是完整的**：贴图优先读 assets/art/fx/ 里的美术产物（tools/artgen/fx.py
//     生成，文件名按通配找，单行横排的方形帧）；没有时用运行时生成的程序贴图——
//     那是正经的第二种画法，不是缺图时的应急色块。
//
// 用法：
//   engine::ParticleSystem leaves(种子);
//   leaves.configure(engine::ParticleKind::Leaf, 6.f, {0, 0, 1280, 720});   // 每秒 6 片
//   leaves.prewarm(8.f);                  // 可选：开场就是「已经下了一会儿」的样子
//   每帧：leaves.update(dt); ... leaves.render(engine, cameraX, cameraY);
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "engine/Engine.h"

namespace fanren::engine {

enum class ParticleKind {
    Dust,      // 阳光里的尘埃光点：极慢的漂浮，Add
    Firefly,   // 萤火：绕圈游走、忽明忽暗，Add
    Petal,     // 花瓣：飘落、旋转、左右摆
    Leaf,      // 落叶：同上，更大更重
    Rain,      // 雨：斜线
    Snow,      // 雪：慢落、轻摆
    Ember,     // 火星：上升、闪烁、越飞越小越红，Add
    Mist,      // 薄雾：大块柔雾缓慢横移，alpha 很低
};

// 粒子坐标的参照系。
//   Screen —— 屏幕像素，镜头怎么动它都钉在屏幕上（满屏的雨雪）；
//   World  —— 世界像素，画的时候减去镜头位置（火盆上的火星、池塘边的萤火）。
enum class ParticleSpace { Screen, World };

struct Vec2 { float x{}, y{}; };

struct Particle {
    float x = 0.f, y = 0.f;       // 本体位置（摆动另算，见 particleScreenPosition）
    float vx = 0.f, vy = 0.f;     // 像素/秒
    float age = 0.f, life = 0.f;  // 秒
    float size = 0.f;             // 屏幕上的直径（像素）；像素贴图按贴图原尺寸 ×3 画，不看它
    float angle = 0.f;            // 角度制
    float spin = 0.f;             // 度/秒
    float phase = 0.f;            // 摆动与明灭的起始相位（弧度）
    float sway = 0.f;             // 摆动幅度（像素）
    float swayFreq = 0.f;         // 摆动频率（赫兹）
    Color color{255, 255, 255, 255};
    std::uint8_t variant = 0;     // 贴图变体（对变体数取模）
};

// 生命期包络 [0,1]：前 fadeIn（生命期比例）淡入，最后 fadeOut 淡出，中间为 1。
[[nodiscard]] float lifeEnvelope(float age, float life, float fadeIn, float fadeOut) noexcept;

// 这一种粒子用什么混合：光点类（尘埃、萤火、火星）Add，其余 Alpha。
[[nodiscard]] BlendMode particleBlend(ParticleKind kind) noexcept;

// 粒子这一刻画在屏幕上的哪儿：本体位置 + 摆动；世界空间再减去镜头。
[[nodiscard]] Vec2 particleScreenPosition(ParticleKind kind, const Particle& p, ParticleSpace space,
                                          float cameraX, float cameraY) noexcept;

// 程序贴图。光点类是线性取样的柔光圆；叶与花瓣是 16px 尺度的像素小图（按 ×3 画，
// 与地图、人物同一像素密度），灰阶明暗，颜色靠绘制时的 tint 上。
struct SpriteImage {
    int w = 0;
    int h = 0;
    std::vector<std::uint32_t> pixels;   // packRgba 格式
    ScaleMode scale = ScaleMode::Linear;
};
[[nodiscard]] int proceduralVariants(ParticleKind kind) noexcept;   // 雨为 0：画成线
[[nodiscard]] SpriteImage proceduralSprite(ParticleKind kind, int variant);

// 美术贴图的帧布局。assets/art/fx 的约定（fx.py / index.json）：多帧图**单行横排、方形帧**，
// 于是帧数 = 宽 ÷ 高；宽不足高（雨丝 4×16）就是一帧。按尺寸推而不去读 index.json：
// 引擎层不解析 JSON（那是 io 层的事），而这一条约定够粒子用。
struct SheetLayout {
    int frames = 1;
    int frameW = 0;
    int frameH = 0;
};
[[nodiscard]] SheetLayout sheetLayout(Point textureSize) noexcept;

// 粒子这一刻用第几帧：fps > 0 按寿命轮播（火星明灭、萤火亮暗），各颗错开起点；
// fps = 0 每颗粒子固定一帧（叶形、花瓣形、雪片大小各不相同）。
[[nodiscard]] int particleFrame(const Particle& p, int frames, float fps) noexcept;

// 像素贴图（叶、花瓣、美术的光点与雪片）画到屏幕上的放大倍数：16px 美术 ×3，施工图 1.1 节。
inline constexpr int kParticlePixelScale = 3;

// 美术贴图的缺省目录（相对资源根）。
inline constexpr const char* kParticleArtDir = "art/fx";

class ParticleSystem {
public:
    static constexpr std::size_t kDefaultCapacity = 512;

    explicit ParticleSystem(std::uint32_t seed = 1);

    // 只可移动：它持有程序贴图（OwnedTexture），拷一份就是两个主人。
    // 移动显式声明为 noexcept，std::vector 扩容时才会挪而不是去拷。
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;
    ParticleSystem(ParticleSystem&&) noexcept = default;
    ParticleSystem& operator=(ParticleSystem&&) noexcept = default;
    ~ParticleSystem() = default;

    // 换一种粒子：清掉现有的，按 rate（每秒生几颗，负数按 0）在 area 里发。
    // area 的含义随种类：落下来的（花瓣、叶、雨、雪）从 area 顶上进、底下出；
    // 火星从 area 底边往上冒，冒出 area 之后照样飞到寿命用完（area 给火盆口那一窄条即可）；
    // 其余在 area 里随处生灭。坐标按 space 解释。
    void configure(ParticleKind kind, float rate, const RectF& area,
                   ParticleSpace space = ParticleSpace::Screen);

    // 同时存在的上限。调小时多出来的立刻丢掉（丢最老的）。
    void setCapacity(std::size_t capacity);

    // 到哪个目录（相对资源根）找美术贴图，缺省 kParticleArtDir。给一个没有图的目录
    // 就一律用程序贴图——测试靠它在有美术、没美术两种树上都把两种画法验到。
    void setArtDirectory(std::string dir);
    // 上一次 render 用的是不是美术贴图（false：程序贴图，或还没 render 过）。
    [[nodiscard]] bool usesArt() const { return usesArt_; }

    void update(float dt);
    // 按 1/30 秒一步空跑 seconds 秒：进场景的第一帧就是稳态，而不是雨刚开始下。
    void prewarm(float seconds);
    void render(Engine& engine, float cameraX, float cameraY);
    void clear();

    [[nodiscard]] const std::vector<Particle>& particles() const { return particles_; }
    [[nodiscard]] ParticleKind kind() const { return kind_; }
    [[nodiscard]] ParticleSpace space() const { return space_; }
    [[nodiscard]] const RectF& area() const { return area_; }
    [[nodiscard]] float rate() const { return rate_; }
    [[nodiscard]] std::size_t capacity() const { return capacity_; }

private:
    // xorshift32：够快、够散，最要紧的是算法写死在这里，处处同一串数。
    struct Rng {
        std::uint32_t state = 0x6D2B79F5u;
        std::uint32_t next() noexcept {
            std::uint32_t x = state;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            state = x;
            return x;
        }
        float unit() noexcept { return static_cast<float>(next() >> 8) * (1.f / 16777216.f); }
        float range(float lo, float hi) noexcept { return lo + (hi - lo) * unit(); }
    };

    void spawn(float dt);
    [[nodiscard]] bool expired(const Particle& p) const;
    void ensureSprites(Engine& engine);

    Rng rng_;
    ParticleKind kind_ = ParticleKind::Dust;
    ParticleSpace space_ = ParticleSpace::Screen;
    RectF area_{};
    float rate_ = 0.f;
    float spawnDebt_ = 0.f;
    std::size_t capacity_ = kDefaultCapacity;
    std::vector<Particle> particles_;

    // 贴图在第一次 render 时才解析（要 Engine 才找得到文件、建得了纹理）。
    // spritesFor_ / spritesEngine_ 记下是替哪种粒子、在哪台引擎上解析的，换了就重来。
    struct Sprite {
        TextureId id = kInvalidTexture;
        SheetLayout layout;
    };
    std::vector<Sprite> sprites_;
    std::vector<OwnedTexture> ownedSprites_;   // 程序贴图归本系统所有；读盘的按路径共享，不在这里
    const Engine* spritesEngine_ = nullptr;
    ParticleKind spritesFor_ = ParticleKind::Dust;
    std::string artDir_ = kParticleArtDir;
    bool usesArt_ = false;
    bool pixelArt_ = false;   // 按帧原尺寸 ×3 画（像素美术）还是按粒子大小拉伸（柔图）
    bool tinted_ = true;      // 乘种类配色（灰度 / 白色出图）还是保持原色（自带颜色的图）
    float fps_ = 0.f;         // 帧轮播速度，0 = 每颗固定一帧
};

}  // namespace fanren::engine
