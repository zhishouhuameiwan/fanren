#pragma once
// SDL3 平台层的唯一门面。
//
// SDL 类型一概不出现在本头文件里：窗口、渲染器、字体、混音器全部藏在 .cpp
// 的 Impl 里，外界只拿得到 TextureId 这样的不透明句柄。这样 game/ui 层编译时
// 不需要 SDL 头，换掉平台层也不会波及上层。
//
// 八方旅人化改造（docs/octopath-overhaul.md 1.4 节）在这里追加了一整套 HD-2D
// 要用的绘制能力：渲染目标、混合模式、浮点坐标、翻转旋转、顶点色几何、程序纹理、
// 裁剪、文字描边投影、累计时钟。**旧接口一个签名都没动**——几十处调用点与测试靠它们，
// 旧画面也必须一个像素都不变。新接口的用法与契约见 docs/interfaces-octo-engine.md。
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/Result.h"

namespace fanren::engine {

// 物理键位对外一律是 SDL 扫描码的数值（= USB HID usage，跨平台稳定；docs/settings.md 4.4）。0 = 空格位。
using ScanCode = int;
// 扫描码须小于它（= SDL_SCANCODE_COUNT，Engine.cpp 里 static_assert 钉着；设置文件那一侧是 io::kScancodeLimit）。
inline constexpr ScanCode kScanCodeLimit = 512;

struct Color { std::uint8_t r{}, g{}, b{}, a{255}; };
struct Rect  { int x{}, y{}, w{}, h{}; };
struct Point { int x{}, y{}; };

// 浮点矩形：新绘制接口一律用它。人物在两格之间插值滑动、粒子的亚像素漂移、
// 镜头平滑跟随，落点都不在整数上；取整会让慢速移动一抽一抽地走。
struct RectF { float x{}, y{}, w{}, h{}; };

// 逻辑分辨率固定 1280x720，整数缩放到窗口（见 docs/map_spec.md 第 1 节）
inline constexpr int kLogicalWidth  = 1280;
inline constexpr int kLogicalHeight = 720;
inline constexpr int kTileSize      = 32;

using TextureId = std::uint32_t;   // 0 表示无效
inline constexpr TextureId kInvalidTexture = 0;

// 纹理被放大、缩小时怎么取样。
//   Pixel  —— 像素美术。SDL 3.4 的 PIXELART：整数倍时与最近邻一样锐利，
//             亚像素位置上也不会一格宽一格窄地闪边（1.1 节：像素美术一律用它）。
//   Linear —— 双线性。后处理的模糊目标、光斑、柔雾——本来就该是糊的东西。
enum class ScaleMode { Pixel, Linear };

// 画面特效档位（docs/settings.md 4.3）。PostFx 单点读它：Lite = 关辉光、关景深，
// 光照、暗角、调色照做。放在 Engine 上而不是 PostFx 的静态量：每个 Application 各一份，测试之间不串。
enum class EffectsLevel { Full, Lite };

// 混合模式。src 是这次画上去的颜色，a 是它的 alpha，dst 是目标上原有的颜色：
//   None   dst = src                   整块拷贝，alpha 一并拷过去
//   Alpha  dst = src*a + dst*(1-a)     普通半透明
//   Add    dst = src*a + dst           发光、光源叠加（只会变亮）
//   Mod    dst = src*dst               乘色，**不看 alpha**：光照图、调色
//   Mul    dst = src*dst + dst*(1-a)   带 alpha 的乘色（a=0 时不变）
enum class BlendMode { None, Alpha, Add, Mod, Mul };

// drawTexture 的扩展参数。缺省值等价于旧接口的画法（原色、半透明混合、不翻不转）。
struct DrawOptions {
    Color tint{255, 255, 255, 255};   // 颜色与透明度调制：纹理色 × tint
    BlendMode blend = BlendMode::Alpha;
    bool flipX = false;
    bool flipY = false;
    float angle = 0.f;                // 角度制、顺时针、绕 dst 的中心转
};

// drawGeometry 的顶点。
struct Vertex {
    float x{}, y{};                   // 当前渲染目标上的坐标（像素）
    Color color{255, 255, 255, 255};  // 顶点色，三角形内插值；有纹理时与纹理色相乘
    float u{}, v{};                   // 归一化纹理坐标 [0,1]；无纹理时不用
};

// drawText 的扩展样式。缺省值（两样都关）画出来与旧接口一模一样。
struct TextStyle {
    bool shadow = false;
    Color shadowColor{0, 0, 0, 160};
    int shadowOffset = 2;             // 向右下偏移的像素数
    bool outline = false;
    Color outlineColor{0, 0, 0, 220};
    // 以下为对契约的追加（追加在末尾，按字段顺序的聚合初始化不受影响）：
    // 名牌 1 像素够了，战斗里的伤害数字要 2 像素才压得住花哨的背景。
    int outlineWidth = 1;
};

// createTexture 的像素格式：每个元素是 0xRRGGBBAA（R 在最高字节）。
// 按 32 位整数定义而不是按字节序定义，所以与机器大小端无关。
[[nodiscard]] constexpr std::uint32_t packRgba(const Color& c) noexcept {
    return (static_cast<std::uint32_t>(c.r) << 24) | (static_cast<std::uint32_t>(c.g) << 16) |
           (static_cast<std::uint32_t>(c.b) << 8) | static_cast<std::uint32_t>(c.a);
}

class Engine {
public:
    Engine();
    ~Engine();
    // 声明了拷贝操作就抑制了隐式移动操作，Engine 因此既不可拷贝也不可移动——
    // 这是有意的：它持有 SDL 的全局状态（SDL_Init / TTF_Init 的引用计数、
    // 窗口、渲染器），可移动就意味着可能有两个对象先后去 SDL_Quit。
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // headless == true 时不开窗口、不建渲染器：供无头 bot 与 CI 使用。
    // 此模式下所有绘制调用都是合法的空操作，尺寸测量仍然可用。
    //
    // 新接口同样守这条契约：建纹理、建渲染目标、读后缓冲一律返回 kInvalidTexture，
    // textureSize 返回 {0,0}，其余绘制与状态设置全是空操作（EngineTests 钉着）。
    core::Result<bool> init(const std::string& title, bool headless);
    void shutdown();

