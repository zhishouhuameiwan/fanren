#include "engine/Engine.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

// SDL3 相对 SDL2 的两处硬性变化，本文件是全工程唯一需要知道它们的地方：
//   1. SDL3_image 没有 IMG_Init / IMG_Quit，直接调 IMG_LoadTexture 即可；
//   2. SDL3_mixer 换成了 MIX_Mixer / MIX_Track / MIX_Audio 的对象模型，
//      SDL2 的 Mix_OpenAudio + 通道号那一套完全不存在。且要出声必须用
//      MIX_CreateMixerDevice()：MIX_CreateMixer() 建的是往内存缓冲区渲染的
//      离线混音器，且 spec 不许为 NULL。

namespace fanren::engine {
namespace {

constexpr std::size_t kKeyCount = static_cast<std::size_t>(Engine::Key::Count);

// 首次触发后停 250ms 再开始重复，之后每 60ms 一次。只有上下左右连发（docs/gamepad.md 第 5 节，见 Impl::press）。
// 这两个值决定了菜单长按的手感：延迟短了会「手一抖跳两格」，间隔长了翻长列表磨人。
constexpr std::uint64_t kKeyRepeatDelayMs = 250;
constexpr std::uint64_t kKeyRepeatIntervalMs = 60;

// 断点调试或窗口拖动后 dt 会是几秒，直接喂给逻辑层会让角色瞬移穿墙。
constexpr double kMaxDeltaSeconds = 0.1;

// 文字纹理缓存的存活帧数。对白框每帧重画同一句话，不缓存就是每帧
// 一次 CreateTexture + 一次 DestroyTexture。
constexpr std::uint64_t kTextCacheTtlFrames = 240;
constexpr std::uint64_t kTextCacheSweepFrames = 120;

// 渲染目标的像素格式。ARGB8888 是 SDL 各后端（D3D11/12、Vulkan、GL、软件）都原生
// 支持的那一种，建目标时不会被悄悄换成别的格式再转来转去。
constexpr SDL_PixelFormat kTargetFormat = SDL_PIXELFORMAT_ARGB8888;

// 音效轨道池（docs/settings.md 4.1）。MIX_PlayAudio 发完即忘、不接受任何混音参数，调不了音量，
// 所以音效改成自己管的一池轨道。12 条够：连击 1+N 最多 6 下命中音，加上界面音效仍有余量。
constexpr std::size_t kSfxTrackCount = 12;

// 关了垂直同步时一帧至少占多久：封顶 240 帧/秒（docs/settings.md 4.2）。
constexpr std::uint64_t kUncappedFrameNs = static_cast<std::uint64_t>(SDL_NS_PER_SECOND) / 240u;

SDL_RendererLogicalPresentation presentationFor(bool integerScale) {
    // 整数缩放：像素美术一旦按非整数倍拉伸，格子边缘就会出现宽窄不一的行。
    // 铺满是玩家自己选的取舍（窗口 / 屏幕不是 1280×720 的整数倍时不留大黑边）。
    return integerScale ? SDL_LOGICAL_PRESENTATION_INTEGER_SCALE : SDL_LOGICAL_PRESENTATION_LETTERBOX;
}

// 增益夹在 [0, 1]：MIX_SetTrackGain 不收负数；1 就是改造前的响度，设置里没有「比原来更响」这一档。
// NaN 也落到 0（!(x > 0) 对 NaN 成立），不把它交给混音器。
float sanitizeGain(float gain) {
    return !(gain > 0.f) ? 0.f : std::min(gain, 1.f);
}

// 键位（docs/settings.md 第 6 节）。改造前是一张 19 条的 kBindings；改键之后拆成两半：
// 固定键（这里，编译期常量，谁也改不了）与每个动作两格自定义键位（Impl::customKeys，运行时可换）。
// 两半合起来的默认值与那 19 条一一对应（EngineTests 的 MapsScancodesToLogicalKeys 一字没改照样绿）。
static_assert(kScanCodeLimit == SDL_SCANCODE_COUNT, "engine::kScanCodeLimit 必须等于 SDL_SCANCODE_COUNT");
static_assert(Engine::kEscapeCode == SDL_SCANCODE_ESCAPE && Engine::kBackspaceCode == SDL_SCANCODE_BACKSPACE &&
                  Engine::kDeleteCode == SDL_SCANCODE_DELETE,
              "抓键用的三个扫描码要与 SDL 的枚举对得上");

struct FixedKey {
    SDL_Scancode scancode;
    Engine::Key key;
};

// 固定键：方向键、Enter、小键盘 Enter、Esc。不可改、不可删——不管自定义键位改成什么样，
// 玩家都能用它们走出任何一个菜单。
constexpr std::array<FixedKey, 7> kFixedKeys{{
    {SDL_SCANCODE_UP, Engine::Key::Up},
    {SDL_SCANCODE_DOWN, Engine::Key::Down},
    {SDL_SCANCODE_LEFT, Engine::Key::Left},
    {SDL_SCANCODE_RIGHT, Engine::Key::Right},
    {SDL_SCANCODE_RETURN, Engine::Key::Confirm},
    {SDL_SCANCODE_KP_ENTER, Engine::Key::Confirm},
    {SDL_SCANCODE_ESCAPE, Engine::Key::Cancel},
}};

// 默认的两格自定义键位（按 Key 的次序，0 = 空格位）：
//   方向与 WASD 等价；Confirm / Cancel 各给几个习惯键位，免得玩家在「回车还是空格」上试错；
//   F5 存盘——选功能键而不是字母：走格子时字母键全是移动与确认，而存盘是个不该被手滑按到的动作；
//   E / Q 路径行动（施工图 1.5 节）——就在 WASD 旁边，左手不用离开方向键；两个都给，是因为惯用 E 的
//   与惯用 Q 的玩家各占一半，谁也不该去翻按键说明。
constexpr Engine::KeyTable kDefaultKeys{{
    {SDL_SCANCODE_W, 0},                      // Up
    {SDL_SCANCODE_S, 0},                      // Down
    {SDL_SCANCODE_A, 0},                      // Left
    {SDL_SCANCODE_D, 0},                      // Right
    {SDL_SCANCODE_Z, SDL_SCANCODE_SPACE},     // Confirm
    {SDL_SCANCODE_X, 0},                      // Cancel
    {SDL_SCANCODE_TAB, 0},                    // Menu
    {SDL_SCANCODE_LCTRL, 0},                  // Skip
    {SDL_SCANCODE_F5, 0},                     // Save
    {SDL_SCANCODE_E, SDL_SCANCODE_Q},         // Action
}};

// 保留键：左右 Alt（Alt+Enter）、左右 Win、菜单键，与抓键时当「清空」用的 Backspace、Delete。
constexpr std::array<SDL_Scancode, 7> kReservedKeys{
    SDL_SCANCODE_LALT, SDL_SCANCODE_RALT,        SDL_SCANCODE_LGUI,   SDL_SCANCODE_RGUI,
    SDL_SCANCODE_APPLICATION, SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_DELETE,
};

// 动作 id（设置文件 keys 里的键），按 Key 的次序。与 io::kKeyActionIds 是同一组词（KeyVocabulary 用例钉着）。
constexpr std::array<const char*, kKeyCount> kKeyIds{
    "up", "down", "left", "right", "confirm", "cancel", "menu", "skip", "save", "action",
};

// 手柄键位（docs/gamepad.md 第 2 节，固定、不可改）：按位置认键，叫法用 Xbox 的。每个动作的手柄键按显示优先的次序排
// （提示文案 {pad.<id>} 取那个动作的第一个）。一个手柄键只属于一个动作；LB、LT 不配动作，不在表里。
constexpr std::size_t kPadButtonCount = static_cast<std::size_t>(Engine::PadButton::Count);

struct PadKey {
    Engine::PadButton button;
    Engine::Key key;
};

constexpr std::array<PadKey, 16> kPadKeys{{
    {Engine::PadButton::DpadUp, Engine::Key::Up},       {Engine::PadButton::StickUp, Engine::Key::Up},
    {Engine::PadButton::DpadDown, Engine::Key::Down},   {Engine::PadButton::StickDown, Engine::Key::Down},
    {Engine::PadButton::DpadLeft, Engine::Key::Left},   {Engine::PadButton::StickLeft, Engine::Key::Left},
    {Engine::PadButton::DpadRight, Engine::Key::Right}, {Engine::PadButton::StickRight, Engine::Key::Right},
    {Engine::PadButton::A, Engine::Key::Confirm},       {Engine::PadButton::B, Engine::Key::Cancel},
    {Engine::PadButton::Y, Engine::Key::Menu},          {Engine::PadButton::Start, Engine::Key::Menu},
    {Engine::PadButton::RT, Engine::Key::Skip},         {Engine::PadButton::RB, Engine::Key::Skip},
    {Engine::PadButton::Back, Engine::Key::Save},       {Engine::PadButton::X, Engine::Key::Action},
}};

// 左摇杆数字化后的四向与它们对应的方向键。
constexpr std::array<PadKey, 4> kStickWays{{
    {Engine::PadButton::StickUp, Engine::Key::Up},
    {Engine::PadButton::StickDown, Engine::Key::Down},
    {Engine::PadButton::StickLeft, Engine::Key::Left},
    {Engine::PadButton::StickRight, Engine::Key::Right},
}};

// 摇杆、扳机的阈值（docs/gamepad.md 第 3 节）。按住时放宽一截（迟滞），推在边上的手指一抖不会一按一松。
constexpr float kStickPressRadius = 0.5f;
constexpr float kStickHoldRadius = 0.35f;
constexpr float kStickHoldCos = 0.57357644f;   // cos 55°：按住某个方向时，偏离那条轴不超过 55°（对角线再过去 10°）就不换
constexpr float kTriggerPressValue = 0.5f;
constexpr float kTriggerHoldValue = 0.3f;
constexpr int kMaxRumbleMs = 1000;

// 这个手柄键配的是哪个动作（LB、LT = 没配）。
[[nodiscard]] std::optional<Engine::Key> padKeyOf(Engine::PadButton button) {
    for (const PadKey& p : kPadKeys) {
        if (p.button == button) return p.key;
    }
    return std::nullopt;
}

// SDL 的手柄键 → PadButton。Xbox 键（Windows 的 Game Bar 要用它）、摇杆按下、分享键、背键、触摸板一律不认。
[[nodiscard]] std::optional<Engine::PadButton> padButtonOf(Uint8 button) {
    switch (button) {
        case SDL_GAMEPAD_BUTTON_SOUTH: return Engine::PadButton::A;
        case SDL_GAMEPAD_BUTTON_EAST: return Engine::PadButton::B;
        case SDL_GAMEPAD_BUTTON_WEST: return Engine::PadButton::X;
        case SDL_GAMEPAD_BUTTON_NORTH: return Engine::PadButton::Y;
        case SDL_GAMEPAD_BUTTON_BACK: return Engine::PadButton::Back;
        case SDL_GAMEPAD_BUTTON_START: return Engine::PadButton::Start;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return Engine::PadButton::LB;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return Engine::PadButton::RB;
        case SDL_GAMEPAD_BUTTON_DPAD_UP: return Engine::PadButton::DpadUp;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return Engine::PadButton::DpadDown;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return Engine::PadButton::DpadLeft;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return Engine::PadButton::DpadRight;
        default: return std::nullopt;
    }
}

// 轴值归一（第 3 节）：摇杆 v / 32767 夹到 [-1, 1]（-32768 → -1），扳机 v / 32767 夹到 [0, 1]。
[[nodiscard]] float stickValue(Sint16 value) {
    return std::clamp(static_cast<float>(value) / 32767.f, -1.f, 1.f);
}
[[nodiscard]] float triggerValue(Sint16 value) {
    return std::clamp(static_cast<float>(value) / 32767.f, 0.f, 1.f);
}

[[nodiscard]] constexpr bool isDirection(Engine::Key key) noexcept {
    return key == Engine::Key::Up || key == Engine::Key::Down || key == Engine::Key::Left || key == Engine::Key::Right;
}

// 震动强度：夹到 [0, 1]（NaN 当 0）再换成 SDL 的 0…0xFFFF。
[[nodiscard]] Uint16 rumbleStrength(float value) {
    const float unit = !(value > 0.f) ? 0.f : std::min(value, 1.f);
    return static_cast<Uint16>(std::lround(unit * 65535.f));
}

[[nodiscard]] bool validCode(ScanCode code) noexcept {
    return code > 0 && code < kScanCodeLimit;
}

// 纯修饰的保留键：抓键时不抓它们（它们多半是 Alt+Enter、Alt+Tab、Win 组合键的前一半，抓走了就会报
// 「留作他用」并结束抓键）。菜单键、Backspace、Delete 虽也是保留键，却是实打实按下去的一个键，照抓。
[[nodiscard]] bool pureModifier(SDL_Scancode code) noexcept {
    return code == SDL_SCANCODE_LALT || code == SDL_SCANCODE_RALT || code == SDL_SCANCODE_LGUI ||
           code == SDL_SCANCODE_RGUI;
}

// 这个动作除了第 slot 格以外还有没有键（固定键，或另一格）。改一格、清空一格的「只剩这一个键」看它。
[[nodiscard]] bool hasAnotherKey(const Engine::KeyTable& table, Engine::Key action, int slot) {
    const auto a = static_cast<std::size_t>(action);
    return !Engine::fixedKeys(action).empty() || table[a][static_cast<std::size_t>(1 - slot)] != 0;
}

bool pathExists(const std::string& path) {
    SDL_PathInfo info{};
    return SDL_GetPathInfo(path.c_str(), &info) && info.type != SDL_PATHTYPE_NONE;
}

// 资源根目录。测试与 CI 的工作目录是 build/，游戏正常启动时是工程根，
// 打包后又是 exe 同级，所以从当前目录和 exe 目录各自向上找几层。
std::string findAssetRoot() {
    std::vector<std::string> prefixes{"", "../", "../../", "../../../"};
    if (const char* base = SDL_GetBasePath()) {
        const std::string b = base;
        prefixes.push_back(b);
        prefixes.push_back(b + "../");
        prefixes.push_back(b + "../../");
        prefixes.push_back(b + "../../../");
    }
    for (const std::string& prefix : prefixes) {
        const std::string candidate = prefix + "assets";
        if (pathExists(candidate)) {
            return candidate;
        }
    }
    return {};
}

std::string findFontFile(const std::string& assetRoot) {
    if (!assetRoot.empty()) {
        const std::string dir = assetRoot + "/fonts";
        for (const char* pattern : {"*.ttf", "*.otf", "*.ttc"}) {
            int count = 0;
            char** hits = SDL_GlobDirectory(dir.c_str(), pattern, 0, &count);
            if (hits != nullptr) {
                std::vector<std::string> names;
                names.reserve(static_cast<std::size_t>(count));
                for (int i = 0; i < count; ++i) {
                    names.emplace_back(hits[i]);
                }
                SDL_free(hits);
                if (!names.empty()) {
                    // 排序只为让「目录里有多个字体时选哪个」是确定的，
                    // 而不是取决于文件系统返回顺序。
                    std::sort(names.begin(), names.end());
                    return dir + "/" + names.front();
                }
            }
        }
    }
    // 临时退路：assets/fonts/ 里没有字体时借用系统中文字体。
    // 这样至少不会黑屏，但字形与排版会随机器变化，不能当作正式方案。
    for (const char* sysFont : {"C:/Windows/Fonts/msyh.ttc", "C:/Windows/Fonts/simhei.ttf",
                                "C:/Windows/Fonts/simsun.ttc",
                                "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"}) {
        if (pathExists(sysFont)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "assets/fonts/ 里没有字体，临时回退到系统字体 %s", sysFont);
            return sysFont;
        }
    }
    return {};
}

std::string sdlError() {
    const char* err = SDL_GetError();
    return (err != nullptr && err[0] != '\0') ? std::string(err) : std::string("未知错误");
}

SDL_BlendMode toSdlBlend(BlendMode mode) {
    switch (mode) {
        case BlendMode::None: return SDL_BLENDMODE_NONE;
        case BlendMode::Alpha: return SDL_BLENDMODE_BLEND;
        case BlendMode::Add: return SDL_BLENDMODE_ADD;
        case BlendMode::Mod: return SDL_BLENDMODE_MOD;
        case BlendMode::Mul: return SDL_BLENDMODE_MUL;
    }
    return SDL_BLENDMODE_BLEND;
}

SDL_ScaleMode toSdlScale(ScaleMode mode) {
    return mode == ScaleMode::Pixel ? SDL_SCALEMODE_PIXELART : SDL_SCALEMODE_LINEAR;
}

SDL_FColor toFColor(const Color& c) {
    return SDL_FColor{static_cast<float>(c.r) / 255.f, static_cast<float>(c.g) / 255.f,
                      static_cast<float>(c.b) / 255.f, static_cast<float>(c.a) / 255.f};
}

SDL_FRect toFRect(const RectF& r) {
    return SDL_FRect{r.x, r.y, r.w, r.h};
}

// 纹理的颜色调制、透明度调制与混合模式是**纹理身上的状态**，不是这一次绘制的参数。
// 同一张纹理这次按 Add 画、下次按旧接口画，中间不重设的话旧接口会继承 Add。
// 所以两条绘制路径每次都把三样设满——SDL 在值不变时几乎不花代价。
void applyTextureState(SDL_Texture* texture, const Color& tint, SDL_BlendMode blend) {
    SDL_SetTextureColorMod(texture, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(texture, tint.a);
    SDL_SetTextureBlendMode(texture, blend);
}

}  // namespace

// ---------------------------------------------------------------------------
// Impl：所有 SDL 句柄都住在这里，公共头因此一个 SDL 类型都不用见。
// ---------------------------------------------------------------------------
struct Engine::Impl {
    bool initialised = false;
    bool headless = true;
    bool quit = false;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    std::string assetRoot;
    std::string fontPath;
    std::unordered_map<int, TTF_Font*> fonts;  // 像素字号 → 字体
    // (字号, 描边宽) → 描边字体。描边字体单独开一份（TTF_CopyFont 共享字体文件）：
    // 在同一个 TTF_Font 上来回 TTF_SetFontOutline 会清掉它的字形缓存，
    // 而正文与描边每帧都要交替用。
    std::unordered_map<std::uint64_t, TTF_Font*> outlineFonts;

