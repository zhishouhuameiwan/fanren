#pragma once
// 世界画面（`--map` 下的行走画面）的 HD-2D 渲染：施工图 docs/octopath-overhaul.md 第 1.1–1.3 节与第 3 节。
//
// 分工：WorldScene 判定「画什么」——谁在场、哪道门开着、哪道门通往目标、哪一格此刻按得动、
// 目标在哪——把答案装进一个 WorldFrame 交过来；这里只管「怎么画」：烘焙图还是旧图元、人物走到
// 哪一帧、镜头在哪、光与粒子与后处理。判据一个都不在这里另写：画面与规则各算一套，迟早
// 「看得见的」和「走得通的」对不上（WorldScene.h 里 visibleNpcAt 那段注释讲的就是这个）。
//
// **纯视觉**：格子逻辑一点不动。tryStep 一调，GameState::position 立刻变——无头测试靠这一点
// 驱动整章剧情。这里的插值滑动、走路帧、镜头缓动、换图淡入都只是「画在哪儿」，从不回头去
// 延迟状态；无头模式下渲染全是空操作，advance 只推几个浮点数。
//
// 一帧的渲染次序（照 FxDemoScene 的调用次序写）：
//   后处理 beginScene
//     ① 下层：烘焙 below.png（缺图 → MapArt 旧图元：ground / overlay / building，按 48px 画）
//     ② 地面标记：传送箭头与门光、可交互处的闪光、目标光圈、水面波光（加色）
//     ③ 人物脚下柔影
//     ④ 按脚底 y 排序：设施、NPC、主角（精灵；缺图 → MapArt::drawFigure / 旧设施底板）
//     ⑤ 上层：烘焙 above.png，主角周围约 2 格半径渐隐（缺图 → front 层旧图元，同样渐隐）
//     ⑥ 粒子：满屏落下的（花瓣、落叶）与世界里的（尘埃、萤火、薄雾、定点火星）
//     ⑦ 辉光通道：灯的光晕、设施的发光帧、箭头与闪光、目标光圈、萤火
//     ⑧ 点光源：meta.json 的灯 + 主角随行微光（夜 / 黄昏 / 室内）
//   后处理 endScene（光照 → 辉光 → 景深 → 暗角 → 调色）
//   UI：名牌、头顶气泡、目标菱形或边缘箭头、地名横幅、目标框、存盘提示、换图淡入的黑幕
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "engine/Engine.h"
#include "engine/Particles.h"
#include "engine/PostFx.h"
#include "game/MapVisual.h"
#include "game/SpriteAtlas.h"
#include "game/WorldHud.h"