    // 每帧：pollEvents 收集输入 → beginFrame → 绘制 → endFrame
    //
    // beginFrame 把渲染目标切回后缓冲、取消裁剪再清屏：这两样是「本帧」的状态，
    // 上一帧忘了收尾不该让下一帧整个画进一张离屏纹理里、或只剩一个小窗口。
    // endFrame 同样先切回后缓冲再 present。
    void pollEvents();
    bool shouldQuit() const;
    void beginFrame();
    void endFrame();
    double deltaSeconds() const;

    // 累计运行秒数：init 以来每次 beginFrame 的 deltaSeconds 之和。
    // 给动画与粒子当相位用（灯笼明灭、萤火闪烁）。按钳位后的 delta 累加而不是读墙钟，
    // 所以它与逻辑层拿到的 dt 是同一个口径：断点停了十秒，动画不会跳十秒。
    [[nodiscard]] double elapsedSeconds() const;

    // ---- 纹理 ----
    //
    // 路径的解析顺序：原样可用 → 资源根下的相对路径（"art/fx/glow.png"）→
    // 资源根下 textures/ 里。按路径缓存，同一张图载两次拿到同一个句柄。
    //
    // 单参数版现在等于 ScaleMode::Pixel（PIXELART）；改造前它是 NEAREST。这是旧接口里
    // 唯一变了行为的一处：改的时候游戏里没有任何调用点，而整数倍放大时两者逐像素相同，
    // 只在亚像素位置上 PIXELART 边缘更干净（docs/interfaces-octo-engine.md 2.1）。
    TextureId loadTexture(const std::string& path);   // 失败返回 kInvalidTexture
    TextureId loadTexture(const std::string& path, ScaleMode mode);