    // 一张纹理的全部簿记。naturalBlend 是建纹理时 SDL 给的缺省混合模式：
    // 旧的 drawTexture 每次按它重设，于是新接口在同一张纹理上改过的状态
    // 不会漏到旧接口的画法里去（旧画面必须一个像素都不变）。
    struct TextureRecord {
        SDL_Texture* texture = nullptr;
        int w = 0;
        int h = 0;
        bool target = false;
        SDL_BlendMode naturalBlend = SDL_BLENDMODE_BLEND;
        std::string pathKey;   // 按路径缓存的纹理才有；销毁时凭它把缓存项一并摘掉
    };
    std::unordered_map<TextureId, TextureRecord> textures;
    std::unordered_map<std::string, TextureId> texturesByPath;
    TextureId nextTextureId = kInvalidTexture + 1;
    TextureId currentTarget = kInvalidTexture;   // kInvalidTexture = 后缓冲

    // drawGeometry 的顶点转换缓冲。复用而不是每次新建：景深带、粒子每帧都走这里。
    std::vector<SDL_Vertex> vertexScratch;

    struct CachedText {
        SDL_Texture* texture = nullptr;
        int w = 0;
        int h = 0;
        std::uint64_t lastUsedFrame = 0;
    };

    // 缓存键是结构体而不是拼出来的字符串。对白框每帧重画同一句话，拼串意味着
    // 每帧一次堆分配加一次全文 memcpy，而这条开销随文本量线性放大——全本十二万字
    // 对白都要过这里。配上 is_transparent 的哈希与比较，命中缓存时用 string_view
    // 查表，一次分配都不发生。outline 是描边宽度，0 表示正文。
    struct TextKey {
        std::string text;
        int size = 0;
        std::uint32_t rgba = 0;
        int outline = 0;
    };
    struct TextKeyView {
        std::string_view text;
        int size = 0;
        std::uint32_t rgba = 0;
        int outline = 0;
    };
    struct TextKeyHash {
        using is_transparent = void;
        static std::size_t mix(std::string_view text, int size, std::uint32_t rgba,
                               int outline) noexcept {
            std::size_t h = std::hash<std::string_view>{}(text);
            h = h * 1099511628211ULL + static_cast<std::size_t>(static_cast<unsigned>(size));
            h = h * 1099511628211ULL + static_cast<std::size_t>(rgba);
            h = h * 1099511628211ULL + static_cast<std::size_t>(static_cast<unsigned>(outline));
            return h;
        }
        std::size_t operator()(const TextKey& k) const noexcept {
            return mix(k.text, k.size, k.rgba, k.outline);
        }
        std::size_t operator()(const TextKeyView& k) const noexcept {
            return mix(k.text, k.size, k.rgba, k.outline);
        }
    };
    struct TextKeyEqual {
        using is_transparent = void;
        template <typename A, typename B>
        static bool same(const A& a, const B& b) noexcept {
            // 先比字号、颜色与描边：不等时直接短路，省掉整段文本的比较。
            return a.size == b.size && a.rgba == b.rgba && a.outline == b.outline &&
                   std::string_view(a.text) == std::string_view(b.text);
        }
        bool operator()(const TextKey& a, const TextKey& b) const noexcept { return same(a, b); }
        bool operator()(const TextKey& a, const TextKeyView& b) const noexcept {
            return same(a, b);
        }
        bool operator()(const TextKeyView& a, const TextKey& b) const noexcept {
            return same(a, b);
        }
    };
    std::unordered_map<TextKey, CachedText, TextKeyHash, TextKeyEqual> textCache;

    static std::uint32_t packColor(const Color& c) { return packRgba(c); }

    // 原始扫描码的按下状态。逻辑键位由它推导，这样 Enter 与 Z 同时按住时
    // 松开其中一个不会误判成「Confirm 已松开」。
    std::array<bool, static_cast<std::size_t>(SDL_SCANCODE_COUNT)> scancodeDown{};
    std::array<bool, kKeyCount> pressed{};
    std::array<std::uint64_t, kKeyCount> repeatAtMs{};

    // 每个动作的两格自定义键位（固定键在 kFixedKeys，谁也改不了）。只经 setCustomKeys 整张换。
    KeyTable customKeys = kDefaultKeys;
    // 抓键（改键面板）：capturing 期间下一次物理按下被记进 captured，不映射成任何逻辑键。
    bool capturing = false;
    std::optional<ScanCode> captured;

    // 手柄（docs/gamepad.md 第 4 节）：每个手柄一份状态，按 SDL 的实例 id 记。句柄为空 = 不是这里打开的
    // （测试用 SDL_PushEvent 塞的事件，which 不在表里时现建）：不算「接着的手柄」，也不能震。
    struct PadState {
        SDL_Gamepad* handle = nullptr;
        std::array<bool, kPadButtonCount> down{};   // 18 个 PadButton 各自按没按（摇杆四向、扳机由第 3 节的纯函数写进来）
        float stickX = 0.f;                          // 左摇杆两轴的最新值（归一到 [-1, 1]，y 向下为正）
        float stickY = 0.f;
        bool stickFresh = false;                     // 摇杆有新值还没判方向（等这份报告收齐，见 settleStick）
    };
    std::unordered_map<SDL_JoystickID, PadState> pads;
    // 最后一下配了动作的按下来自哪种设备（第 6 节，提示文案跟着它走）；是手柄时记下是哪一个（震动找它）。
    InputDevice lastDevice = InputDevice::Keyboard;
    SDL_JoystickID lastPad = 0;
    bool rumbleOn = true;   // 手柄震动开关（第 8 节，缺省开）

    std::uint64_t lastFrameNs = 0;
    std::uint64_t frameIndex = 0;
    double delta = 0.0;
    double elapsed = 0.0;