namespace fanren::game {

class Application;

// 像素尺度（施工图 1.1）：地图与人物是 16px 的像素美术，放大三倍，一格在屏幕上 48 像素。
// 逻辑格仍是 engine::kTileSize = 32：地图、碰撞、对象、测试全都不动，48 只活在渲染层。
inline constexpr int kArtTilePx = 16;
inline constexpr int kArtScale = 3;
inline constexpr int kWorldTilePx = kArtTilePx * kArtScale;

// 两格之间滑一步的时长。与 WorldScene 的单步冷却（0.12 秒）同一个数：按住方向键时
// 上一步刚滑到、下一步正好起步，走起来是连着的。
inline constexpr float kStepSlideSeconds = 0.12f;

// 换图（传送点、脚本 teleport）后从黑色淡入的时长。逻辑上换图仍是瞬时的。
inline constexpr float kMapFadeSeconds = 0.3f;

struct Vec2f {
    float x = 0.f;
    float y = 0.f;
};

// ---------------------------------------------------------------------------
// 纯函数：不开窗口就能单测（tests/WorldViewTests.cpp）
// ---------------------------------------------------------------------------

// 镜头在一个轴上的起点（屏幕像素）：地图比视口小就居中（结果为负），否则让 focus 落在
// 视口正中、再夹在 [0, 地图 − 视口] 里，镜头不越过地图边缘。
[[nodiscard]] float cameraAxis(float focus, float mapExtent, float viewExtent);

// 镜头左上角（屏幕像素，48px 一格）：focusCell 是要对准的那一点（格坐标，可带小数）。
[[nodiscard]] Vec2f cameraTarget(int mapWidthCells, int mapHeightCells, Vec2f focusCell);

// 指数趋近：每秒把剩下的距离收掉 1 − e^(−rate)。不越过目标、dt 为 0 不动。
// 与帧率无关——60 帧与 30 帧跑同样的秒数，镜头停在同一处。
[[nodiscard]] float approach(float current, float target, float dt, float rate);

// 一步的插值滑动。position 是画面上人物所在的格坐标（带小数）。
class StepSlide {
public:
    // 瞬移：换图、脚本挪人、开场。
    void snap(Vec2f cell);
    // 从画面上当前的位置（不一定是上一格，可能还在滑）滑向新的一格。
    void moveTo(Vec2f cell);
    void advance(float dt);
    [[nodiscard]] Vec2f position() const;
    [[nodiscard]] bool moving() const { return elapsed_ < duration_; }
    [[nodiscard]] Vec2f target() const { return to_; }

private:
    Vec2f from_{};
    Vec2f to_{};
    float elapsed_ = 0.f;
    float duration_ = 0.f;
};

// 深度排序键：先比脚底的 y（越靠下越后画、越在前面），同一行再从左到右，最后按登记次序。
// 最后一条让排序在「完全重叠」时也是确定的——同一帧画两遍必须一模一样。
struct DepthKey {
    float footY = 0.f;
    float x = 0.f;
    int order = 0;
};
[[nodiscard]] bool drawsBefore(const DepthKey& a, const DepthKey& b);

// 树冠 / 屋檐的不透明度：离主角 distanceCells 格。约 1.25 格以内压到 0.3，
// 2.75 格以外完全不透明，中间 smoothstep。
[[nodiscard]] float canopyAlpha(float distanceCells);

// 换图淡入的黑幕不透明度：刚换图为 1，kMapFadeSeconds 后为 0。
[[nodiscard]] float mapFadeAlpha(float seconds);

// 地名横幅的不透明度。afterFade 为真（换图进来的）：等黑幕淡一半后淡入、停几秒、淡出；
// 为假（开场第一次进世界）：一上来就在、停几秒、淡出——外层（标题画面）的开场淡入已经在做淡入，
// 横幅再从零淡一遍，第一眼看到的就是一块空角落。
[[nodiscard]] float bannerAlpha(float seconds, bool afterFade);

// 传送点的箭头朝哪（0 上 / 1 右 / 2 下 / 3 左）：玩家要往哪个方向迈步才会踏进去。
// 判据：那一侧挡路（或出了地图），而对面那一侧是走得通的来路；贴着地图边的门优先。
// 一个方向都对不上（门摆在空地中间）时朝下。
[[nodiscard]] int portalArrowFacing(const core::TileMap& map, const core::MapObject& portal);
// 门洞：箭头朝上、而上面不是地图边——墙上的一道门（画门光），不是走出地图的路口。
[[nodiscard]] bool portalIsDoorway(const core::TileMap& map, const core::MapObject& portal);

// 设施「立着的那一件」归谁画（一件东西只许一个主人，不然就叠成两份）：
// 烘焙图在用时，告示板与蒲团整件都在烘焙里（A1 的 mapprops.paint_facilities 按墙面、按地面各画各的），
// 运行时不再叠精灵；其余几种烘焙里只有地面那一截（地火口、存档石、幌子），立着的物件连同它的
// 帧动画与发光帧由运行时的精灵画。烘焙图缺了（退回旧图元）时九种一律由运行时画。
[[nodiscard]] bool facilitySpriteAtRuntime(const std::string& kind, bool belowBaked);

// NPC 头顶的路径行动气泡（施工图 5.1：有可用路径行动时浮一个小图标）。
enum class PathBubble { None, Generic, Inquire, Purchase, Challenge };
// 气泡在物件图集里的 id（bubble.path / bubble.path_inquire …）；None 为 nullptr。
[[nodiscard]] const char* pathBubbleSprite(PathBubble bubble);

// ---------------------------------------------------------------------------
// 一帧要画的东西：WorldScene 按规则判定好之后交过来
// ---------------------------------------------------------------------------
struct WorldFrame {
    struct Npc {
        const core::MapObject* object = nullptr;
        std::string roleId;
        std::string name;          // 名牌上的字（data/roles 里查不到时是 role_id 本身）
        int facing = 2;            // 同 GameState::facing
        PathBubble bubble = PathBubble::None;
    };
    struct Portal {
        const core::MapObject* object = nullptr;
        bool open = true;          // require_flag 满足了没有
        bool guide = false;        // 通往目标的下一道门（WorldScene::portalIsGuide）
    };
    struct Facility {
        const core::MapObject* object = nullptr;
        std::string kind;
    };

