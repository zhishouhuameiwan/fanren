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
//     ② 地面标记：开着的机关、传送箭头与门光、可交互处的闪光、目标光圈、水面波光（加色）
//     ③ 人物脚下柔影（NPC、明雷、跟随者、主角）
//     ④ 按脚底 y 排序：设施、NPC、关着的机关、宝箱、采集点、明雷、跟随者、主角（精灵按每张表自己的倍数；
//        缺图 → MapArt::drawFigure / 旧设施底板 / 新对象的占位底板，见 WorldProps.h）
//     ⑤ 上层：烘焙 above.png，主角周围约 2 格半径渐隐（缺图 → front 层旧图元，同样渐隐）
//     ⑥ 粒子：满屏落下的（花瓣、落叶）与世界里的（尘埃、萤火、薄雾、定点火星）
//     ⑦ 辉光通道：灯的光晕、设施的发光帧、箭头与闪光、目标光圈、萤火
//     ⑧ 点光源：meta.json 的灯 + 主角随行微光（夜 / 黄昏 / 室内）
//   后处理 endScene（光照 → 辉光 → 景深 → 暗角 → 调色）
//   UI：名牌、头顶气泡、明雷头顶标记、目标菱形或边缘箭头、地名横幅、目标框、存盘提示、换图淡入的黑幕
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
#include "game/WorldProps.h"

namespace fanren::game {

class Application;
namespace minimap {
class MapThumbnail;   // game/Minimap.h（它要 include 本文件，这里只前置声明）
}

// 像素尺度（施工图 1.1）：地图与人物是 16px 的像素美术，放大三倍，一格在屏幕上 48 像素。
// 逻辑格仍是 engine::kTileSize = 32：地图、碰撞、对象、测试全都不动，48 只活在渲染层。
inline constexpr int kArtTilePx = 16;
inline constexpr int kArtScale = 3;
inline constexpr int kWorldTilePx = kArtTilePx * kArtScale;
// 手绘高清地图（第 7 章美术重构，2026-11-19）：Seedream 整张重画、24 像素一格、×2 画——屏幕上一格仍是 48，
// 与手绘人物（×2）同一种像素颗粒。图在 art/maps_hd/<图 id>/，有就优先用，没有照旧用 16 像素的烘焙图。
inline constexpr int kHdArtTilePx = 24;
static_assert(kHdArtTilePx * 2 == kWorldTilePx, "高清地图 ×2 后一格要与旧图同大");
// 手绘物件（宝箱、采集点、机关、设施，tools/artgen/hd_objects.py）：只在高清地图上用，×2 画，与地图同一种颗粒。
// 表在 art/sprites/hd_objects/objects.json（与 index.json 的 objects / facilities 同一个写法），没登记的物件照旧用 ×3 的生成图。
inline constexpr int kHdObjectScale = 2;

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
    // 从画面上当前的位置（不一定是上一格，可能还在滑）滑向新的一格，滑 seconds 秒（缺省与主角一步同长；
    // 明雷比主角走得慢，一步滑多久由它自己的步长定）。
    void moveTo(Vec2f cell, float seconds = kStepSlideSeconds);
    void advance(float dt);
    [[nodiscard]] Vec2f position() const;
    [[nodiscard]] bool moving() const { return elapsed_ < duration_; }
    [[nodiscard]] Vec2f target() const { return to_; }
    // 这一步已经滑了多少秒（没在滑时是整步的时长或 0）。跟随者起步时拿主角的这个数对齐进度。
    [[nodiscard]] float elapsed() const { return elapsed_; }

private:
    Vec2f from_{};
    Vec2f to_{};
    float elapsed_ = 0.f;
    float duration_ = 0.f;
};

// 走路帧的姿态：滑步在走就播走路帧，停下后再留 kWalkGraceSeconds 才回站姿（按住方向键时两步之间那不到一帧的
// 空当，腿不闪回站姿）。主角与每位跟随者各一份，同一套规矩。
struct WalkPose {
    bool walking = false;
    float walkClock = 0.f;      // 这一段连续走路累计的秒数（walkFrame 的输入）
    float idleClock = 0.f;
    // moving：滑步此刻还没走完。startClock：起步那一刻的 walkClock——直接落在「迈甲」那一帧，
    // 从站立帧起播的话第一格是端着架子平移过去的。
    void update(bool moving, float dt, float startClock);
    void stop();
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