    bool audioReady = false;
    bool mixerLibReady = false;
    MIX_Mixer* mixer = nullptr;
    MIX_Track* bgmTrack = nullptr;
    // 正在放的 BGM id。嘉元城南城、西城这类相邻的图共用一首曲子，换图时
    // 同一首再点一次不该从头放——那会让玩家每过一道门就听一遍前奏。
    std::string currentBgm;
    std::unordered_map<std::string, MIX_Audio*> audioCache;
    // 音效轨道池与每条轨道「第几个开始放的」（全忙时抢最早开始的那条）。建不出来的格子是 nullptr。
    std::array<MIX_Track*, kSfxTrackCount> sfxTracks{};
    std::array<std::uint64_t, kSfxTrackCount> sfxStartedAt{};
    std::uint64_t sfxSerial = 0;

    // 系统设置（docs/settings.md 第 4 节）。无头也存值：测试读回的就是这些。
    float bgmGain = 1.f;
    float sfxGain = 1.f;
    bool fullscreen = false;          // 无头时 fullscreen() 读它；有窗口时读窗口的真实状态
    bool integerScale = true;
    bool vsync = true;
    EffectsLevel effects = EffectsLevel::Full;
    std::uint64_t lastPresentNs = 0;  // 上一帧 present 完的时刻，关垂直同步时补睡用

    // 逻辑键「按住」= 键盘上它的键按住，或任一手柄上它的任一手柄键按住（docs/gamepad.md 第 4 节）。
    [[nodiscard]] bool keyHeld(Key key) const {
        for (const FixedKey& f : kFixedKeys) {
            if (f.key == key && scancodeDown[static_cast<std::size_t>(f.scancode)]) {
                return true;
            }
        }
        for (const ScanCode code : customKeys[static_cast<std::size_t>(key)]) {
            if (validCode(code) && scancodeDown[static_cast<std::size_t>(code)]) {
                return true;
            }
        }
        for (const auto& entry : pads) {
            for (const PadKey& p : kPadKeys) {
                if (p.key == key && entry.second.down[static_cast<std::size_t>(p.button)]) return true;
            }
        }
        return false;
    }

    // 记一次「刚按下」——键盘的一个物理键、手柄的一个手柄键从没按到按下，都走这一处（docs/gamepad.md 第 5、6 节的单点）：
    //   · 本帧算「刚按下」。**只有方向键设重复计时**（按住 250ms 后每 60ms 再出一次）：确认、取消、菜单、快进、存盘、
    //     路径行动一次物理按下只出一次——否则按住 Tab / Esc 时，世界层见「刚按下」开主菜单、主菜单见「刚按下」合上，
    //     菜单每 60ms 开合一次；按住 F5 每 60ms 存一次盘。
    //   · 设备跟着这一下走（手柄还记下是哪一个，震动找它）。
    void press(Key key, InputDevice from, SDL_JoystickID pad) {
        const auto k = static_cast<std::size_t>(key);
        pressed[k] = true;
        repeatAtMs[k] = isDirection(key) ? SDL_GetTicks() + kKeyRepeatDelayMs : 0;
        lastDevice = from;
        if (from == InputDevice::Gamepad) lastPad = pad;
    }

    // 物理键 sc 刚按下：它是哪几个动作的键（固定或自定义），那几个动作各记一次「刚按下」。没配动作的键（Alt、Win、
    // 媒体键……）什么也不记，设备也不切。
    void pressScancode(ScanCode sc) {
        for (const FixedKey& f : kFixedKeys) {
            if (f.scancode == sc) press(f.key, InputDevice::Keyboard, 0);
        }
        for (std::size_t k = 0; k < kKeyCount; ++k) {
            for (const ScanCode code : customKeys[k]) {
                // 0 是空格位，不是一个键：不设这道闸，扫描码 0 会与表里所有空格位一起命中。
                if (validCode(code) && code == sc) press(static_cast<Key>(k), InputDevice::Keyboard, 0);
            }
        }
    }

    // ---- 手柄（docs/gamepad.md 第 4、7 节）----

    // 某个手柄的一个 PadButton 变成 down。从没按到按下才算一次按下：同一个值重复来（SDL 补发轴初值时就这样）不算。
    void setPadButton(SDL_JoystickID which, PadState& pad, PadButton button, bool down) {
        bool& slot = pad.down[static_cast<std::size_t>(button)];
        if (slot == down) return;
        slot = down;
        if (!down) return;
        if (capturing) {
            // 抓键（等玩家在键盘上按新键）：手柄 B = 作罢，交出 Esc 的扫描码，改键面板照「作罢」那条路走——
            // 不然只拿手柄的玩家按 A 进了「按下新键」就出不来。别的手柄键一律不理、抓键继续；按下的物理状态上面照记了
            // （松开时对得上），只是不出逻辑键。
            if (button == PadButton::B) {
                capturing = false;
                captured = kEscapeCode;
                lastDevice = InputDevice::Gamepad;
                lastPad = which;
            }
            return;
        }
        if (const std::optional<Key> key = padKeyOf(button)) press(*key, InputDevice::Gamepad, which);
    }

    // 左摇杆的最新两轴 → 四向（第 3 节的纯函数）。先松开旧方向、再按下新方向：换向不经中心时两件事在同一个事件里发生。
    void updateStick(SDL_JoystickID which, PadState& pad) {
        std::optional<Key> held;
        for (const PadKey& way : kStickWays) {
            if (pad.down[static_cast<std::size_t>(way.button)]) held = way.key;
        }
        const std::optional<Key> now = Engine::stickDirection(pad.stickX, pad.stickY, held);
        for (const PadKey& way : kStickWays) {
            if (way.key != now) setPadButton(which, pad, way.button, false);
        }
        for (const PadKey& way : kStickWays) {
            if (way.key == now) setPadButton(which, pad, way.button, true);
        }
    }

    // 摇杆按「一份完整的报告」判方向，不按单个轴事件判（docs/gamepad.md 第 4 节，整改轮 HIGH-1）：SDL 一份报告里先发 LEFTX、
    // 再发 LEFTY、最后发 GAMEPAD_UPDATE_COMPLETE；逐个轴事件判的话，两个事件之间是「新 X + 旧 Y」的半截状态——斜推后松手，
    // X 先归零那一刻是 (0, 0.5)，偏离右轴 90°，凭空判出一次「下」。所以轴事件只存值、记下「有新值」，由 settleStick 判：
    // 报告末尾的 UPDATE_COMPLETE 一次，一帧的事件处理完之后还有新值没判的再一次（SDL_PushEvent 塞的事件没有报告末尾的标记）。
    void padAxis(SDL_JoystickID which, Uint8 axis, Sint16 value) {
        switch (axis) {
            case SDL_GAMEPAD_AXIS_LEFTX:
            case SDL_GAMEPAD_AXIS_LEFTY: {
                PadState& pad = pads[which];   // 不在表里（SDL_PushEvent 塞的）：现建一份，句柄为空
                (axis == SDL_GAMEPAD_AXIS_LEFTX ? pad.stickX : pad.stickY) = stickValue(value);
                pad.stickFresh = true;
                break;
            }
            case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
            case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: {
                // 扳机是单轴：没有半截状态，收到就判。
                PadState& pad = pads[which];
                const PadButton trigger = axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER ? PadButton::LT : PadButton::RT;
                const bool wasDown = pad.down[static_cast<std::size_t>(trigger)];
                setPadButton(which, pad, trigger, Engine::triggerDown(triggerValue(value), wasDown));
                break;
            }
            default:
                break;   // 右摇杆、越界的轴：不配动作，也不建状态
        }
    }

    // 这个手柄的摇杆有新值没判：按第 3 节判一次，清掉标记。UPDATE_COMPLETE 与一帧收尾走的都是它。
    void settleStick(SDL_JoystickID which, PadState& pad) {
        if (!pad.stickFresh) return;
        pad.stickFresh = false;
        updateStick(which, pad);
    }