    std::vector<Npc> npcs;                          // 只有在场的
    std::vector<Portal> portals;
    std::vector<Facility> facilities;
    std::vector<const core::MapObject*> hotspots;   // 按键才响、而且此刻当真还能响的触发区
    const core::MapObject* objectiveTarget = nullptr;   // 目标就在这张图上时是它
    std::string objectiveText;                      // 左上角目标框第一行
    std::string objectiveWhere;                     // 第二行（WorldScene::objectiveWhereText）
    std::string mapTitle;                           // 地名横幅
    int dangerStars = 0;                            // 野外图的危险星级（0 = 这张图没有遭遇；算法见 docs/interfaces-octo-encounters.md）
    std::string saveNotice;                         // 空串 = 不显示

    void clear();
};

// ---------------------------------------------------------------------------
// 渲染器本体
// ---------------------------------------------------------------------------
class WorldView {
public:
    WorldView();
    ~WorldView();
    WorldView(const WorldView&) = delete;
    WorldView& operator=(const WorldView&) = delete;

    // 场景进栈时调：下一次看到地图按「开场」处理（镜头就位、不淡入、横幅直接在）。
    void reset();

    // 纯视觉的推进：插值、走路帧、镜头缓动、粒子、横幅与黑幕的计时。
    // WorldScene::update 每帧调；对话框压在上面时 update 轮不到世界层，render 会自己补这一步。
    void advance(Application& app, double deltaSeconds);

    void render(Application& app, const WorldFrame& frame);

    // ---- 给测试与调试看的状态 ----
    [[nodiscard]] Vec2f playerCell() const { return player_.position(); }
    [[nodiscard]] bool playerMoving() const { return player_.moving(); }
    [[nodiscard]] bool walking() const { return walking_; }
    [[nodiscard]] Vec2f camera() const { return camera_; }
    [[nodiscard]] float fadeAlpha() const { return mapFadeAlpha(fadeClock_); }
    [[nodiscard]] float bannerOpacity() const { return bannerAlpha(bannerClock_, bannerAfterFade_); }

private:
    struct ParticleSlot {
        engine::ParticleSystem system;
        bool emissive = false;      // 萤火：再画一份进辉光通道
        bool world = false;         // 世界坐标（跟着地图走）还是屏幕坐标（满屏的雨雪花叶）
        engine::RectF reach{};      // 世界坐标里粒子可能出现的范围（裁剪用）
    };

    struct MapAssets {
        std::string mapId;
        bool loaded = false;
        engine::TextureId below = engine::kInvalidTexture;
        engine::TextureId above = engine::kInvalidTexture;
        engine::Point aboveSize{};
        MapVisual visual;
        std::vector<std::uint8_t> water;   // width × height，1 = 水面格
        std::vector<ParticleSlot> particles;
    };

    // 这一帧的取景：取整后的镜头与看得见的格子范围 [x0, x1) × [y0, y1)。
    static constexpr std::size_t kChapterSlots = 16;   // 第 1–14 章，留一点余量

    struct Viewport {
        float camX = 0.f;
        float camY = 0.f;
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        Vec2f player{};             // 主角画面上的格坐标
        // 第 n 章演完了没有（选外观用）：一帧问一次章节表，下标即章号。
        std::array<bool, kChapterSlots> chapterDone{};
    };

    struct Drawable {
        enum class Kind { Facility, Npc, Player };
        DepthKey key;
        Kind kind = Kind::Npc;
        std::size_t index = 0;      // frame.facilities / frame.npcs 里的下标
    };

    void sync(Application& app);
    void updateWalk(float dt);
    [[nodiscard]] Vec2f focusCell() const;

    void ensureAtlas(Application& app);
    void ensureMap(Application& app, const core::TileMap& map);
    void releaseMap(engine::Engine& engine);
    engine::TextureId loadBaked(engine::Engine& engine, const core::TileMap& map, const char* file);
    void setupParticles(const core::TileMap& map);
    engine::TextureId sheetTexture(engine::Engine& engine, const CharacterSheet& sheet);
    [[nodiscard]] const ObjectSprite* objectSprite(const std::string& id) const;
    void warnOnce(const std::string& key, const std::string& message);