    // ---- 伙伴改版（docs/interfaces-companions.md 3.4–3.7 与第 6 节）：**只定结构**。----
    // 世界逻辑路（WorldScene）照规则判好往里填，世界画面路（WorldView）照着画；两路都不另起一套。
    // 跟着主角走的伙伴：上场伙伴按队伍次序，第 0 位紧跟主角。格是逻辑位置，滑步插值归画面。
    struct Follower {
        std::string roleId;        // 队伍里存的角色 id（取外观用它）
        core::Point cell;
        int facing = 2;            // 同 GameState::facing
    };
    struct Chest {
        const core::MapObject* object = nullptr;
        bool opened = false;       // 开过就一直是开着的样子
    };
    struct Gather {
        const core::MapObject* object = nullptr;
        bool depleted = false;     // 冷却中：画成采过的样子
    };
    // 只放此刻在场的明雷（清掉了、还没到重生日子的不放）。
    struct Foe {
        const core::MapObject* object = nullptr;
        std::string look;          // foe 的 look 属性：画哪个角色 / 敌人
        int facing = 2;            // 此刻的朝向（会走的明雷按它的站位，不走的是 facing 属性），同 GameState::facing
        bool elite = false;
        // ---- 巡逻与追击（docs/ch07-playtest-foes.md）----
        // 此刻站在哪一格（逻辑格；滑步插值归画面）。(-1,-1) = 图上那一格：手搭的帧、改版前的样子不必填它。
        core::Point cell{-1, -1};
        bool alert = false;        // 在追：头顶冒「!」（代替明雷标记）
        float stepSeconds = 0.f;   // 走一格多久：画面上滑一步就滑这么久（0 = 与主角一步同长）
    };
    struct Gate {
        const core::MapObject* object = nullptr;
        bool open = false;         // 开了之后不占格，画成开着的样子
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
    std::string pathActionHint;                     // 面向可交互 NPC 时的键位与行动名
    // 按确认会发生什么（2026-11-20 用户试玩：「离近之后要有对应按钮提示」）：「[Z] 打开」之类，
    // 浮在那一格的头顶。空串 = 面前没有能按的东西。interactNpc 非空时浮在那个人的名牌上方。
    std::string interactHint;
    core::Point interactCell{-1, -1};
    const core::MapObject* interactNpc = nullptr;
    // 这张图的收集进度（2026-11-20 用户试玩：「找东西没啥回报感」，docs/ch07-playtest-ui.md 第 3 节）：
    // 「宝箱 2/3 · 采集 1/2」，画在目标框底下。空串 = 不显示（历练里、图上没有宝箱采集点）。
    std::string exploreLine;
    bool exploreComplete = false;                   // 找齐了：那一行换成金字
    std::vector<Follower> followers;                // 空 = 没人跟（改版前的样子）
    std::vector<Chest> chests;
    std::vector<Gather> gathers;
    std::vector<Foe> foes;
    std::vector<Gate> gates;

    // ---- 小地图（第 7 章改版·路 M，docs/ch07-foundation.md 3.1）----
    // minimap：这一帧画不画右上角那块小地图（设置开着、世界层在最上面、没有对白与过场）。
    // objects：此刻用的那份对象状态（采集点「采过」看它）；heroFacing：主角朝向（小地图上那个三角）。
    bool minimap = false;
    const core::WorldObjectState* objects = nullptr;
    int heroFacing = 2;
    std::string minimapKey;   // 小地图右下角那一小行「M 地图」（跟着键位与设备走，经 Application::text）

    void clear();
};

// ---------------------------------------------------------------------------
// 跟随者的滑步（伙伴改版 3.7）：帧里只给格，滑不滑由画面自己判——与主角同一个做法
// ---------------------------------------------------------------------------
// 每位跟随者一个 StepSlide：格变了一格就滑过去，变了不止一格就瞬移。整列清掉重来（全体瞬移、重排）的三种情形：
// 主角这一帧是瞬移的（开场、换图、脚本在同图上挪人——epoch 变了）、名单变了（人数或次序或角色 id）、第一次见。
class FollowerTrack {
public:
    struct Slot {
        std::string roleId;
        core::Point cell{};
        int facing = 2;
        StepSlide slide;
        WalkPose pose;
    };