    // 已经插着的手柄在子系统启动时 SDL 会补发 ADDED（第 0 节探针实测），所以只在这里开，不另外枚举。
    void openPad(SDL_JoystickID which) {
        const auto it = pads.find(which);
        if (it != pads.end() && it->second.handle != nullptr) return;   // 同一个手柄的 ADDED 又来一次：不开第二次
        SDL_Gamepad* handle = SDL_OpenGamepad(which);
        if (handle == nullptr) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "打开手柄失败（实例 %u），跳过：%s", static_cast<unsigned>(which),
                        sdlError().c_str());
            return;
        }
        pads[which].handle = handle;
    }

    // 拔掉：关句柄、删掉那个手柄的状态（它按着的一切随之松开）。拔掉的是最后一个手柄、且眼下的设备是手柄 → 回到键盘。
    void closePad(SDL_JoystickID which) {
        const auto it = pads.find(which);
        if (it != pads.end()) {
            if (it->second.handle != nullptr) SDL_CloseGamepad(it->second.handle);
            pads.erase(it);
        }
        if (lastDevice == InputDevice::Gamepad && openPadCount() == 0) lastDevice = InputDevice::Keyboard;
    }

    [[nodiscard]] int openPadCount() const {
        return static_cast<int>(std::count_if(pads.begin(), pads.end(),
                                              [](const auto& entry) { return entry.second.handle != nullptr; }));
    }

    // Alt+Enter（主键盘与小键盘的 Enter 都认）。Alt 以事件自带的修饰键状态为准：SDL 按键盘的真实状态填它，
    // Alt 在窗口拿到焦点之前就按下了也算数。
    [[nodiscard]] static bool altEnter(const SDL_KeyboardEvent& key) {
        return (key.scancode == SDL_SCANCODE_RETURN || key.scancode == SDL_SCANCODE_KP_ENTER) &&
               (key.mod & SDL_KMOD_ALT) != 0;
    }

    // 取一条放音效的轨道：先找空闲的（没在放的）；全忙就抢最早开始的那条——它多半已经放到尾音了。
    MIX_Track* takeSfxTrack() {
        std::size_t pick = kSfxTrackCount;
        for (std::size_t i = 0; i < kSfxTrackCount; ++i) {
            if (sfxTracks[i] != nullptr && !MIX_TrackPlaying(sfxTracks[i])) {
                pick = i;
                break;
            }
        }
        if (pick == kSfxTrackCount) {
            for (std::size_t i = 0; i < kSfxTrackCount; ++i) {
                if (sfxTracks[i] == nullptr) continue;
                if (pick == kSfxTrackCount || sfxStartedAt[i] < sfxStartedAt[pick]) pick = i;
            }
        }
        if (pick == kSfxTrackCount) return nullptr;
        sfxStartedAt[pick] = ++sfxSerial;
        return sfxTracks[pick];
    }

    // 关了垂直同步：present 之后补睡到 1/240 秒（docs/settings.md 4.2）。开着时只记时刻。
    // 用 SDL_DelayNS（让出 CPU）而不是 SDL_DelayPrecise：后者按头文件的说法最后一截是忙等，与封顶的本意相反。
    void capFrameRate() {
        const std::uint64_t now = SDL_GetTicksNS();
        if (!vsync && lastPresentNs != 0 && now > lastPresentNs && now - lastPresentNs < kUncappedFrameNs) {
            SDL_DelayNS(kUncappedFrameNs - (now - lastPresentNs));
        }
        lastPresentNs = SDL_GetTicksNS();
    }

    TTF_Font* fontFor(int size) {
        if (fontPath.empty() || size <= 0) {
            return nullptr;
        }
        const auto it = fonts.find(size);
        if (it != fonts.end()) {
            return it->second;
        }
        // 每个字号单独开一个 TTF_Font，而不是对同一个字体反复 TTF_SetFontSize：
        // 改字号会清掉字形缓存，而对话框 + HUD 每帧都在两三个字号之间来回切。
        TTF_Font* font = TTF_OpenFont(fontPath.c_str(), static_cast<float>(size));
        if (font == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "打开字体 %s（%d px）失败：%s",
                         fontPath.c_str(), size, sdlError().c_str());
        }
        fonts.emplace(size, font);  // 失败也记下来，避免每帧重试一个开不了的字体
        return font;
    }

    TTF_Font* outlineFontFor(int size, int width) {
        const std::uint64_t key = (static_cast<std::uint64_t>(static_cast<unsigned>(size)) << 32) |
                                  static_cast<unsigned>(width);
        const auto it = outlineFonts.find(key);
        if (it != outlineFonts.end()) {
            return it->second;
        }
        TTF_Font* font = nullptr;
        if (TTF_Font* base = fontFor(size)) {
            font = TTF_CopyFont(base);
            if (font != nullptr && !TTF_SetFontOutline(font, width)) {
                TTF_CloseFont(font);
                font = nullptr;
            }
            if (font == nullptr) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "建描边字体（%d px，描边 %d）失败：%s",
                             size, width, sdlError().c_str());
            }
        }
        outlineFonts.emplace(key, font);  // 同 fontFor：失败也记，不每帧重试
        return font;
    }

    // 资源 id 解析成实际路径：先当作已经是可用路径，再按资源根拼。
    [[nodiscard]] std::string resolve(const std::string& subdir, const std::string& id,
                                      const std::vector<const char*>& extensions) const {
        if (pathExists(id)) {
            return id;
        }
        if (assetRoot.empty()) {
            return {};
        }
        const std::string stem = assetRoot + "/" + subdir + "/" + id;
        if (pathExists(stem)) {
            return stem;
        }
        for (const char* ext : extensions) {
            const std::string candidate = stem + ext;
            if (pathExists(candidate)) {
                return candidate;
            }
        }
        return {};
    }

    // 图片路径：原样可用 → 资源根下的相对路径 → 资源根下 textures/。
    // 中间那一档是给 assets/art/** 的：游戏从工程根启动、测试从 build-xxx/ 启动，
    // 「art/fx/glow.png」这种写法两边都得认得。
    [[nodiscard]] std::string resolveImage(const std::string& path) const {
        if (pathExists(path)) {
            return path;
        }
        if (!assetRoot.empty() && pathExists(assetRoot + "/" + path)) {
            return assetRoot + "/" + path;
        }
        std::string resolved = resolve("textures", path, {});
        return resolved.empty() ? path : resolved;  // 找不到交给 SDL_image 报具体错误
    }

    MIX_Audio* audioFor(const std::string& subdir, const std::string& id) {
        const std::string key = subdir + "/" + id;
        const auto it = audioCache.find(key);
        if (it != audioCache.end()) {
            return it->second;
        }
        const std::string path = resolve(subdir, id, {".ogg", ".mp3", ".wav", ".flac"});
        MIX_Audio* audio = nullptr;
        if (path.empty()) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "找不到音频资源 %s", key.c_str());
        } else {
            // predecode=true：音效要即时发声，边播边解会有首次触发的卡顿。
            audio = MIX_LoadAudio(mixer, path.c_str(), subdir == "sfx");
            if (audio == nullptr) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "载入音频 %s 失败：%s", path.c_str(),
                            sdlError().c_str());
            }
        }
        audioCache.emplace(key, audio);  // 失败也缓存 nullptr，不每帧重试
        return audio;
    }

    void sweepTextCache() {
        for (auto it = textCache.begin(); it != textCache.end();) {
            if (it->second.lastUsedFrame + kTextCacheTtlFrames < frameIndex) {
                SDL_DestroyTexture(it->second.texture);
                it = textCache.erase(it);
            } else {
                ++it;
            }
        }
    }

    // 一行字（正文或描边层）的缓存纹理；字体开不了、渲染失败返回 nullptr。
    // 返回的指针在下一次 sweepTextCache（endFrame 里）之前一直有效：
    // unordered_map 的元素地址不随插入与重哈希变动。
    CachedText* textFor(const std::string& utf8, int size, const Color& color, int outline) {
        TTF_Font* font = outline > 0 ? outlineFontFor(size, outline) : fontFor(size);
        if (font == nullptr) {
            return nullptr;
        }
        const std::uint32_t rgba = packColor(color);
        // 查表走 string_view，不复制文本；只有真正要新建纹理时才构造持有文本的键。
        auto it = textCache.find(TextKeyView{utf8, size, rgba, outline});
        if (it == textCache.end()) {
            const SDL_Color fg{color.r, color.g, color.b, color.a};
            SDL_Surface* surface = TTF_RenderText_Blended(font, utf8.c_str(), utf8.size(), fg);
            if (surface == nullptr) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "渲染文字失败：%s", sdlError().c_str());
                return nullptr;
            }
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            const int w = surface->w;
            const int h = surface->h;
            SDL_DestroySurface(surface);
            if (texture == nullptr) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "文字转纹理失败：%s",
                            sdlError().c_str());
                return nullptr;
            }
            it = textCache
                     .emplace(TextKey{utf8, size, rgba, outline}, CachedText{texture, w, h, 0})
                     .first;
        }
        it->second.lastUsedFrame = frameIndex;
        return &it->second;
    }

    void blitText(const CachedText& text, int x, int y) const {
        const SDL_FRect dst{static_cast<float>(x), static_cast<float>(y),
                            static_cast<float>(text.w), static_cast<float>(text.h)};
        SDL_RenderTexture(renderer, text.texture, nullptr, &dst);
    }

    [[nodiscard]] const TextureRecord* record(TextureId id) const {
        if (id == kInvalidTexture) {
            return nullptr;
        }
        const auto it = textures.find(id);
        return it == textures.end() ? nullptr : &it->second;
    }

    // 登记一张刚建好的 SDL 纹理，发一个句柄。pathKey 非空时一并记进路径缓存。
    TextureId registerTexture(SDL_Texture* texture, bool target, std::string pathKey) {
        float w = 0.f;
        float h = 0.f;
        SDL_GetTextureSize(texture, &w, &h);
        SDL_BlendMode blend = SDL_BLENDMODE_BLEND;
        SDL_GetTextureBlendMode(texture, &blend);
        const TextureId id = nextTextureId++;
        if (!pathKey.empty()) {
            texturesByPath.emplace(pathKey, id);
        }
        textures.emplace(id, TextureRecord{texture, static_cast<int>(w), static_cast<int>(h),
                                           target, blend, std::move(pathKey)});
        return id;
    }

    [[nodiscard]] SDL_Texture* targetTexture(TextureId id) const {
        const TextureRecord* rec = record(id);
        return rec != nullptr ? rec->texture : nullptr;
    }

    // 作用域内临时切回后缓冲，出作用域恢复原目标。抓帧、读后缓冲都要读「屏幕上那一张」，
    // 而调用方可能正绑着一张离屏目标——那时读到的是离屏目标，不是玩家看见的画面。
    class BackbufferScope {
    public:
        explicit BackbufferScope(Impl& owner) : owner_(owner), previous_(owner.currentTarget) {
            if (previous_ != kInvalidTexture) {
                SDL_SetRenderTarget(owner_.renderer, nullptr);
            }
        }
        ~BackbufferScope() {
            if (previous_ != kInvalidTexture) {
                SDL_SetRenderTarget(owner_.renderer, owner_.targetTexture(previous_));
            }
        }
        BackbufferScope(const BackbufferScope&) = delete;
        BackbufferScope& operator=(const BackbufferScope&) = delete;

    private:
        Impl& owner_;
        TextureId previous_;
    };

    // 读后缓冲里「内容」那一块。整数缩放的窗口比 1280×720 大时两侧有黑边，
    // 按 SDL 的说明回读的是屏幕上的实际像素，内容区要按逻辑呈现矩形裁出来；
    // 回读结果已经就是内容区（尺寸对得上）时原样返回。
    [[nodiscard]] SDL_Surface* readBackbufferContent() const {
        SDL_Surface* full = SDL_RenderReadPixels(renderer, nullptr);
        if (full == nullptr) {
            return nullptr;
        }
        SDL_FRect content{};
        if (!SDL_GetRenderLogicalPresentationRect(renderer, &content)) {
            return full;
        }
        const SDL_Rect crop{static_cast<int>(content.x), static_cast<int>(content.y),
                            static_cast<int>(content.w), static_cast<int>(content.h)};
        const bool alreadyContent = crop.w == full->w && crop.h == full->h;
        const bool fits = crop.w > 0 && crop.h > 0 && crop.x >= 0 && crop.y >= 0 &&
                          crop.x + crop.w <= full->w && crop.y + crop.h <= full->h;
        if (alreadyContent || !fits) {
            return full;
        }
        SDL_Surface* cropped = SDL_CreateSurface(crop.w, crop.h, full->format);
        if (cropped != nullptr) {
            SDL_SetSurfaceBlendMode(full, SDL_BLENDMODE_NONE);
            SDL_BlitSurface(full, &crop, cropped, nullptr);
        }
        SDL_DestroySurface(full);
        return cropped;
    }

    // 四角各一色的矩形。渐变的两个公开接口都落到这里。
    void drawQuad(const RectF& r, const Color& tl, const Color& tr, const Color& br,
                  const Color& bl, BlendMode blend) const {
        const SDL_Vertex v[4] = {
            {{r.x, r.y}, toFColor(tl), {0.f, 0.f}},
            {{r.x + r.w, r.y}, toFColor(tr), {1.f, 0.f}},
            {{r.x + r.w, r.y + r.h}, toFColor(br), {1.f, 1.f}},
            {{r.x, r.y + r.h}, toFColor(bl), {0.f, 1.f}},
        };
        static constexpr int kQuadIndices[6] = {0, 1, 2, 0, 2, 3};
        // 无纹理的几何按「画笔」的混合模式混合（有纹理时才看纹理的）。
        SDL_SetRenderDrawBlendMode(renderer, toSdlBlend(blend));
        SDL_RenderGeometry(renderer, nullptr, v, 4, kQuadIndices, 6);
    }
};

Engine::Engine() : impl_(std::make_unique<Impl>()) {}

Engine::~Engine() {
    shutdown();
}