    [[nodiscard]] Viewport viewport(Application& app, const core::TileMap& map) const;

    // 世界层（进后处理）
    void drawBaked(engine::Engine& engine, engine::TextureId texture, const Viewport& vp, int cx0,
                   int cy0, int cx1, int cy1);
    void drawTileLayer(engine::Engine& engine, const core::TileMap& map, const std::vector<int>& layer,
                       const Viewport& vp, bool fadeNearPlayer);
    void drawBelow(engine::Engine& engine, const core::TileMap& map, const Viewport& vp);
    void drawAbove(engine::Engine& engine, const core::TileMap& map, const Viewport& vp);
    void drawWaterGlints(engine::Engine& engine, const core::TileMap& map, const Viewport& vp);
    void drawObjectSprite(engine::Engine& engine, engine::TextureId texture, const ObjectSprite& sprite,
                          float footX, float footY, float seconds, const engine::Color& tint,
                          engine::BlendMode blend);
    void drawPortals(engine::Engine& engine, const core::TileMap& map, const WorldFrame& frame,
                     const Viewport& vp, bool emissivePass);
    void drawHotspots(engine::Engine& engine, const WorldFrame& frame, const Viewport& vp,
                      bool emissivePass);
    void drawObjectiveRing(engine::Engine& engine, const WorldFrame& frame, const Viewport& vp);
    void drawShadows(engine::Engine& engine, const WorldFrame& frame, const Viewport& vp);
    void drawSorted(engine::Engine& engine, Application& app, const WorldFrame& frame, const Viewport& vp);
    void drawCharacter(engine::Engine& engine, const std::string& roleId, bool player, float footX,
                       float footY, int facing, bool moving, const Viewport& vp);
    void drawFacility(engine::Engine& engine, const WorldFrame::Facility& facility, const Viewport& vp,
                      bool emissivePass);
    void drawParticles(engine::Engine& engine, const Viewport& vp, bool emissivePass);
    void drawLightGlows(engine::Engine& engine, const Viewport& vp);
    void addLights(const Viewport& vp);

    // UI（后处理之后）
    [[nodiscard]] float headTop(const std::string& roleId, float footY, const Viewport& vp) const;
    [[nodiscard]] static ChapterDone chapterDoneOf(const Viewport& vp);
    void drawNamePlates(engine::Engine& engine, const ui::Theme& theme, const WorldFrame& frame,
                        const Viewport& vp);
    void drawObjectiveMarker(engine::Engine& engine, const ui::Theme& theme, const WorldFrame& frame,
                             const Viewport& vp);
    void drawHud(engine::Engine& engine, const ui::Theme& theme, const WorldFrame& frame, const Viewport& vp);

    // ---- 画面状态（advance 推进）----
    bool entered_ = false;          // reset 之后见过地图没有
    std::string seenMap_;
    core::Point lastCell_{};
    StepSlide player_;
    bool walking_ = false;
    float walkClock_ = 0.f;
    float idleClock_ = 0.f;
    Vec2f camera_{};
    float clock_ = 0.f;             // 视觉时钟：闪烁、帧动画、波光的相位
    float fadeClock_ = kMapFadeSeconds;
    float bannerClock_ = 0.f;
    bool bannerAfterFade_ = false;
    bool advanced_ = false;         // 上一次 render 之后 advance 过没有

    // ---- 资源（render 时按需载入，无头模式从不碰）----
    bool atlasTried_ = false;
    bool atlasOk_ = false;
    SpriteIndex atlas_;
    std::map<std::string, engine::TextureId> sheetTextures_;
    engine::TextureId objectsTexture_ = engine::kInvalidTexture;
    engine::TextureId emissiveTexture_ = engine::kInvalidTexture;
    engine::TextureId shadowTexture_ = engine::kInvalidTexture;
    engine::TextureId glowTexture_ = engine::kInvalidTexture;
    MapAssets assets_;
    std::unique_ptr<engine::PostFx> fx_;
    hud::PlaceBanner banner_;
    hud::Geometry geometry_;
    std::set<std::string> warned_;
    std::vector<Drawable> drawables_;   // 每帧复用
    std::vector<engine::RectF> plates_; // 本帧已经落下的名牌（后来的避开它们往上叠）
};

}  // namespace fanren::game