    // epoch：主角最近一次瞬移的序号（WorldView 自己数）。leader / leaderPose：主角此刻的滑步与走路帧——
    // 这一刻起步的跟随者直接对齐主角这一步已经走过的进度（主角没在滑就从 0 起），走路帧也跟主角同一拍。
    // 不拿「上一帧 advance 推了多少秒」去猜：游戏里 WorldScene::update 先 advance、后 tryStep，主角要到 render 里
    // 才起步、那一刻进度是 0；测试里也可能反过来。对齐主角此刻的进度，两种次序都同一帧起步、同一帧到。
    void sync(const std::vector<WorldFrame::Follower>& followers, unsigned epoch, const StepSlide& leader,
              const WalkPose& leaderPose);
    void advance(float dt, float walkStart);
    void clear();
    [[nodiscard]] const std::vector<Slot>& slots() const { return slots_; }

private:
    std::vector<Slot> slots_;
    unsigned epoch_ = 0;
    bool seen_ = false;
};

// 这只明雷此刻的逻辑格：帧里填了 cell 就是它，没填（-1,-1）是图上那一格。
[[nodiscard]] core::Point foeCellOf(const WorldFrame::Foe& foe);

// ---------------------------------------------------------------------------
// 明雷的滑步（巡逻与追击，docs/ch07-playtest-foes.md）：帧里只给格，滑不滑由画面自己判——与跟随者同一个做法
// ---------------------------------------------------------------------------
// 按明雷 id 认人。格变了一格就照帧里的 stepSeconds 滑过去；变了不止一格（回原位、读档）就瞬移；
// 换了图（mapId 不同）整张表清掉重来。帧里没有了的（打赢了、旗标撤了）从表里拿掉。
// 妖兽表没有走路帧：走着的时候画面上颠一颠（walkClock 是颠动的相位），人形表照常播走路帧。
class FoeTrack {
public:
    struct Slot {
        std::string id;
        core::Point cell{};
        StepSlide slide;
        float walkClock = 0.f;     // 这一段连着走了多久；停下归零
    };

    void sync(const std::vector<WorldFrame::Foe>& foes, const std::string& mapId);
    void advance(float dt);
    void clear();
    [[nodiscard]] const Slot* find(const WorldFrame::Foe& foe) const;
    [[nodiscard]] const std::vector<Slot>& slots() const { return slots_; }

private:
    std::vector<Slot> slots_;
    std::string mapId_;
};

// ---------------------------------------------------------------------------
// 立着的东西按什么次序画（纯函数，不开窗口可测）
// ---------------------------------------------------------------------------
// 世界像素（48px 一格）里的脚底点排序：设施、NPC、关着的机关、宝箱、采集点、明雷、跟随者、主角一起排。
// 登记次序就是上面这个次序——叠在主角格上的跟随者（换图之后）排在主角前面画，被主角挡住。
// 开着的机关不在这里：它画在地面一层（WorldProps.h 的 gateStands）。
// view 是镜头看得见的那块（世界像素）：NPC、新对象、跟随者出了它左右两格、上一格、下两格就不排（设施照旧全排，
// 由 drawFacility 自己裁）。
// foeCells 是明雷画面上的格（滑步中带小数），与 frame.foes 一一对应；不给（nullptr）就按 foeCellOf 的逻辑格。
struct WorldDrawItem {
    enum class Kind { Facility, Npc, Player, Follower, Chest, Gather, Gate, Foe };
    DepthKey key;
    Kind kind = Kind::Npc;
    std::size_t index = 0;      // 在各自列表（frame.facilities / npcs / chests … / 跟随者）里的下标；主角恒为 0
};
[[nodiscard]] std::vector<WorldDrawItem> depthOrder(const WorldFrame& frame, Vec2f playerCell,
                                                    const std::vector<Vec2f>& followerCells,
                                                    const engine::RectF& view,
                                                    const std::vector<Vec2f>* foeCells = nullptr);

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

    // 跟着帧里的跟随者名单与格走一步（伙伴改版 3.7）：render 每帧开头自己调；无头下没有 render，
    // 测试直接调它，与 render 走的是同一条。
    void trackFollowers(Application& app, const WorldFrame& frame);
    // 明雷同理（巡逻与追击）：render 每帧开头调；测试直接调。
    void trackFoes(Application& app, const WorldFrame& frame);

    // ---- 给测试与调试看的状态 ----
    [[nodiscard]] Vec2f playerCell() const { return player_.position(); }
    [[nodiscard]] bool playerMoving() const { return player_.moving(); }
    [[nodiscard]] bool walking() const { return pose_.walking; }
    [[nodiscard]] const std::vector<FollowerTrack::Slot>& followers() const { return followers_.slots(); }
    [[nodiscard]] const std::vector<FoeTrack::Slot>& foes() const { return foes_.slots(); }
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
        int artCell = kArtTilePx;   // 这张图的烘焙图一格几像素：16（程序烘焙）或 kHdArtTilePx（手绘高清）
        bool hd = false;            // 用的是 art/maps_hd/ 的手绘图（设施不再有烘进图里的，一律运行时画）
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