core::Result<bool> Engine::init(const std::string& title, bool headless) {
    if (impl_->initialised) {
        return core::Result<bool>::failure("Engine::init 重复调用");
    }
    impl_->headless = headless;

    // 无头模式只要事件子系统：CI 机器上没有显示设备，拉起 VIDEO 会直接失败。
    const SDL_InitFlags flags =
        headless ? SDL_INIT_EVENTS : (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);
    if (!SDL_Init(flags)) {
        return core::Result<bool>::failure("SDL_Init 失败：" + sdlError());
    }
    if (!TTF_Init()) {
        const std::string err = sdlError();
        SDL_Quit();
        return core::Result<bool>::failure("TTF_Init 失败：" + err);
    }

    impl_->assetRoot = findAssetRoot();

    // 字体在无头模式下同样要加载：measureText 是排版的地基，
    // 无头 bot 与 CI 都靠它算对话框要断几行。
    impl_->fontPath = findFontFile(impl_->assetRoot);
    if (impl_->fontPath.empty()) {
        TTF_Quit();
        SDL_Quit();
        return core::Result<bool>::failure(
            "找不到字体：assets/fonts/ 下没有 ttf/otf/ttc，系统字体也不可用");
    }
    if (impl_->fontFor(16) == nullptr) {
        const std::string path = impl_->fontPath;
        impl_->fonts.clear();
        TTF_Quit();
        SDL_Quit();
        return core::Result<bool>::failure("字体 " + path + " 无法打开");
    }

    if (!headless) {
        // init 之前设过的系统设置（全屏、缩放、垂直同步、音量、特效档位）一律在这里生效，口径统一；
        // 缺省值全与改造前相同（窗口、整数倍、垂直同步开、增益 1）。
        const SDL_WindowFlags windowFlags =
            SDL_WINDOW_RESIZABLE | (impl_->fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WindowFlags{0});
        impl_->window = SDL_CreateWindow(title.c_str(), kLogicalWidth, kLogicalHeight, windowFlags);
        if (impl_->window == nullptr) {
            const std::string err = sdlError();
            shutdown();
            return core::Result<bool>::failure("创建窗口失败：" + err);
        }
        impl_->renderer = SDL_CreateRenderer(impl_->window, nullptr);
        if (impl_->renderer == nullptr) {
            const std::string err = sdlError();
            shutdown();
            return core::Result<bool>::failure("创建渲染器失败：" + err);
        }
        // 缺省整数缩放、垂直同步开（与改造前相同）；init 之前就设过的按设过的来。
        SDL_SetRenderLogicalPresentation(impl_->renderer, kLogicalWidth, kLogicalHeight,
                                         presentationFor(impl_->integerScale));
        SDL_SetRenderVSync(impl_->renderer, impl_->vsync ? 1 : 0);

        // 音频起不来不算致命错误：没声音也要能继续玩。
        if (MIX_Init()) {
            impl_->mixerLibReady = true;
            impl_->mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
            if (impl_->mixer != nullptr) {
                impl_->bgmTrack = MIX_CreateTrack(impl_->mixer);
                impl_->audioReady = impl_->bgmTrack != nullptr;
            }
            if (!impl_->audioReady) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "音频初始化失败，将静音运行：%s",
                            sdlError().c_str());
            } else {
                MIX_SetTrackGain(impl_->bgmTrack, impl_->bgmGain);
                std::size_t built = 0;
                for (MIX_Track*& track : impl_->sfxTracks) {
                    track = MIX_CreateTrack(impl_->mixer);
                    if (track == nullptr) continue;
                    MIX_SetTrackGain(track, impl_->sfxGain);
                    ++built;
                }
                if (built < kSfxTrackCount) {
                    // 少几条只是同时响的音效少几个；一条都没有就是没有音效，照样能玩。
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "音效轨道只建成 %zu / %zu 条：%s", built,
                                kSfxTrackCount, sdlError().c_str());
                }
            }
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "MIX_Init 失败，将静音运行：%s",
                        sdlError().c_str());
        }

        // 手柄（docs/gamepad.md 第 4 节）：起不来只打一行警告、照样启动，与音频同一个口径。已经插着的手柄，子系统一开
        // SDL 就补发 GAMEPAD_ADDED，由 pollEvents 打开，这里不另外枚举（两处都开会把同一个手柄开两次）。
        // 无头不开：几百条无头测试与 --headless 机器人因此不受开发机上插着的手柄影响。
        if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "手柄子系统起不来，只能用键盘：%s", sdlError().c_str());
        }
    }

    impl_->lastFrameNs = SDL_GetTicksNS();
    impl_->elapsed = 0.0;
    impl_->initialised = true;
    return core::Result<bool>::success(true);
}

void Engine::shutdown() {
    if (impl_->mixerLibReady) {
        for (auto& entry : impl_->audioCache) {
            if (entry.second != nullptr) {
                MIX_DestroyAudio(entry.second);
            }
        }
        if (impl_->bgmTrack != nullptr) {
            MIX_DestroyTrack(impl_->bgmTrack);
        }
        for (MIX_Track* track : impl_->sfxTracks) {
            if (track != nullptr) {
                MIX_DestroyTrack(track);
            }
        }
        if (impl_->mixer != nullptr) {
            MIX_DestroyMixer(impl_->mixer);
        }
        MIX_Quit();
    }
    impl_->audioCache.clear();
    impl_->bgmTrack = nullptr;
    impl_->sfxTracks.fill(nullptr);
    impl_->sfxStartedAt.fill(0);
    impl_->sfxSerial = 0;
    impl_->mixer = nullptr;
    impl_->mixerLibReady = false;
    impl_->audioReady = false;

    for (auto& entry : impl_->textCache) {
        SDL_DestroyTexture(entry.second.texture);
    }
    impl_->textCache.clear();
    for (auto& entry : impl_->textures) {
        SDL_DestroyTexture(entry.second.texture);
    }
    impl_->textures.clear();
    impl_->texturesByPath.clear();
    impl_->currentTarget = kInvalidTexture;
    impl_->vertexScratch.clear();

    // 描边字体是正文字体的副本（共享字体文件），先关副本再关本体。
    for (auto& entry : impl_->outlineFonts) {
        if (entry.second != nullptr) {
            TTF_CloseFont(entry.second);
        }
    }
    impl_->outlineFonts.clear();
    for (auto& entry : impl_->fonts) {
        if (entry.second != nullptr) {
            TTF_CloseFont(entry.second);
        }
    }
    const bool hadFonts = !impl_->fonts.empty();
    impl_->fonts.clear();
    // 清掉字体路径，fontFor() 才会立刻放弃。否则 shutdown 之后误调 measureText
    // 会在 TTF_Quit() 之后再去 TTF_OpenFont，每次都要走一遍失败路径刷日志。
    impl_->fontPath.clear();
    impl_->assetRoot.clear();

    if (impl_->renderer != nullptr) {
        SDL_DestroyRenderer(impl_->renderer);
        impl_->renderer = nullptr;
    }
    if (impl_->window != nullptr) {
        SDL_DestroyWindow(impl_->window);
        impl_->window = nullptr;
    }

    // 手柄：句柄在 SDL_Quit 之前关，状态清空，设备回到键盘（docs/gamepad.md 第 9 节）。
    for (auto& entry : impl_->pads) {
        if (entry.second.handle != nullptr) SDL_CloseGamepad(entry.second.handle);
    }
    impl_->pads.clear();
    impl_->lastDevice = InputDevice::Keyboard;
    impl_->lastPad = 0;

    if (impl_->initialised || hadFonts) {
        TTF_Quit();
        SDL_Quit();
    }
    impl_->initialised = false;
    impl_->quit = false;
    impl_->scancodeDown.fill(false);
    impl_->pressed.fill(false);
    impl_->repeatAtMs.fill(0);
    impl_->capturing = false;
    impl_->captured.reset();
}

void Engine::pollEvents() {
    impl_->pressed.fill(false);
    if (!impl_->initialised) {
        return;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                impl_->quit = true;
                break;
            case SDL_EVENT_KEY_DOWN: {
                // 系统自带的重复速率在各机器上不一样，这里只认「物理按下」，
                // 重复由下面的节流统一产生，手感才可控。
                if (event.key.repeat) {
                    break;
                }
                const auto sc = static_cast<std::size_t>(event.key.scancode);
                // 没映射的键（媒体键、启动键、笔记本 Fn 组合、厂商宏键）SDL 发扫描码 0（SDL_SCANCODE_UNKNOWN）、raw ≠ 0。
                // 0 在键位表里是「空格位」：放进来就与八个空格位一起命中，同一帧八个逻辑键全按下（世界层先判存盘，
                // 悄悄覆盖 quick.sav）。与越界一起挡在最前面：Alt+Enter、抓键都不认它。
                if (event.key.scancode == SDL_SCANCODE_UNKNOWN || sc >= static_cast<std::size_t>(SDL_SCANCODE_COUNT)) {
                    break;
                }
                // Alt+Enter 切全屏（docs/settings.md 4.2），并**吞掉这次按键**：不记 scancodeDown，
                // 也就不出 Confirm——记了的话，按住 250ms 之后下面的节流还会自动重复出 Confirm。
                if (Impl::altEnter(event.key)) {
                    setFullscreen(!fullscreen());
                    break;
                }
                // 抓键（改键面板）：这一下交给 takeCapturedKey，不映射成任何逻辑键，也不记按下——
                // 记了的话，它刚被配给哪个动作，那个动作就成了「按住」。
                if (impl_->capturing) {
                    // 纯修饰的保留键（左右 Alt、左右 Win）放过：照常记按下、不结束抓键——Alt+Enter 切全屏、
                    // Alt+Tab 切窗口时先到的那一下 Alt 不该被当成「玩家要配的键」。
                    if (pureModifier(event.key.scancode)) {
                        impl_->scancodeDown[sc] = true;
                        break;
                    }
                    // 按着 Alt / Win 按下去的键也一律不抓、不结束抓键：那是组合键（Alt+F4 关窗、Alt+空格 系统菜单、
                    // Win+D 之类），本来也配不了。抓走的话，Alt+F4 退出的同一下 F4 就被配进了当前格、退出时还写进盘里。
                    if ((event.key.mod & (SDL_KMOD_ALT | SDL_KMOD_GUI)) != 0) {
                        break;
                    }
                    impl_->capturing = false;
                    impl_->captured = static_cast<ScanCode>(sc);
                    impl_->lastDevice = InputDevice::Keyboard;   // 被抓走的那一下键盘键：设备是键盘（docs/gamepad.md 第 6 节）
                    break;
                }
                impl_->scancodeDown[sc] = true;
                impl_->pressScancode(static_cast<ScanCode>(sc));
                break;
            }
            case SDL_EVENT_KEY_UP: {
                const auto sc = static_cast<std::size_t>(event.key.scancode);
                if (sc < static_cast<std::size_t>(SDL_SCANCODE_COUNT)) {
                    impl_->scancodeDown[sc] = false;
                }
                break;
            }
            // 手柄（docs/gamepad.md 第 4 节）。不看是谁开的手柄子系统：测试自己开、或用 SDL_PushEvent 塞，走的都是这一段。
            case SDL_EVENT_GAMEPAD_ADDED:
                impl_->openPad(event.gdevice.which);
                break;
            case SDL_EVENT_GAMEPAD_REMOVED:
                impl_->closePad(event.gdevice.which);
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                if (const std::optional<PadButton> button = padButtonOf(event.gbutton.button)) {
                    impl_->setPadButton(event.gbutton.which, impl_->pads[event.gbutton.which], *button,
                                        event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
                }
                break;
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                impl_->padAxis(event.gaxis.which, event.gaxis.axis, event.gaxis.value);
                break;
            case SDL_EVENT_GAMEPAD_UPDATE_COMPLETE:
                // 一份报告收齐了：摇杆这才按完整的两轴判方向（见 Impl::padAxis）。
                if (const auto pad = impl_->pads.find(event.gdevice.which); pad != impl_->pads.end()) {
                    impl_->settleStick(pad->first, pad->second);
                }
                break;
            default:
                break;
        }
    }
    // 一帧的事件处理完：还有新值没判的摇杆再判一次（SDL_PushEvent 塞的事件没有报告末尾的 UPDATE_COMPLETE）。
    // 排在抓键那道闸之前：抓键期间摇杆的物理状态照记，只是不出逻辑键。
    for (auto& entry : impl_->pads) impl_->settleStick(entry.first, entry.second);

    // 抓键期间不出任何逻辑键，连按住的键的自动重复也不出（重复计时清零：抓完之后要再按一下才算）。
    if (impl_->capturing) {
        impl_->pressed.fill(false);
        impl_->repeatAtMs.fill(0);
        return;
    }

    const std::uint64_t now = SDL_GetTicks();
    for (std::size_t k = 0; k < kKeyCount; ++k) {
        const bool held = impl_->keyHeld(static_cast<Key>(k));
        if (!held) {
            impl_->repeatAtMs[k] = 0;
            continue;
        }
        if (impl_->repeatAtMs[k] != 0 && now >= impl_->repeatAtMs[k]) {
            impl_->pressed[k] = true;
            impl_->repeatAtMs[k] = now + kKeyRepeatIntervalMs;
        }
    }
}

bool Engine::shouldQuit() const {
    return impl_->quit;
}

void Engine::beginFrame() {
    const std::uint64_t now = SDL_GetTicksNS();
    const std::uint64_t elapsed = now > impl_->lastFrameNs ? now - impl_->lastFrameNs : 0;
    impl_->lastFrameNs = now;
    impl_->delta = std::min(static_cast<double>(elapsed) / 1.0e9, kMaxDeltaSeconds);
    impl_->elapsed += impl_->delta;
    ++impl_->frameIndex;

    if (impl_->renderer != nullptr) {
        if (impl_->currentTarget != kInvalidTexture) {
            SDL_SetRenderTarget(impl_->renderer, nullptr);
            impl_->currentTarget = kInvalidTexture;
        }
        SDL_SetRenderClipRect(impl_->renderer, nullptr);
        SDL_SetRenderDrawColor(impl_->renderer, 0, 0, 0, 255);
        SDL_RenderClear(impl_->renderer);
    }
}