    // 从像素数组建纹理（运行时生成的径向渐变、暗角、程序粒子图）。
    // rgba 按行存放，长度必须恰好是 w*h，元素格式见 packRgba。
    // 返回的纹理归调用方所有，不用了要 destroyTexture（或交给 OwnedTexture）。
    TextureId createTexture(int w, int h, const std::vector<std::uint32_t>& rgba, ScaleMode mode);

    // 纹理的像素尺寸。无效句柄、已销毁的句柄、无头模式一律 {0,0}。
    [[nodiscard]] Point textureSize(TextureId id) const;

    // 改纹理的取样方式。后处理的场景图要「降采样时线性、贴回屏幕时像素」两用，
    // 所以取样方式不能只在建的时候定死。
    void setTextureScaleMode(TextureId id, ScaleMode mode);

    // 销毁纹理。句柄随即失效，再拿它画是空操作。
    // loadTexture 的纹理按路径共享：销毁之后别人手里的同一个句柄也一并失效，
    // 下次 loadTexture 同一路径会重新读盘。只在确定没人再用时销毁（例如离开一张图时
    // 丢掉它的烘焙图）。销毁当前渲染目标会先切回后缓冲。
    void destroyTexture(TextureId id);

    // 资源根下 dir 目录里文件名匹配 pattern（通配符 * ?，不分大小写）的文件，
    // 排好序的完整路径。目录不存在返回空表。无头模式照常可用（只查文件系统）。
    //
    // 给「美术还在路上」的场合用：有几张叶子贴图就用几张，不把文件名写死在代码里。
    [[nodiscard]] std::vector<std::string> findAssets(const std::string& dir,
                                                      const std::string& pattern) const;

    // ---- 渲染目标 ----
    //
    // 建一张可以往里画的纹理（离屏画布）。内容建好时清成全透明。
    // 返回的纹理归调用方所有。
    TextureId createRenderTarget(int w, int h, ScaleMode mode);

    // 之后的绘制都画进 target。kInvalidTexture 表示回到后缓冲。
    // 不是渲染目标的句柄（或已销毁的）按回到后缓冲处理，并打一行警告。
    //
    // 注意：视口、裁剪、坐标系都是**每个目标各一份**（SDL3 的规定）。画进一张
    // w×h 的目标时坐标就是那张图的像素坐标，不再是 1280×720 的逻辑坐标。
    void setRenderTarget(TextureId target);
    [[nodiscard]] TextureId renderTarget() const;   // 当前目标；后缓冲为 kInvalidTexture

    // 把当前渲染目标整个清成 color（连 alpha，不混合，不受裁剪影响）。
    void clear(const Color& color);

    // 把后缓冲当前的画面拷成一张新纹理（线性取样），归调用方所有。
    // 开战「碎屏」转场、菜单背景模糊要拿「这一帧最后长什么样」当素材。
    // **必须在 endFrame 之前调**（理由同 captureFrame）；调用时绑着别的目标也照样读后缓冲。
    // 这是一次 GPU→CPU 回读，偶尔调一次没事，不要每帧调。
    [[nodiscard]] TextureId snapshotBackbuffer();

    // ---- 绘制 ----
    void drawTexture(TextureId id, const Rect& src, const Rect& dst);
    // src 宽或高非正表示整张图（与旧接口同一约定）。
    void drawTexture(TextureId id, const RectF& src, const RectF& dst, const DrawOptions& options);
    void drawRect(const Rect& rect, const Color& color, bool filled);
    void fillRect(const RectF& rect, const Color& color, BlendMode blend);

    // 竖向渐变（上 → 下）与横向渐变（左 → 右）。颜色带 alpha，可以渐隐。
    void drawGradient(const RectF& rect, const Color& top, const Color& bottom, BlendMode blend);
    void drawGradientH(const RectF& rect, const Color& left, const Color& right, BlendMode blend);