    void sync(Application& app);
    [[nodiscard]] float walkStartClock() const;
    [[nodiscard]] Vec2f focusCell() const;

    void ensureAtlas(Application& app);
    void ensureMap(Application& app, const core::TileMap& map);
    void releaseMap(engine::Engine& engine);
    // dir 下的 file：尺寸要恰好是「格数 × artCell」。quietIfMissing：找不到不提示（手绘图是可选的）。
    engine::TextureId loadBaked(engine::Engine& engine, const core::TileMap& map, const std::string& dir,
                                const char* file, int artCell, bool quietIfMissing);
    void setupParticles(const core::TileMap& map);
    // 人物表与敌人表的图（file 相对 art/sprites/），按文件缓存；载不进来警告一次、返回无效句柄。
    engine::TextureId sheetTexture(engine::Engine& engine, const std::string& file);
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
                          engine::BlendMode blend, float scale = static_cast<float>(kArtScale));
    // 高清地图上这个物件的手绘版（没有、或不在高清地图上返回 nullptr）。
    [[nodiscard]] const ObjectSprite* hdObjectSprite(const std::string& id) const;
    void drawPortals(engine::Engine& engine, const core::TileMap& map, const WorldFrame& frame,
                     const Viewport& vp, bool emissivePass);
    void drawHotspots(engine::Engine& engine, const WorldFrame& frame, const Viewport& vp,
                      bool emissivePass);
    void drawObjectiveRing(engine::Engine& engine, const WorldFrame& frame, const Viewport& vp);
    void drawShadows(engine::Engine& engine, const WorldFrame& frame, const Viewport& vp);
    void drawSorted(engine::Engine& engine, Application& app, const WorldFrame& frame, const Viewport& vp);
    void drawCharacter(engine::Engine& engine, const std::string& roleId, bool player, float footX,
                       float footY, int facing, bool moving, float walkClock, const Viewport& vp);
    // 四种新对象（伙伴改版 2.4、第 6 节）。宝箱、采集点、机关按对象矩形底边中点落地。
    void drawProp(engine::Engine& engine, const core::MapObject& object, const char* spriteId,
                  const PropPlaceholder& placeholder, const Viewport& vp);
    void drawOpenGates(engine::Engine& engine, const WorldFrame& frame, const Viewport& vp);
    void drawFoe(engine::Engine& engine, const WorldFrame::Foe& foe, const Viewport& vp);
    // 明雷画面上的格（滑步中带小数）：表里有它就是滑到的那一点，没有就是逻辑格。
    [[nodiscard]] Vec2f foeDrawCell(const WorldFrame::Foe& foe) const;
    [[nodiscard]] float foeHeadTop(const WorldFrame::Foe& foe, float footY, const Viewport& vp) const;
    void drawFoeMarks(engine::Engine& engine, const WorldFrame& frame, const Viewport& vp);
    void warnMissingProp(const char* spriteId);
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
    // 右上角的小地图（docs/ch07-foundation.md 3.1）。covering：它盖住主角时淡成半透明。
    void drawMinimap(engine::Engine& engine, const ui::Theme& theme, const WorldFrame& frame, const Viewport& vp,
                     bool covering);

    // ---- 画面状态（advance 推进）----
    bool entered_ = false;          // reset 之后见过地图没有
    std::string seenMap_;
    core::Point lastCell_{};
    StepSlide player_;
    WalkPose pose_;                 // 主角的走路帧
    unsigned snapEpoch_ = 0;        // 主角每瞬移一次加一：跟随者据此整列瞬移
    FollowerTrack followers_;
    FoeTrack foes_;                 // 会走的明雷的滑步
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
    SpriteIndex hdObjects_;   // 手绘物件表（只用 objects / facilities 两样）
    engine::TextureId hdObjectsTexture_ = engine::kInvalidTexture;
    engine::TextureId shadowTexture_ = engine::kInvalidTexture;
    engine::TextureId glowTexture_ = engine::kInvalidTexture;
    MapAssets assets_;
    std::unique_ptr<engine::PostFx> fx_;
    hud::PlaceBanner banner_;
    hud::Geometry geometry_;
    std::set<std::string> warned_;
    std::vector<WorldDrawItem> drawables_;   // 每帧复用
    std::vector<engine::RectF> plates_; // 本帧已经落下的名牌（后来的避开它们往上叠）
    std::unique_ptr<minimap::MapThumbnail> minimap_;   // 小地图的底图（换图才重建）
    const core::TileMap* minimapMap_ = nullptr;        // 这一帧的地图（render 记下，drawMinimap 用）
};

}  // namespace fanren::game