void Engine::endFrame() {
    if (impl_->renderer != nullptr) {
        if (impl_->currentTarget != kInvalidTexture) {
            SDL_SetRenderTarget(impl_->renderer, nullptr);
            impl_->currentTarget = kInvalidTexture;
        }
        SDL_RenderPresent(impl_->renderer);
        impl_->capFrameRate();
    }
    if (impl_->frameIndex % kTextCacheSweepFrames == 0) {
        impl_->sweepTextCache();
    }
}

double Engine::deltaSeconds() const {
    return impl_->delta;
}

double Engine::elapsedSeconds() const {
    return impl_->elapsed;
}

core::Result<bool> Engine::captureFrame(const std::string& path) {
    if (impl_->renderer == nullptr) {
        return core::Result<bool>::failure("无头模式没有渲染器，抓不了帧");
    }
    SDL_Surface* shot = nullptr;
    {
        const Impl::BackbufferScope onBackbuffer(*impl_);
        shot = SDL_RenderReadPixels(impl_->renderer, nullptr);
    }
    if (shot == nullptr) {
        return core::Result<bool>::failure("读取帧缓冲失败：" + sdlError());
    }
    // 存 PNG 而不是 BMP：一帧 1280x720 的 BMP 是 3.5MB，而这些图的用途就是
    // 被人（或被工具）一张张打开看。IMG_SavePNG 随 SDL3_image 一起已经链进来了。
    const bool saved = IMG_SavePNG(shot, path.c_str());
    const std::string err = saved ? std::string{} : sdlError();
    SDL_DestroySurface(shot);
    if (!saved) {
        return core::Result<bool>::failure("写 " + path + " 失败：" + err);
    }
    return core::Result<bool>::success(true);
}

TextureId Engine::loadTexture(const std::string& path) {
    return loadTexture(path, ScaleMode::Pixel);
}

TextureId Engine::loadTexture(const std::string& path, ScaleMode mode) {
    if (impl_->renderer == nullptr) {
        // 无头模式没有渲染器，纹理无从谈起。返回无效句柄而不是失败，
        // 让上层的绘制代码可以原样跑过去。
        return kInvalidTexture;
    }
    // 取样方式是纹理的属性，同一张图按两种方式要就得是两张纹理。
    const std::string key = mode == ScaleMode::Pixel ? path : path + "#linear";
    const auto cached = impl_->texturesByPath.find(key);
    if (cached != impl_->texturesByPath.end()) {
        return cached->second;
    }

    const std::string resolved = impl_->resolveImage(path);
    SDL_Texture* texture = IMG_LoadTexture(impl_->renderer, resolved.c_str());
    if (texture == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "载入纹理 %s 失败：%s", path.c_str(),
                    sdlError().c_str());
        impl_->texturesByPath.emplace(key, kInvalidTexture);
        return kInvalidTexture;
    }
    SDL_SetTextureScaleMode(texture, toSdlScale(mode));
    return impl_->registerTexture(texture, false, key);
}

TextureId Engine::createTexture(int w, int h, const std::vector<std::uint32_t>& rgba,
                                ScaleMode mode) {
    if (impl_->renderer == nullptr) {
        return kInvalidTexture;
    }
    if (w <= 0 || h <= 0 ||
        rgba.size() != static_cast<std::size_t>(w) * static_cast<std::size_t>(h)) {
        // 尺寸对不上就是越界读，不能交给 SDL 去猜。
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "createTexture：%dx%d 需要 %lld 个像素，给了 %zu 个", w, h,
                     static_cast<long long>(w) * static_cast<long long>(h), rgba.size());
        return kInvalidTexture;
    }
    // SDL_CreateSurfaceFrom 的像素参数不带 const，但这张表面只当拷贝源用、不会被写。
    SDL_Surface* surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA8888,
                                                 const_cast<std::uint32_t*>(rgba.data()), w * 4);
    if (surface == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "createTexture 建表面失败：%s",
                    sdlError().c_str());
        return kInvalidTexture;
    }
    // 经表面建纹理，让 SDL 挑后端原生支持的格式，而不是逼它每次上传都转一遍。
    SDL_Texture* texture = SDL_CreateTextureFromSurface(impl_->renderer, surface);
    SDL_DestroySurface(surface);
    if (texture == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "createTexture 失败：%s", sdlError().c_str());
        return kInvalidTexture;
    }
    SDL_SetTextureScaleMode(texture, toSdlScale(mode));
    return impl_->registerTexture(texture, false, {});
}

Point Engine::textureSize(TextureId id) const {
    const Impl::TextureRecord* rec = impl_->record(id);
    return rec != nullptr ? Point{rec->w, rec->h} : Point{};
}

void Engine::setTextureScaleMode(TextureId id, ScaleMode mode) {
    if (const Impl::TextureRecord* rec = impl_->record(id)) {
        SDL_SetTextureScaleMode(rec->texture, toSdlScale(mode));
    }
}

void Engine::destroyTexture(TextureId id) {
    const auto it = id == kInvalidTexture ? impl_->textures.end() : impl_->textures.find(id);
    if (it == impl_->textures.end()) {
        return;  // 无效、伪造、已销毁的句柄：空操作
    }
    if (impl_->currentTarget == id) {
        SDL_SetRenderTarget(impl_->renderer, nullptr);
        impl_->currentTarget = kInvalidTexture;
    }
    if (!it->second.pathKey.empty()) {
        impl_->texturesByPath.erase(it->second.pathKey);
    }
    SDL_DestroyTexture(it->second.texture);
    impl_->textures.erase(it);
}

std::vector<std::string> Engine::findAssets(const std::string& dir,
                                            const std::string& pattern) const {
    std::vector<std::string> found;
    if (impl_->assetRoot.empty()) {
        return found;
    }
    const std::string base = impl_->assetRoot + "/" + dir;
    int count = 0;
    char** hits = SDL_GlobDirectory(base.c_str(), pattern.c_str(), SDL_GLOB_CASEINSENSITIVE, &count);
    if (hits == nullptr) {
        return found;  // 目录不存在：美术还没到，不是错误
    }
    found.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        found.push_back(base + "/" + hits[i]);
    }
    SDL_free(hits);
    // 排序只为让「有几张变体、各是第几张」与文件系统的返回顺序无关。
    std::sort(found.begin(), found.end());
    return found;
}

TextureId Engine::createRenderTarget(int w, int h, ScaleMode mode) {
    if (impl_->renderer == nullptr) {
        return kInvalidTexture;
    }
    if (w <= 0 || h <= 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "createRenderTarget：尺寸 %dx%d 不合法", w, h);
        return kInvalidTexture;
    }
    SDL_Texture* texture =
        SDL_CreateTexture(impl_->renderer, kTargetFormat, SDL_TEXTUREACCESS_TARGET, w, h);
    if (texture == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "建渲染目标 %dx%d 失败：%s", w, h,
                    sdlError().c_str());
        return kInvalidTexture;
    }
    SDL_SetTextureScaleMode(texture, toSdlScale(mode));
    // 新目标的内容按 SDL 的规定是未定义的（显存里上一个人留下的东西）。清成全透明，
    // 忘了先 clear 就直接贴上去的调用方看到的是「什么都没有」，而不是一屏花屏。
    SDL_SetRenderTarget(impl_->renderer, texture);
    SDL_SetRenderDrawColor(impl_->renderer, 0, 0, 0, 0);
    SDL_RenderClear(impl_->renderer);
    SDL_SetRenderTarget(impl_->renderer, impl_->targetTexture(impl_->currentTarget));
    return impl_->registerTexture(texture, true, {});
}

void Engine::setRenderTarget(TextureId target) {
    if (impl_->renderer == nullptr) {
        return;
    }
    TextureId resolved = kInvalidTexture;
    SDL_Texture* texture = nullptr;
    if (target != kInvalidTexture) {
        const Impl::TextureRecord* rec = impl_->record(target);
        if (rec != nullptr && rec->target) {
            resolved = target;
            texture = rec->texture;
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "setRenderTarget：句柄 %u 不是渲染目标，改画到后缓冲", target);
        }
    }
    if (!SDL_SetRenderTarget(impl_->renderer, texture)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "切换渲染目标失败：%s", sdlError().c_str());
        return;
    }
    impl_->currentTarget = resolved;
}

TextureId Engine::renderTarget() const {
    return impl_->currentTarget;
}

void Engine::clear(const Color& color) {
    if (impl_->renderer == nullptr) {
        return;
    }
    SDL_SetRenderDrawColor(impl_->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(impl_->renderer);
}

TextureId Engine::snapshotBackbuffer() {
    if (impl_->renderer == nullptr) {
        return kInvalidTexture;
    }
    SDL_Surface* shot = nullptr;
    {
        const Impl::BackbufferScope onBackbuffer(*impl_);
        shot = impl_->readBackbufferContent();
    }
    if (shot == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "读后缓冲失败：%s", sdlError().c_str());
        return kInvalidTexture;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(impl_->renderer, shot);
    SDL_DestroySurface(shot);
    if (texture == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "后缓冲转纹理失败：%s", sdlError().c_str());
        return kInvalidTexture;
    }
    // 快照拿去做碎屏（旋转、缩放）和模糊，线性取样才不会出锯齿。
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    return impl_->registerTexture(texture, false, {});
}

void Engine::drawTexture(TextureId id, const Rect& src, const Rect& dst) {
    if (impl_->renderer == nullptr || id == kInvalidTexture) {
        return;
    }
    const auto it = impl_->textures.find(id);
    if (it == impl_->textures.end() || it->second.texture == nullptr) {
        return;  // 上层拿着过期或伪造的句柄画图不该导致崩溃
    }
    // 旧接口 = 原色 + 纹理自己的缺省混合。每次重设，新接口改过的状态不许漏过来。
    applyTextureState(it->second.texture, Color{255, 255, 255, 255}, it->second.naturalBlend);
    const SDL_FRect dstRect{static_cast<float>(dst.x), static_cast<float>(dst.y),
                            static_cast<float>(dst.w), static_cast<float>(dst.h)};
    // src 宽高非正表示「整张图」，省得调用方为满图绘制去查纹理尺寸。
    if (src.w <= 0 || src.h <= 0) {
        SDL_RenderTexture(impl_->renderer, it->second.texture, nullptr, &dstRect);
        return;
    }
    const SDL_FRect srcRect{static_cast<float>(src.x), static_cast<float>(src.y),
                            static_cast<float>(src.w), static_cast<float>(src.h)};
    SDL_RenderTexture(impl_->renderer, it->second.texture, &srcRect, &dstRect);
}

void Engine::drawTexture(TextureId id, const RectF& src, const RectF& dst,
                         const DrawOptions& options) {
    if (impl_->renderer == nullptr) {
        return;
    }
    const Impl::TextureRecord* rec = impl_->record(id);
    if (rec == nullptr) {
        return;  // 同旧接口：无效或过期的句柄画不出东西，但不崩
    }
    applyTextureState(rec->texture, options.tint, toSdlBlend(options.blend));
    const SDL_FRect dstRect = toFRect(dst);
    const SDL_FRect srcRect = toFRect(src);
    const SDL_FRect* srcPtr = (src.w <= 0.f || src.h <= 0.f) ? nullptr : &srcRect;
    const int flip = (options.flipX ? SDL_FLIP_HORIZONTAL : 0) |
                     (options.flipY ? SDL_FLIP_VERTICAL : 0);
    if (options.angle == 0.f && flip == 0) {
        SDL_RenderTexture(impl_->renderer, rec->texture, srcPtr, &dstRect);
        return;
    }
    SDL_RenderTextureRotated(impl_->renderer, rec->texture, srcPtr, &dstRect, options.angle,
                             nullptr, static_cast<SDL_FlipMode>(flip));
}