    // 三角形列表。texture 可为 kInvalidTexture（纯顶点色）。
    // indices 每三个一组指向 vertices；为空时按 vertices 的顺序每三个一组。
    // 有纹理时颜色调制只看顶点色（SDL 的规定：纹理的 tint 在这里不生效）。
    void drawGeometry(TextureId texture, const std::vector<Vertex>& vertices,
                      const std::vector<int>& indices, BlendMode blend);

    void drawLine(float x1, float y1, float x2, float y2, const Color& color);

    // 裁剪矩形，只作用于**当前**渲染目标；nullptr 取消。
    void setClipRect(const Rect* rect);

    // ---- 文本 ----
    // size 为像素字号。字体在 init 时从 assets/fonts/ 载入，全项目一套。
    void drawText(const std::string& utf8, int x, int y, int size, const Color& color);
    // 带投影 / 描边的版本。正文画在 (x, y)，与旧接口落在同一处——描边往外扩、
    // 投影往右下偏，排版（measureText）不必为样式改一个数。
    void drawText(const std::string& utf8, int x, int y, int size, const Color& color,
                  const TextStyle& style);
    Point measureText(const std::string& utf8, int size) const;

    // ---- 抓帧 ----
    // 把当前这一帧存成 PNG。开发自检用：改了渲染却看不见自己改成什么样，
    // 只能靠「应该好看了」交付，而那在本工程里不算证据。
    //
    // **必须在 endFrame 之前调**：present 之后后台缓冲的内容按 SDL 的规定是
    // 未定义的。无头模式没有渲染器，如实返回失败而不是悄悄写一个空文件。
    // 调用时若还绑着别的渲染目标，照样抓后缓冲（抓完把目标恢复原样）。
    [[nodiscard]] core::Result<bool> captureFrame(const std::string& path);

    // ---- 输入 ----
    // Key 是逻辑键位，不是物理扫描码；映射表在 engine 内部维护。
    // Save、Action 都追加在 Count 之前：keyPressed 的数组按 Count 定长，插在中间不影响。
    // Action 是八方旅人式的「路径行动」键（E / Q，施工图 1.5 节）。
    // Alt+Enter（主键盘与小键盘的 Enter）是切全屏，整个吞掉：不算 Confirm，按住也不自动重复出 Confirm。
    // 键盘与手柄（docs/gamepad.md）换算成同一套逻辑键：按住 = 键盘按住或任一手柄上它的任一手柄键按住。
    enum class Key { Up, Down, Left, Right, Confirm, Cancel, Menu, Skip, Save, Action, Count };
    bool keyDown(Key key) const;      // 当前是否按住
    // 本帧是否刚按下。只有上下左右连发：按住 250ms 后每 60ms 再出一次（翻长列表、调数值）；
    // 确认、取消、菜单、快进、存盘、路径行动一次物理按下只出一次，按住不连发（docs/gamepad.md 第 5 节：
    // 否则按住 Tab / Esc 时主菜单每 60ms 开合一次）。要「按住」的语义（快进）问 keyDown。
    bool keyPressed(Key key) const;

    // ---- 键位（docs/settings.md 4.4、第 6 节）----
    //
    // 一个动作的键 = 固定键（方向键、Enter、小键盘 Enter、Esc：不可改、不可删，玩家永远不会把自己锁在菜单外）
    // + 两格自定义键位。默认的自定义键位与固定键合起来恰是改造前那 19 条映射。
    static constexpr std::size_t kKeyCount = static_cast<std::size_t>(Key::Count);
    using KeySlots = std::array<ScanCode, 2>;
    using KeyTable = std::array<KeySlots, kKeyCount>;   // 按 Key 的次序