void Engine::drawRect(const Rect& rect, const Color& color, bool filled) {
    if (impl_->renderer == nullptr) {
        return;
    }
    const SDL_FRect r{static_cast<float>(rect.x), static_cast<float>(rect.y),
                      static_cast<float>(rect.w), static_cast<float>(rect.h)};
    SDL_SetRenderDrawBlendMode(impl_->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(impl_->renderer, color.r, color.g, color.b, color.a);
    if (filled) {
        SDL_RenderFillRect(impl_->renderer, &r);
    } else {
        SDL_RenderRect(impl_->renderer, &r);
    }
}

void Engine::fillRect(const RectF& rect, const Color& color, BlendMode blend) {
    if (impl_->renderer == nullptr) {
        return;
    }
    const SDL_FRect r = toFRect(rect);
    SDL_SetRenderDrawBlendMode(impl_->renderer, toSdlBlend(blend));
    SDL_SetRenderDrawColor(impl_->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(impl_->renderer, &r);
}

void Engine::drawGradient(const RectF& rect, const Color& top, const Color& bottom,
                          BlendMode blend) {
    if (impl_->renderer == nullptr) {
        return;
    }
    impl_->drawQuad(rect, top, top, bottom, bottom, blend);
}

void Engine::drawGradientH(const RectF& rect, const Color& left, const Color& right,
                           BlendMode blend) {
    if (impl_->renderer == nullptr) {
        return;
    }
    impl_->drawQuad(rect, left, right, right, left, blend);
}

void Engine::drawGeometry(TextureId texture, const std::vector<Vertex>& vertices,
                          const std::vector<int>& indices, BlendMode blend) {
    if (impl_->renderer == nullptr || vertices.empty()) {
        return;
    }
    SDL_Texture* sdlTexture = nullptr;
    if (texture != kInvalidTexture) {
        const Impl::TextureRecord* rec = impl_->record(texture);
        if (rec == nullptr) {
            return;  // 纹理句柄过期：不改画成无纹理的色块，那比什么都不画更难查
        }
        sdlTexture = rec->texture;
        SDL_SetTextureBlendMode(sdlTexture, toSdlBlend(blend));
    } else {
        SDL_SetRenderDrawBlendMode(impl_->renderer, toSdlBlend(blend));
    }

    std::vector<SDL_Vertex>& converted = impl_->vertexScratch;
    converted.clear();
    converted.reserve(vertices.size());
    for (const Vertex& v : vertices) {
        converted.push_back(SDL_Vertex{{v.x, v.y}, toFColor(v.color), {v.u, v.v}});
    }
    const bool ok = SDL_RenderGeometry(impl_->renderer, sdlTexture, converted.data(),
                                       static_cast<int>(converted.size()),
                                       indices.empty() ? nullptr : indices.data(),
                                       static_cast<int>(indices.size()));
    if (!ok) {
        // 下标越界、不是三的倍数：SDL 拒画而不崩，但这是调用方的错，要让它看得见。
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "drawGeometry 失败：%s", sdlError().c_str());
    }
}

void Engine::drawLine(float x1, float y1, float x2, float y2, const Color& color) {
    if (impl_->renderer == nullptr) {
        return;
    }
    SDL_SetRenderDrawBlendMode(impl_->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(impl_->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderLine(impl_->renderer, x1, y1, x2, y2);
}

void Engine::setClipRect(const Rect* rect) {
    if (impl_->renderer == nullptr) {
        return;
    }
    if (rect == nullptr) {
        SDL_SetRenderClipRect(impl_->renderer, nullptr);
        return;
    }
    const SDL_Rect r{rect->x, rect->y, rect->w, rect->h};
    SDL_SetRenderClipRect(impl_->renderer, &r);
}

void Engine::drawText(const std::string& utf8, int x, int y, int size, const Color& color) {
    if (impl_->renderer == nullptr || utf8.empty()) {
        return;
    }
    if (const Impl::CachedText* text = impl_->textFor(utf8, size, color, 0)) {
        impl_->blitText(*text, x, y);
    }
}

void Engine::drawText(const std::string& utf8, int x, int y, int size, const Color& color,
                      const TextStyle& style) {
    if (impl_->renderer == nullptr || utf8.empty()) {
        return;
    }
    // 描边字形比正文每边胖 outline 像素（SDL_ttf 的约定），往左上挪同样多才与正文对齐。
    const int outline = style.outline ? style.outlineWidth : 0;
    if (style.shadow) {
        // 开着描边时投影也用描边的字形：影子跟着「整个带边的字」走，
        // 否则描边外面会再漏出一圈细细的正文影子。
        if (const Impl::CachedText* shadow = impl_->textFor(utf8, size, style.shadowColor, outline)) {
            impl_->blitText(*shadow, x - outline + style.shadowOffset,
                            y - outline + style.shadowOffset);
        }
    }
    if (outline > 0) {
        if (const Impl::CachedText* edge = impl_->textFor(utf8, size, style.outlineColor, outline)) {
            impl_->blitText(*edge, x - outline, y - outline);
        }
    }
    if (const Impl::CachedText* text = impl_->textFor(utf8, size, color, 0)) {
        impl_->blitText(*text, x, y);
    }
}

Point Engine::measureText(const std::string& utf8, int size) const {
    TTF_Font* font = impl_->fontFor(size);
    if (font == nullptr) {
        return Point{};
    }
    const int lineHeight = TTF_GetFontHeight(font);
    if (utf8.empty()) {
        // 空串宽度为 0，但高度仍是一行：排版层据此摆空行的位置。
        return Point{0, lineHeight};
    }
    int w = 0;
    int h = 0;
    if (!TTF_GetStringSize(font, utf8.c_str(), utf8.size(), &w, &h)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "测量文字失败：%s", sdlError().c_str());
        return Point{0, lineHeight};
    }
    return Point{w, h > 0 ? h : lineHeight};
}

bool Engine::keyDown(Key key) const {
    if (key == Key::Count) {
        return false;
    }
    return impl_->keyHeld(key);
}

bool Engine::keyPressed(Key key) const {
    if (key == Key::Count) {
        return false;
    }
    return impl_->pressed[static_cast<std::size_t>(key)];
}

// ---------------------------------------------------------------------------
// 键位（docs/settings.md 4.4、第 6 节）
// ---------------------------------------------------------------------------

const char* Engine::keyId(Key key) {
    const auto k = static_cast<std::size_t>(key);
    return k < kKeyCount ? kKeyIds[k] : "";
}

std::optional<Engine::Key> Engine::keyFromId(std::string_view id) {
    for (std::size_t k = 0; k < kKeyCount; ++k) {
        if (id == kKeyIds[k]) return static_cast<Key>(k);
    }
    return std::nullopt;
}

std::vector<ScanCode> Engine::fixedKeys(Key key) {
    std::vector<ScanCode> keys;
    for (const FixedKey& f : kFixedKeys) {
        if (f.key == key) keys.push_back(static_cast<ScanCode>(f.scancode));
    }
    return keys;
}

Engine::KeySlots Engine::defaultCustomKeys(Key key) {
    const auto k = static_cast<std::size_t>(key);
    return k < kKeyCount ? kDefaultKeys[k] : KeySlots{};
}

Engine::KeyTable Engine::defaultKeyTable() {
    return kDefaultKeys;
}

Engine::KeySlots Engine::customKeys(Key key) const {
    const auto k = static_cast<std::size_t>(key);
    return k < kKeyCount ? impl_->customKeys[k] : KeySlots{};
}

std::optional<Engine::Key> Engine::fixedKeyOwner(ScanCode code) {
    for (const FixedKey& f : kFixedKeys) {
        if (static_cast<ScanCode>(f.scancode) == code) return f.key;
    }
    return std::nullopt;
}

bool Engine::reservedKey(ScanCode code) {
    return std::any_of(kReservedKeys.begin(), kReservedKeys.end(),
                       [code](SDL_Scancode r) { return static_cast<ScanCode>(r) == code; });
}

core::Result<bool> Engine::validateKeyTable(const KeyTable& table) {
    using R = core::Result<bool>;
    std::array<int, static_cast<std::size_t>(kScanCodeLimit)> owner{};   // 扫描码 → 动作下标 + 1（0 = 还没人用）
    for (std::size_t k = 0; k < kKeyCount; ++k) {
        const std::string who = kKeyIds[k];
        for (std::size_t s = 0; s < 2; ++s) {
            const ScanCode code = table[k][s];
            if (code == 0) continue;
            const std::string where = who + " 的第 " + std::to_string(s + 1) + " 格";
            if (!validCode(code)) {
                return R::failure(where + "是 " + std::to_string(code) + "，不是 1–" +
                                  std::to_string(kScanCodeLimit - 1) + " 的扫描码");
            }
            if (const std::optional<Key> fixedOwner = fixedKeyOwner(code)) {
                return R::failure(where + "配了 " + keyLabel(code) + "，那是 " + keyId(*fixedOwner) + " 的固定键");
            }
            if (reservedKey(code)) {
                return R::failure(where + "配了 " + keyLabel(code) + "，那个键留作他用");
            }
            int& used = owner[static_cast<std::size_t>(code)];
            if (used != 0) {
                return R::failure(where + "配了 " + keyLabel(code) + "，它已经配给了 " +
                                  kKeyIds[static_cast<std::size_t>(used - 1)]);
            }
            used = static_cast<int>(k) + 1;
        }
        if (fixedKeys(static_cast<Key>(k)).empty() && table[k][0] == 0 && table[k][1] == 0) {
            return R::failure(who + " 一个键都没有");
        }
    }
    return R::success(true);
}

core::Result<bool> Engine::setCustomKeys(const KeyTable& table) {
    if (table == impl_->customKeys) return core::Result<bool>::success(true);
    auto valid = validateKeyTable(table);
    if (!valid) return valid;
    impl_->customKeys = table;
    return valid;
}

Engine::KeyChangeResult Engine::assignKey(const KeyTable& table, Key action, int slot, ScanCode code) {
    KeyChangeResult result{table, KeyChange::Unchanged, Key::Count};
    const auto a = static_cast<std::size_t>(action);
    if (a >= kKeyCount || slot < 0 || slot > 1 || !validCode(code)) {
        result.change = KeyChange::RejectedInvalid;
        return result;
    }
    // 1. 固定键、保留键：谁也不许动。
    if (const std::optional<Key> fixedOwner = fixedKeyOwner(code)) {
        result.change = KeyChange::RejectedFixed;
        result.other = *fixedOwner;
        return result;
    }
    if (reservedKey(code)) {
        result.change = KeyChange::RejectedReserved;
        return result;
    }
    const auto s = static_cast<std::size_t>(slot);
    const ScanCode previous = table[a][s];
    // 2. 就在这一格：无事。
    if (previous == code) return result;
    // 3. 在自己的另一格：两格互换（这一格原来空着 = 从另一格挪过来，另一格空出来）。
    if (table[a][1 - s] == code) {
        result.table[a][1 - s] = previous;
        result.table[a][s] = code;
        result.change = previous != 0 ? KeyChange::SwappedSlots : KeyChange::MovedSlot;
        return result;
    }
    // 4. 在别的动作 B 的某一格：那一格换成这一格原来的键（互换；原来空着 = B 那一格空出来）。
    //    B 因此一个键都不剩 → 拒绝。
    for (std::size_t b = 0; b < kKeyCount; ++b) {
        if (b == a) continue;
        for (std::size_t t = 0; t < 2; ++t) {
            if (table[b][t] != code) continue;
            if (previous == 0 && !hasAnotherKey(table, static_cast<Key>(b), static_cast<int>(t))) {
                result.change = KeyChange::RejectedLastKey;
                result.other = static_cast<Key>(b);
                return result;
            }
            result.table[b][t] = previous;
            result.table[a][s] = code;
            result.change = previous != 0 ? KeyChange::TradedWith : KeyChange::TookFrom;
            result.other = static_cast<Key>(b);
            return result;
        }
    }
    // 5. 谁也没用这个键：直接放进来。
    result.table[a][s] = code;
    result.change = KeyChange::Assigned;
    return result;
}

Engine::KeyChangeResult Engine::clearKey(const KeyTable& table, Key action, int slot) {
    KeyChangeResult result{table, KeyChange::Unchanged, Key::Count};
    const auto a = static_cast<std::size_t>(action);
    if (a >= kKeyCount || slot < 0 || slot > 1) {
        result.change = KeyChange::RejectedInvalid;
        return result;
    }
    const auto s = static_cast<std::size_t>(slot);
    if (table[a][s] == 0) {
        result.change = KeyChange::AlreadyEmpty;
        return result;
    }
    if (!hasAnotherKey(table, action, slot)) {
        result.change = KeyChange::RejectedLastKey;
        result.other = action;
        return result;
    }
    result.table[a][s] = 0;
    result.change = KeyChange::Cleared;
    return result;
}

void Engine::beginKeyCapture() {
    impl_->capturing = true;
    impl_->captured.reset();
    // 从这一刻起就不出逻辑键：本帧已经记下的「刚按下」作废，按着的键不再自动重复。
    impl_->pressed.fill(false);
    impl_->repeatAtMs.fill(0);
}

void Engine::cancelKeyCapture() {
    impl_->capturing = false;
    impl_->captured.reset();
}

bool Engine::capturingKey() const {
    return impl_->capturing;
}

std::optional<ScanCode> Engine::takeCapturedKey() {
    std::optional<ScanCode> code = impl_->captured;
    impl_->captured.reset();
    return code;
}

std::string Engine::keyLabel(ScanCode code) {
    if (code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z) {
        return std::string(1, static_cast<char>('A' + (code - SDL_SCANCODE_A)));
    }
    if (code >= SDL_SCANCODE_1 && code <= SDL_SCANCODE_9) {
        return std::string(1, static_cast<char>('1' + (code - SDL_SCANCODE_1)));
    }
    if (code >= SDL_SCANCODE_F1 && code <= SDL_SCANCODE_F12) {
        return "F" + std::to_string(code - SDL_SCANCODE_F1 + 1);
    }
    if (code >= SDL_SCANCODE_KP_1 && code <= SDL_SCANCODE_KP_9) {
        return "小键盘" + std::to_string(code - SDL_SCANCODE_KP_1 + 1);
    }
    switch (code) {
        case SDL_SCANCODE_0: return "0";
        case SDL_SCANCODE_KP_0: return "小键盘0";
        case SDL_SCANCODE_SPACE: return "空格";
        case SDL_SCANCODE_RETURN: return "Enter";
        case SDL_SCANCODE_KP_ENTER: return "小键盘Enter";
        case SDL_SCANCODE_ESCAPE: return "Esc";
        case SDL_SCANCODE_TAB: return "Tab";
        case SDL_SCANCODE_UP: return "↑";
        case SDL_SCANCODE_DOWN: return "↓";
        case SDL_SCANCODE_LEFT: return "←";
        case SDL_SCANCODE_RIGHT: return "→";
        case SDL_SCANCODE_LCTRL: return "Ctrl";
        case SDL_SCANCODE_RCTRL: return "右Ctrl";
        case SDL_SCANCODE_LSHIFT: return "Shift";
        case SDL_SCANCODE_RSHIFT: return "右Shift";
        default: break;
    }
    const char* name = validCode(code) ? SDL_GetScancodeName(static_cast<SDL_Scancode>(code)) : nullptr;
    if (name != nullptr && name[0] != '\0') return name;
    return "键#" + std::to_string(code);
}

// ---------------------------------------------------------------------------
// 手柄（docs/gamepad.md）
// ---------------------------------------------------------------------------

std::vector<Engine::PadButton> Engine::padButtons(Key key) {
    std::vector<PadButton> buttons;
    for (const PadKey& p : kPadKeys) {
        if (p.key == key) buttons.push_back(p.button);
    }
    return buttons;
}

std::string Engine::padLabel(PadButton button) {
    switch (button) {
        case PadButton::A: return "A";
        case PadButton::B: return "B";
        case PadButton::X: return "X";
        case PadButton::Y: return "Y";
        case PadButton::Back: return "⧉";
        case PadButton::Start: return "≡";
        case PadButton::LB: return "LB";
        case PadButton::RB: return "RB";
        case PadButton::LT: return "LT";
        case PadButton::RT: return "RT";
        case PadButton::DpadUp: return "十字↑";
        case PadButton::DpadDown: return "十字↓";
        case PadButton::DpadLeft: return "十字←";
        case PadButton::DpadRight: return "十字→";
        case PadButton::StickUp: return "摇杆↑";
        case PadButton::StickDown: return "摇杆↓";
        case PadButton::StickLeft: return "摇杆←";
        case PadButton::StickRight: return "摇杆→";
        case PadButton::Count: break;
    }
    return {};
}

std::optional<Engine::Key> Engine::stickDirection(float x, float y, std::optional<Key> held) {
    if (held && !isDirection(*held)) held.reset();
    const float length = std::sqrt(x * x + y * y);
    const float r = std::min(length, 1.f);   // r 大于 1 当 1
    if (held) {
        // 原方向继续按住：偏离那条轴不超过 55°（偏角的余弦 = 与那条轴的单位向量的点积 / 模长），且 r ≥ 0.35（迟滞）。
        const float along = *held == Key::Right ? x : *held == Key::Left ? -x : *held == Key::Down ? y : -y;
        if (along >= kStickHoldCos * length && r >= kStickHoldRadius) return held;
    }
    // 从零推起，或偏出 55° 换向（换向算一次新的推，整改轮）：一律要 r ≥ 0.5；0.35 的迟滞只给原方向。NaN 也落在「不到位」。
    if (!(r >= kStickPressRadius)) return std::nullopt;
    // 最近的轴；恰在对角线取左右（与世界层「左右优先」同口径）。y 向下为正。
    if (std::fabs(x) >= std::fabs(y)) return x > 0.f ? Key::Right : Key::Left;
    return y > 0.f ? Key::Down : Key::Up;
}

bool Engine::triggerDown(float value, bool wasDown) {
    return value >= (wasDown ? kTriggerHoldValue : kTriggerPressValue);
}

Engine::InputDevice Engine::lastInputDevice() const {
    return impl_->lastDevice;
}

int Engine::gamepadCount() const {
    return impl_->openPadCount();
}

void Engine::setRumbleEnabled(bool on) {
    impl_->rumbleOn = on;
}

bool Engine::rumbleEnabled() const {
    return impl_->rumbleOn;
}

void Engine::rumble(float low, float high, int durationMs) {
    if (!impl_->rumbleOn || impl_->lastDevice != InputDevice::Gamepad) return;
    const auto it = impl_->pads.find(impl_->lastPad);
    if (it == impl_->pads.end() || it->second.handle == nullptr) return;
    // 返回值有意不看：不带马达的手柄是正常情况，每一击都打一行「不支持震动」只是刷屏。
    static_cast<void>(SDL_RumbleGamepad(it->second.handle, rumbleStrength(low), rumbleStrength(high),
                                        static_cast<Uint32>(std::clamp(durationMs, 0, kMaxRumbleMs))));
}

void Engine::playBgm(const std::string& id) {
    if (!impl_->audioReady) {
        return;
    }
    if (id == impl_->currentBgm && MIX_TrackPlaying(impl_->bgmTrack)) {
        return;   // 两张图共用一首曲子，换图不该让它重头来
    }
    MIX_Audio* audio = impl_->audioFor("bgm", id);
    if (audio == nullptr) {
        return;
    }
    if (!MIX_SetTrackAudio(impl_->bgmTrack, audio)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "绑定 BGM 轨道失败：%s", sdlError().c_str());
        return;
    }
    // SDL3_mixer 用属性表传播放参数，没有 SDL2 那个 loops 形参。-1 是无限循环。
    // 250 毫秒淡入：换曲时不至于在一个强拍上硬切进来。
    const SDL_PropertiesID options = SDL_CreateProperties();
    if (options != 0) {
        SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
        SDL_SetNumberProperty(options, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, 250);
    }
    if (!MIX_PlayTrack(impl_->bgmTrack, options)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "播放 BGM %s 失败：%s", id.c_str(),
                    sdlError().c_str());
    } else {
        impl_->currentBgm = id;
    }
    if (options != 0) {
        SDL_DestroyProperties(options);
    }
}

void Engine::stopBgm() {
    if (!impl_->audioReady) {
        return;
    }
    MIX_StopTrack(impl_->bgmTrack, 0);
    impl_->currentBgm.clear();
}

void Engine::playSfx(const std::string& id) {
    if (!impl_->audioReady) {
        return;
    }
    MIX_Audio* audio = impl_->audioFor("sfx", id);
    if (audio == nullptr) {
        return;
    }
    // 从前走 MIX_PlayAudio（借一条临时轨道放完就收），可它不接受任何混音参数、调不了音量；
    // 现在从 init 建好的轨道池里取一条，增益就是音效音量（docs/settings.md 4.1）。
    // 默认增益 1、不循环、不淡入：与 MIX_PlayAudio 放出来的是同一个声音。
    MIX_Track* track = impl_->takeSfxTrack();
    if (track == nullptr) {
        return;   // 一条轨道都没建成：init 时已经说过了，这里不每次刷日志
    }
    if (!MIX_SetTrackAudio(track, audio) || !MIX_PlayTrack(track, 0)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "播放音效 %s 失败：%s", id.c_str(),
                    sdlError().c_str());
    }
}

// ---------------------------------------------------------------------------
// 系统设置（docs/settings.md 第 4 节）
// ---------------------------------------------------------------------------

void Engine::setBgmVolume(float gain) {
    impl_->bgmGain = sanitizeGain(gain);
    if (impl_->bgmTrack != nullptr) {
        MIX_SetTrackGain(impl_->bgmTrack, impl_->bgmGain);
    }
}

void Engine::setSfxVolume(float gain) {
    impl_->sfxGain = sanitizeGain(gain);
    for (MIX_Track* track : impl_->sfxTracks) {
        if (track != nullptr) {
            MIX_SetTrackGain(track, impl_->sfxGain);
        }
    }
}

float Engine::bgmVolume() const {
    return impl_->bgmGain;
}

float Engine::sfxVolume() const {
    return impl_->sfxGain;
}

void Engine::setFullscreen(bool on) {
    impl_->fullscreen = on;
    if (impl_->window == nullptr || fullscreen() == on) {
        return;
    }
    // 缺省就是无边框桌面全屏（没设过 SDL_SetWindowFullscreenMode）：不换显示模式，切得快，也不黑屏。
    if (!SDL_SetWindowFullscreen(impl_->window, on)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "切换全屏失败：%s", sdlError().c_str());
        return;
    }
    // 有的窗口系统上这只是个请求、异步生效。等它落定：fullscreen() 读的是真实状态，
    // Application 每帧拿它对账——没落定就读，会把刚存进去的设置当成「被 Alt+Enter 切回去了」。
    // 在立即生效的窗口系统上（Windows）这是空操作。
    SDL_SyncWindow(impl_->window);
}

bool Engine::fullscreen() const {
    if (impl_->window == nullptr) {
        return impl_->fullscreen;
    }
    return (SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_FULLSCREEN) != 0;
}

void Engine::setIntegerScale(bool on) {
    if (impl_->integerScale == on) {
        return;
    }
    impl_->integerScale = on;
    if (impl_->renderer == nullptr) {
        return;
    }
    // 逻辑呈现是**每个渲染目标各一份**（SDL3 的规定），要设的是后缓冲那一份。
    const Impl::BackbufferScope onBackbuffer(*impl_);
    if (!SDL_SetRenderLogicalPresentation(impl_->renderer, kLogicalWidth, kLogicalHeight,
                                          presentationFor(on))) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "切换画面缩放失败：%s", sdlError().c_str());
    }
}

bool Engine::integerScale() const {
    return impl_->integerScale;
}

void Engine::setVSync(bool on) {
    if (impl_->vsync == on) {
        return;
    }
    impl_->vsync = on;
    if (impl_->renderer != nullptr && !SDL_SetRenderVSync(impl_->renderer, on ? 1 : 0)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s垂直同步失败：%s", on ? "打开" : "关闭",
                    sdlError().c_str());
    }
}

bool Engine::vsync() const {
    return impl_->vsync;
}

void Engine::setEffectsLevel(EffectsLevel level) {
    impl_->effects = level;
}

EffectsLevel Engine::effectsLevel() const {
    return impl_->effects;
}

}  // namespace fanren::engine