    // 抓键时有特殊意思的三个键（改键面板：Esc 作罢，Backspace / Delete 清空这一格）。
    static constexpr ScanCode kEscapeCode = 41;
    static constexpr ScanCode kBackspaceCode = 42;
    static constexpr ScanCode kDeleteCode = 76;

    // 词表：动作 id（设置文件 keys 里的键，与 io::kKeyActionIds 同一组词）↔ Key。
    [[nodiscard]] static const char* keyId(Key key);
    [[nodiscard]] static std::optional<Key> keyFromId(std::string_view id);

    [[nodiscard]] static std::vector<ScanCode> fixedKeys(Key key);
    [[nodiscard]] static KeySlots defaultCustomKeys(Key key);
    [[nodiscard]] static KeyTable defaultKeyTable();
    [[nodiscard]] KeySlots customKeys(Key key) const;

    // 这个扫描码是谁的固定键（不是固定键 = nullopt）；是不是保留键（左右 Alt、左右 Win、菜单键、
    // Backspace、Delete：前三样另有用处，后两样是抓键时的「清空」）。
    [[nodiscard]] static std::optional<Key> fixedKeyOwner(ScanCode code);
    [[nodiscard]] static bool reservedKey(ScanCode code);

    // 整张表的校验：扫描码越界、配了固定键或保留键、两处配了同一个键、某动作一个键都不剩——
    // 任何一条不过就失败，error 说是哪一条（给日志看，用动作 id 与键的显示名）。
    [[nodiscard]] static core::Result<bool> validateKeyTable(const KeyTable& table);
    // 整张换上；校验不过**不改**、返回原因。与眼下相同的表是成功的空操作。
    core::Result<bool> setCustomKeys(const KeyTable& table);

    // 改一格（第 6 节）：把键 code 放到动作 action 的第 slot 格（0 / 1）。纯函数，不碰 SDL 事件：
    //   1. code 是任何动作的固定键或保留键 → 拒绝（RejectedFixed 带上是谁的，RejectedReserved）；
    //   2. code 就在这一格 → 无事（Unchanged）；
    //   3. code 在 action 的另一格 → 两格互换（SwappedSlots）；这一格原来空着时就是从另一格挪过来（MovedSlot）；
    //   4. code 在别的动作 B 的某一格 → 那一格换成这一格原来的键（TradedWith，other = B）；这一格原来空着时，
    //      B 那一格就空了出来（TookFrom，other = B）；B 因此一个键都不剩 → 拒绝（RejectedLastKey，other = B）；
    //   5. 否则这一格 = code（Assigned）。
    // 清空一格（clearKey）：本来就空着 → AlreadyEmpty；action 因此一个键都不剩 → 拒绝（RejectedLastKey，other = action）。
    // 拒绝时 table 就是原表。不变式：每个动作至少一个键（固定或自定义）；两个动作永远不共用一个键。
    // 结果按实际发生的事分开：说明行照它说话，「挪过来」不说成「互换」，「本来就空」不说成「就是这个键」。
    enum class KeyChange {
        Assigned, Unchanged, SwappedSlots, MovedSlot, TradedWith, TookFrom, Cleared, AlreadyEmpty,
        RejectedFixed, RejectedReserved, RejectedLastKey, RejectedInvalid,
    };
    struct KeyChangeResult {
        KeyTable table{};
        KeyChange change = KeyChange::Unchanged;
        Key other = Key::Count;
    };
    [[nodiscard]] static KeyChangeResult assignKey(const KeyTable& table, Key action, int slot, ScanCode code);
    [[nodiscard]] static KeyChangeResult clearKey(const KeyTable& table, Key action, int slot);

    // 抓键（改键面板）：beginKeyCapture 之后，下一次物理按下（非重复）不映射成任何逻辑键，由 takeCapturedKey
    // 取走；抓键期间也不出任何逻辑键（连按住的键的自动重复也没有）。Alt+Enter 照样切全屏、不被抓；
    // 纯修饰的保留键（左右 Alt、左右 Win）也不抓——它们多半是 Alt+Enter、Alt+Tab、Win 组合键的前一半；
    // 按着 Alt / Win 按下去的键（Alt+F4、Alt+空格、Win+D……）同样不抓、不结束抓键：组合键本来也配不了。
    // 没映射的键（扫描码 0：媒体键、Fn 组合之类）任何时候都不算一个键。
    // cancelKeyCapture 是追加的：面板中途被关掉时把引擎放回常态，不然之后一个逻辑键都不出。
    void beginKeyCapture();
    void cancelKeyCapture();
    [[nodiscard]] bool capturingKey() const;
    [[nodiscard]] std::optional<ScanCode> takeCapturedKey();

    // 显示名（第 6 节）：字母、数字、F1–F12 照写；空格 → 空格；Return → Enter；小键盘 Enter → 小键盘Enter；
    // Esc、Tab 照写；方向 → ↑↓←→；左 Ctrl → Ctrl、右 Ctrl → 右Ctrl；左 Shift → Shift、右 Shift → 右Shift；
    // 小键盘数字 → 小键盘0…；其余用 SDL_GetScancodeName，空名 → 键#<数值>。
    [[nodiscard]] static std::string keyLabel(ScanCode code);

    // ---- 手柄（docs/gamepad.md）----
    // 按位置命名（Xbox 的叫法）：A 下、B 右、X 左、Y 上。Stick* 是左摇杆数字化后的四向，LT / RT 是扳机数字化后的按下。
    // 手柄键位固定、不可改（第 2 节）；LB、LT 不配任何动作。非无头时 init 打开手柄子系统（起不来只打一行警告、照样启动），
    // 热插拔跟着 SDL 的 ADDED / REMOVED 走；pollEvents 处理手柄事件不看是谁开的子系统（测试自己开、或用 SDL_PushEvent 塞）。
    enum class PadButton {
        A, B, X, Y, Back, Start, LB, RB, LT, RT,
        DpadUp, DpadDown, DpadLeft, DpadRight,
        StickUp, StickDown, StickLeft, StickRight,
        Count
    };
    enum class InputDevice { Keyboard, Gamepad };

    [[nodiscard]] static std::vector<PadButton> padButtons(Key key);   // 第 2 节那张表，按显示优先次序
    [[nodiscard]] static std::string padLabel(PadButton button);       // 第 2 节「显示名」
    // 左摇杆 → 四向（第 3 节）。x、y 是归一到 [-1, 1] 的轴值，y 向下为正；held 是这根摇杆眼下按着的方向。
    // 推到位：没按住时 r ≥ 0.5；方向取最近的轴（恰在对角线取左右）。按住时偏离原方向不超过 55°、且 r ≥ 0.35 就保持；
    // 偏出 55° 换向算一次新的推，新方向同样要 r ≥ 0.5，不够就当松开。引擎按一份完整的报告（两轴收齐）调它，不按单个轴事件。
    [[nodiscard]] static std::optional<Key> stickDirection(float x, float y, std::optional<Key> held);
    // 扳机（第 3 节）：value 归一到 [0, 1]。没按住时 ≥ 0.5 算按下，按住时 ≥ 0.3 仍按住。
    [[nodiscard]] static bool triggerDown(float value, bool wasDown);
    // 最后一下配给了动作的按下来自键盘还是手柄（第 6 节，缺省键盘）。提示文案跟着它换 .pad 版本。
    [[nodiscard]] InputDevice lastInputDevice() const;
    [[nodiscard]] int gamepadCount() const;                            // 眼下开着的手柄数（句柄非空的）
    // 手柄震动（第 8 节）。开关无头也存值（读回的就是存进去的），缺省开。
    void setRumbleEnabled(bool on);
    [[nodiscard]] bool rumbleEnabled() const;
    // 唯一的闸：开关开着、眼下的设备是手柄、最后按键的那个手柄还接着才震；否则什么也不做。
    // 强度夹到 [0, 1]，时长夹到 [0, 1000] 毫秒。
    void rumble(float low, float high, int durationMs);

    // ---- 音频 ----
    void playBgm(const std::string& id);
    void stopBgm();
    void playSfx(const std::string& id);

    // ---- 系统设置（docs/settings.md 第 4 节）----
    // 缺省值全是改造前的行为：增益 1、窗口、整数倍、垂直同步开、特效完整。
    // 各项**无头也存值**（读回的就是存进去的那个），只是不碰窗口、渲染器与混音器。

    // 音量：线性增益，夹在 [0, 1]（0 静音，1 = 改造前的响度）。BGM 调它那条轨道的增益；
    // 音效调音效轨道池里每一条的增益（MIX_PlayAudio 发完即忘、调不了音量，所以音效改走轨道池）。
    void setBgmVolume(float gain);
    void setSfxVolume(float gain);
    [[nodiscard]] float bgmVolume() const;
    [[nodiscard]] float sfxVolume() const;

    // 全屏：无边框桌面全屏。fullscreen() 读窗口的**真实**状态（窗口系统可能驳回请求）；
    // 无头读存的值。Alt+Enter 由 pollEvents 自己切（见那里），Application 每帧对一次账。
    void setFullscreen(bool on);
    [[nodiscard]] bool fullscreen() const;

    // 画面缩放：整数倍（缺省）/ 铺满（保持 16:9、不够的那一边留黑边）。
    // 铺满时回读后缓冲（snapshotBackbuffer）拿到的内容区不再是 1280×720 的整数倍，按呈现矩形裁。
    void setIntegerScale(bool on);
    [[nodiscard]] bool integerScale() const;

    // 垂直同步。关的时候 endFrame 补睡到 1/240 秒：关它是为了少一帧延迟，不是为了让一个核空转。
    void setVSync(bool on);
    [[nodiscard]] bool vsync() const;

    void setEffectsLevel(EffectsLevel level);
    [[nodiscard]] EffectsLevel effectsLevel() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// 自有纹理的 RAII 句柄：createTexture / createRenderTarget / snapshotBackbuffer
// 建出来的纹理归建它的人，包一层就不会忘了 destroyTexture，场景反复进出也不漏显存。
// loadTexture 的纹理按路径共享，**不要**用它包（它一析构，别人手里的句柄也跟着失效）。
//
// 只可移动：两份句柄指着同一张纹理，先析构的那份会把另一份变成悬空句柄。
// 必须在它引用的 Engine 之前析构；Engine shutdown 之后再析构是安全的空操作
// （纹理已随 shutdown 释放，句柄编号不会复用）。
class OwnedTexture {
public:
    OwnedTexture() = default;
    OwnedTexture(Engine& engine, TextureId id) noexcept : engine_(&engine), id_(id) {}
    ~OwnedTexture() { reset(); }

    OwnedTexture(const OwnedTexture&) = delete;
    OwnedTexture& operator=(const OwnedTexture&) = delete;
    OwnedTexture(OwnedTexture&& other) noexcept : engine_(other.engine_), id_(other.id_) {
        other.engine_ = nullptr;
        other.id_ = kInvalidTexture;
    }
    OwnedTexture& operator=(OwnedTexture&& other) noexcept {
        if (this != &other) {
            reset();
            engine_ = other.engine_;
            id_ = other.id_;
            other.engine_ = nullptr;
            other.id_ = kInvalidTexture;
        }
        return *this;
    }

    [[nodiscard]] TextureId get() const noexcept { return id_; }
    [[nodiscard]] bool valid() const noexcept { return id_ != kInvalidTexture; }

    void reset() {
        if (engine_ != nullptr && id_ != kInvalidTexture) {
            engine_->destroyTexture(id_);
        }
        engine_ = nullptr;
        id_ = kInvalidTexture;
    }

private:
    Engine* engine_ = nullptr;
    TextureId id_ = kInvalidTexture;
};

}  // namespace fanren::engine
