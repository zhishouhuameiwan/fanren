#include "game/WorldView.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <string_view>
#include <utility>

#include "game/Application.h"
#include "game/MapArt.h"
#include "io/VisualLoader.h"

namespace fanren::game {
namespace {

using engine::BlendMode;
using engine::Color;
using engine::RectF;
using engine::Vertex;

constexpr float kCell = static_cast<float>(kWorldTilePx);
constexpr float kArtCell = static_cast<float>(kArtTilePx);
constexpr float kPixel = static_cast<float>(kArtScale);
constexpr float kViewW = static_cast<float>(engine::kLogicalWidth);
constexpr float kViewH = static_cast<float>(engine::kLogicalHeight);
constexpr float kTwoPi = 2.f * std::numbers::pi_v<float>;

// 主角的外观按这个角色 id 查（韩立；第 1–2 章少年版由 role_variants 管）。
constexpr const char* kPlayerRole = "hanli";

// 镜头追主角的快慢：每秒收掉剩下距离的 1 − e^−10。走路时镜头落后主角不到一格，
// 停下后约 0.3 秒对正——够「跟着」，又不至于像钉在主角身上那样一步一抖。
constexpr float kCameraRate = 10.f;

// 停下来之后还按「在走」算多久。按住方向键时两步之间有不到一帧的空当（冷却 0.12 秒按帧
// 取整成 8 帧，滑动 7.2 帧就到了）；不留这点余量，腿会在每一步之间闪回站姿。
constexpr float kWalkGraceSeconds = 0.06f;

// 树冠渐隐：以主角身子为圆心，1.25 格内压到 0.3，2.75 格外恢复不透明。
// 逐顶点算透明度的窗口取 ±3 格，比渐隐半径大一圈，窗口边上恒为 1，与窗口外接得上。
constexpr float kCanopyInner = 1.25f;
constexpr float kCanopyOuter = 2.75f;
constexpr float kCanopyMinAlpha = 0.3f;
constexpr float kCanopyWindowCells = 3.f;

// 地名横幅的时间线（秒）。
constexpr float kBannerDelay = 0.15f;    // 换图后等黑幕淡到一半再出来
constexpr float kBannerFadeIn = 0.5f;
constexpr float kBannerHold = 3.0f;
constexpr float kBannerFadeOut = 0.8f;

// 交互闪光：四帧由小到大再散开，放完歇一会儿再闪——一直闪的点看久了是噪点。
constexpr float kSparkleCycle = 2.2f;

constexpr Color kWhite{255, 255, 255, 255};
constexpr Color kPortalOpen{176, 214, 255, 210};    // 开着的门：冷白
constexpr Color kPortalGuide{255, 206, 104, 255};   // 通往目标的那一道：金
constexpr Color kPortalLocked{120, 96, 96, 110};    // 关着的门：暗红，几乎只剩个影
constexpr Color kSparkle{255, 236, 190, 255};
constexpr Color kGlint{210, 236, 255, 255};         // 水面波光
constexpr Color kObjectiveGlow{255, 198, 92, 255};  // 目标光圈

// 旧画法（缺物件图集时）沿用的颜色与字号，与改造前 WorldScene 里那几个同值。
constexpr Color kPortalFill{26, 30, 44, 210};
constexpr Color kPortalEdge{132, 180, 224, 255};
constexpr Color kPortalLockedEdge{124, 112, 108, 255};
constexpr Color kGlyphDark{18, 16, 20, 255};
constexpr Color kHintEdge{255, 226, 150, 90};
constexpr Color kHintMark{255, 226, 150, 210};
constexpr Color kObjectiveColor{255, 214, 96, 255};
constexpr int kGlyphFontSize = 22;

[[nodiscard]] float fl(int v) { return static_cast<float>(v); }

// 对齐到屏幕像素：像素美术的边不落在半像素上，三层图（地面、人物、树冠）才严丝合缝。
[[nodiscard]] float snap(float v) { return std::round(v); }

[[nodiscard]] std::uint32_t mixBits(std::uint32_t h) {
    h ^= h >> 16;
    h *= 0x7FEB352Du;
    h ^= h >> 15;
    h *= 0x846CA68Bu;
    h ^= h >> 16;
    return h;
}

// 每格一个稳定的杂凑：波光每格位置不同、每帧却一样（同 MapArt 的口径，不用 rand）。
[[nodiscard]] std::uint32_t cellHash(int x, int y) {
    return mixBits(static_cast<std::uint32_t>(x) * 73856093u ^ static_cast<std::uint32_t>(y) * 19349663u);
}

[[nodiscard]] std::uint32_t textHash(std::string_view text) {
    std::uint32_t hash = 2166136261u;   // FNV-1a
    for (const char ch : text) {
        hash ^= static_cast<std::uint32_t>(static_cast<unsigned char>(ch));
        hash *= 16777619u;
    }
    return hash;
}

[[nodiscard]] Color withAlpha(Color c, float alpha) {
    return hud::faded(c, alpha);
}

[[nodiscard]] engine::Rect inset(const engine::Rect& rect, int by) {
    return engine::Rect{rect.x + by, rect.y + by, std::max(1, rect.w - by * 2), std::max(1, rect.h - by * 2)};
}

[[nodiscard]] Color darker(const Color& color) {
    const auto dim = [](std::uint8_t channel) { return static_cast<std::uint8_t>(channel / 2); };
    return Color{dim(color.r), dim(color.g), dim(color.b), color.a};
}

// 从天上落下来的（花瓣、落叶、雨、雪）按屏幕坐标铺满画面：天气是跟着镜头走的；
// 浮在空气里的（尘埃、萤火、雾、火星）按世界坐标，镜头一动它们留在原地，走路时才有纵深。
[[nodiscard]] bool falls(engine::ParticleKind kind) {
    return kind == engine::ParticleKind::Petal || kind == engine::ParticleKind::Leaf ||
           kind == engine::ParticleKind::Rain || kind == engine::ParticleKind::Snow;
}

// 进图第一帧就是「已经下了一会儿」的样子：空跑到最慢的那颗走完一程（寿命见 Particles.cpp）。
[[nodiscard]] float prewarmSeconds(engine::ParticleKind kind) {
    switch (kind) {
        case engine::ParticleKind::Dust: return 10.f;
        case engine::ParticleKind::Firefly: return 9.f;
        case engine::ParticleKind::Petal: return 25.f;
        case engine::ParticleKind::Leaf: return 20.f;
        case engine::ParticleKind::Rain: return 2.f;
        case engine::ParticleKind::Snow: return 30.f;
        case engine::ParticleKind::Ember: return 3.f;
        case engine::ParticleKind::Mist: return 24.f;
    }
    return 10.f;
}

// 定点发射器的形状（世界像素）：发射区多大、每秒几颗、粒子会飘出去多远（裁剪余量）。
struct EmitterShape {
    float w;
    float h;
    float rate;
    float margin;
};
[[nodiscard]] EmitterShape emitterShape(engine::ParticleKind kind) {
    switch (kind) {
        case engine::ParticleKind::Ember: return {30.f, 10.f, 7.f, 280.f};    // 火盆口一窄条，往上冒
        case engine::ParticleKind::Mist: return {144.f, 72.f, 0.3f, 220.f};   // 水面起雾
        case engine::ParticleKind::Firefly: return {144.f, 96.f, 0.7f, 64.f}; // 水边萤火
        case engine::ParticleKind::Dust: return {96.f, 96.f, 1.2f, 32.f};
        // 树冠下的落叶 / 花瓣：从树冠那一截落到地上（落下来的粒子从发射区顶边进、底边出），
        // 一棵树每三秒来一片——一张图上几十棵树，合起来才是「秋天」而不是「下叶子雨」。
        case engine::ParticleKind::Petal:
        case engine::ParticleKind::Leaf: return {96.f, 120.f, 0.35f, 160.f};
        case engine::ParticleKind::Rain:
        case engine::ParticleKind::Snow: return {96.f, 48.f, 1.f, 320.f};
    }
    return {96.f, 96.f, 1.f, 64.f};
}

// 整张图的火星（密室那种「空气里飘着的火星」）按横条铺：火星从发射区底边往上冒、
// 两三秒就熄，一整张图当一个发射区，就只有最底下几行在冒火星。
constexpr float kEmberBandCells = 4.f;

// 灯的光晕在辉光通道里画多大（格）：光照半径管「照亮多大一片」，这个管「发光体本身多大」。
[[nodiscard]] float glowCells(const std::string& kind) {
    if (kind == "furnace") return 1.6f;
    if (kind == "fire") return 1.4f;
    if (kind == "torch") return 1.2f;
    if (kind == "lantern") return 1.1f;
    if (kind == "save") return 1.0f;
    if (kind == "sconce" || kind == "window") return 0.9f;
    if (kind == "stone_lantern") return 0.8f;
    if (kind == "candle") return 0.7f;
    return 1.f;
}

// 每盏灯各闪各的：相位由位置杂凑出来。
[[nodiscard]] float lightPhase(const MapLight& light) {
    const auto h = cellHash(static_cast<int>(light.x * 10.f), static_cast<int>(light.y * 10.f));
    return static_cast<float>(h & 1023u) / 1024.f * kTwoPi;
}

[[nodiscard]] const char* arrowSprite(int facing) {
    switch (facing) {
        case 0: return "portal.arrow_up";
        case 1: return "portal.arrow_right";
        case 3: return "portal.arrow_left";
        default: return "portal.arrow_down";
    }
}

// 树冠渐隐量距离的圆心：人物身子的中段（精灵站在格底边，身子在格中偏上）。
[[nodiscard]] Vec2f canopyCenter(Vec2f player) {
    return Vec2f{player.x + 0.5f, player.y + 0.25f};
}

// 门矩形外紧挨着的一排格子：有几格走得通、有没有出了地图。
struct SideInfo {
    int walkable = 0;
    bool outside = false;
};

SideInfo sideOf(const core::TileMap& map, const core::MapObject& portal, int dir) {
    SideInfo info;
    const int x0 = portal.position.x;
    const int y0 = portal.position.y;
    const int w = std::max(1, portal.width);
    const int h = std::max(1, portal.height);
    const auto probe = [&](int x, int y) {
        if (!map.inBounds(core::Point{x, y})) {
            info.outside = true;
            return;
        }
        if (map.walkable(core::Point{x, y})) ++info.walkable;
    };
    switch (dir) {
        case 0: for (int x = x0; x < x0 + w; ++x) probe(x, y0 - 1); break;
        case 1: for (int y = y0; y < y0 + h; ++y) probe(x0 + w, y); break;
        case 2: for (int x = x0; x < x0 + w; ++x) probe(x, y0 + h); break;
        default: for (int y = y0; y < y0 + h; ++y) probe(x0 - 1, y); break;
    }
    return info;
}

}  // namespace

// ---------------------------------------------------------------------------
// 纯函数
// ---------------------------------------------------------------------------

float cameraAxis(float focus, float mapExtent, float viewExtent) {
    if (mapExtent <= viewExtent) return -(viewExtent - mapExtent) / 2.f;
    return std::clamp(focus - viewExtent / 2.f, 0.f, mapExtent - viewExtent);
}

Vec2f cameraTarget(int mapWidthCells, int mapHeightCells, Vec2f focusCell) {
    return Vec2f{cameraAxis(focusCell.x * kCell, fl(mapWidthCells) * kCell, kViewW),
                 cameraAxis(focusCell.y * kCell, fl(mapHeightCells) * kCell, kViewH)};
}

float approach(float current, float target, float dt, float rate) {
    if (dt <= 0.f || rate <= 0.f) return current;
    return target + (current - target) * std::exp(-rate * dt);
}

void StepSlide::snap(Vec2f cell) {
    from_ = cell;
    to_ = cell;
    elapsed_ = 0.f;
    duration_ = 0.f;
}

void StepSlide::moveTo(Vec2f cell) {
    from_ = position();
    to_ = cell;
    elapsed_ = 0.f;
    duration_ = kStepSlideSeconds;
}

void StepSlide::advance(float dt) {
    if (dt > 0.f) elapsed_ = std::min(duration_, elapsed_ + dt);
}

Vec2f StepSlide::position() const {
    if (duration_ <= 0.f) return to_;
    // 匀速：连着走的时候每一格都是同一个速度，缓入缓出会让人一格一顿。
    const float t = std::clamp(elapsed_ / duration_, 0.f, 1.f);
    return Vec2f{from_.x + (to_.x - from_.x) * t, from_.y + (to_.y - from_.y) * t};
}

bool drawsBefore(const DepthKey& a, const DepthKey& b) {
    if (a.footY != b.footY) return a.footY < b.footY;
    if (a.x != b.x) return a.x < b.x;
    return a.order < b.order;
}

float canopyAlpha(float distanceCells) {
    if (distanceCells <= kCanopyInner) return kCanopyMinAlpha;
    if (distanceCells >= kCanopyOuter) return 1.f;
    const float t = (distanceCells - kCanopyInner) / (kCanopyOuter - kCanopyInner);
    const float s = t * t * (3.f - 2.f * t);
    return kCanopyMinAlpha + (1.f - kCanopyMinAlpha) * s;
}

float mapFadeAlpha(float seconds) {
    return std::clamp(1.f - seconds / kMapFadeSeconds, 0.f, 1.f);
}

float bannerAlpha(float seconds, bool afterFade) {
    float shown = seconds;
    if (afterFade) {
        shown = seconds - kBannerDelay;
        if (shown <= 0.f) return 0.f;
        if (shown < kBannerFadeIn) return shown / kBannerFadeIn;
        shown -= kBannerFadeIn;
    }
    if (shown < kBannerHold) return 1.f;
    return std::clamp(1.f - (shown - kBannerHold) / kBannerFadeOut, 0.f, 1.f);
}

int portalArrowFacing(const core::TileMap& map, const core::MapObject& portal) {
    int best = 2;
    int bestScore = -1;
    for (int dir = 0; dir < 4; ++dir) {
        const SideInfo ahead = sideOf(map, portal, dir);
        const SideInfo behind = sideOf(map, portal, (dir + 2) % 4);
        // 往 dir 迈进门里去：dir 那一侧必须走不通（墙或地图外），身后必须有走得通的来路。
        if (ahead.walkable > 0 || behind.walkable == 0) continue;
        // 贴着地图边的门优先：人就是从那条边走出去的。同分时来路宽的那一向优先。
        const int score = behind.walkable + (ahead.outside ? 100 : 0);
        if (score > bestScore) {
            bestScore = score;
            best = dir;
        }
    }
    return best;
}

bool portalIsDoorway(const core::TileMap& map, const core::MapObject& portal) {
    return portalArrowFacing(map, portal) == 0 && !sideOf(map, portal, 0).outside;
}

bool facilitySpriteAtRuntime(const std::string& kind, bool belowBaked) {
    if (!belowBaked) return true;
    return kind != "board" && kind != "meditate";
}

const char* pathBubbleSprite(PathBubble bubble) {
    switch (bubble) {
        case PathBubble::Generic: return "bubble.path";
        case PathBubble::Inquire: return "bubble.path_inquire";
        case PathBubble::Purchase: return "bubble.path_buy";
        case PathBubble::Challenge: return "bubble.path_duel";
        case PathBubble::None: break;
    }
    return nullptr;
}

void WorldFrame::clear() {
    npcs.clear();
    portals.clear();
    facilities.clear();
    hotspots.clear();
    objectiveTarget = nullptr;
    objectiveText.clear();
    objectiveWhere.clear();
    mapTitle.clear();
    dangerStars = 0;
    saveNotice.clear();
}

// ---------------------------------------------------------------------------
// 画面状态
// ---------------------------------------------------------------------------

WorldView::WorldView() = default;
WorldView::~WorldView() = default;

void WorldView::reset() {
    entered_ = false;
    seenMap_.clear();
    advanced_ = false;
}

Vec2f WorldView::focusCell() const {
    const Vec2f p = player_.position();
    return Vec2f{p.x + 0.5f, p.y + 0.5f};
}

void WorldView::sync(Application& app) {
    const core::TileMap* map = app.currentMap();
    if (map == nullptr) return;
    const core::Point cell = app.state().position;
    const Vec2f at{fl(cell.x), fl(cell.y)};

    if (!entered_ || map->id != seenMap_) {
        // 开场或换图：人与镜头就位，不滑、不追。换图还要从黑里淡进来（逻辑上早已换好）；
        // 开场不淡——外层的开场转场自己会淡。
        const bool first = !entered_;
        entered_ = true;
        seenMap_ = map->id;
        lastCell_ = cell;
        player_.snap(at);
        walking_ = false;
        walkClock_ = 0.f;
        idleClock_ = 0.f;
        camera_ = cameraTarget(map->width, map->height, focusCell());
        fadeClock_ = first ? kMapFadeSeconds : 0.f;
        bannerClock_ = 0.f;
        bannerAfterFade_ = !first;
        return;
    }
    if (cell == lastCell_) return;
    const int steps = std::abs(cell.x - lastCell_.x) + std::abs(cell.y - lastCell_.y);
    lastCell_ = cell;
    if (steps == 1) {
        player_.moveTo(at);
        return;
    }
    // 不是相邻格：脚本在同一张图上把人挪走了。人与镜头一起跳过去——
    // 一路平移过去，像是在放一段没人要求的过场。
    player_.snap(at);
    camera_ = cameraTarget(map->width, map->height, focusCell());
}

void WorldView::updateWalk(float dt) {
    if (player_.moving()) {
        if (!walking_) {
            // 起步直接从「迈甲」那一帧开始：从站立帧起播，第一格就是端着架子平移过去的。
            walking_ = true;
            walkClock_ = atlasOk_ && atlas_.walkFps > 0.f ? 1.f / atlas_.walkFps : 0.125f;
        } else {
            walkClock_ += dt;
        }
        idleClock_ = 0.f;
        return;
    }
    if (!walking_) return;
    idleClock_ += dt;
    walkClock_ += dt;
    if (idleClock_ > kWalkGraceSeconds) {
        walking_ = false;
        walkClock_ = 0.f;
    }
}

void WorldView::advance(Application& app, double deltaSeconds) {
    const float dt = static_cast<float>(std::max(0.0, deltaSeconds));
    sync(app);
    clock_ += dt;
    player_.advance(dt);
    updateWalk(dt);
    fadeClock_ += dt;
    bannerClock_ += dt;
    if (const core::TileMap* map = app.currentMap()) {
        const Vec2f target = cameraTarget(map->width, map->height, focusCell());
        camera_.x = approach(camera_.x, target.x, dt, kCameraRate);
        camera_.y = approach(camera_.y, target.y, dt, kCameraRate);
    }
    for (ParticleSlot& slot : assets_.particles) slot.system.update(dt);
    advanced_ = true;
}

// ---------------------------------------------------------------------------
// 资源
// ---------------------------------------------------------------------------

void WorldView::warnOnce(const std::string& key, const std::string& message) {
    if (!warned_.insert(key).second) return;
    std::fprintf(stderr, "[world] WARNING: %s\n", message.c_str());
}

void WorldView::ensureAtlas(Application& app) {
    if (atlasTried_) return;
    atlasTried_ = true;
    engine::Engine& engine = app.engine();

    const std::vector<std::string> index = engine.findAssets("art/sprites", "index.json");
    if (index.empty()) {
        warnOnce("atlas", "没有 art/sprites/index.json：人物与物件一律退回旧图元画法");
    } else if (auto loaded = io::loadSpriteIndex(index.front()); loaded) {
        atlas_ = std::move(loaded.value);
        atlasOk_ = true;
    } else {
        warnOnce("atlas", loaded.error + "：人物与物件一律退回旧图元画法");
    }

    if (atlasOk_ && !atlas_.objectsFile.empty()) {
        objectsTexture_ = engine.loadTexture("art/sprites/" + atlas_.objectsFile, engine::ScaleMode::Pixel);
        if (objectsTexture_ == engine::kInvalidTexture) {
            warnOnce("objects", "物件图集 art/sprites/" + atlas_.objectsFile + " 载不进来：设施与传送点退回旧画法");
        }
        if (!atlas_.emissiveFile.empty()) {
            emissiveTexture_ =
                engine.loadTexture("art/sprites/" + atlas_.emissiveFile, engine::ScaleMode::Pixel);
        }
    }
    // 柔影与光晕是柔图，按线性取样放大；缺了各有退路（几何椭圆 / 不画光晕）。
    shadowTexture_ = engine.loadTexture("art/fx/shadow.png", engine::ScaleMode::Linear);
    glowTexture_ = engine.loadTexture("art/fx/glow.png", engine::ScaleMode::Linear);
}

engine::TextureId WorldView::sheetTexture(engine::Engine& engine, const CharacterSheet& sheet) {
    if (const auto it = sheetTextures_.find(sheet.file); it != sheetTextures_.end()) return it->second;
    const engine::TextureId id = engine.loadTexture("art/sprites/" + sheet.file, engine::ScaleMode::Pixel);
    if (id == engine::kInvalidTexture) {
        warnOnce("sheet:" + sheet.file,
                 "人物表 art/sprites/" + sheet.file + " 载不进来：用到它的人退回旧图元画法");
    }
    sheetTextures_.emplace(sheet.file, id);
    return id;
}

const ObjectSprite* WorldView::objectSprite(const std::string& id) const {
    if (objectsTexture_ == engine::kInvalidTexture) return nullptr;
    const auto it = atlas_.objects.find(id);
    return it == atlas_.objects.end() ? nullptr : &it->second;
}

engine::TextureId WorldView::loadBaked(engine::Engine& engine, const core::TileMap& map,
                                       const char* file) {
    const std::string dir = "art/maps/" + map.id;
    const std::vector<std::string> found = engine.findAssets(dir, file);
    if (found.empty()) {
        warnOnce(dir + "/" + file, map.id + " 没有烘焙图 " + dir + "/" + file + "：这一层退回旧图元画法");
        return engine::kInvalidTexture;
    }
    const engine::TextureId id = engine.loadTexture(found.front(), engine::ScaleMode::Pixel);
    const engine::Point size = engine.textureSize(id);
    if (bakedLayerUsable(size, map.width, map.height)) return id;
    if (id == engine::kInvalidTexture) {
        warnOnce(dir + "/" + file, found.front() + " 载不进来：这一层退回旧图元画法");
    } else {
        warnOnce(dir + "/" + file,
                 found.front() + " 的尺寸 " + std::to_string(size.x) + "×" + std::to_string(size.y) +
                     " 与地图 " + std::to_string(map.width) + "×" + std::to_string(map.height) +
                     " 格（应为 ×16 像素）对不上，烘焙与地图脱节了：这一层退回旧图元画法");
        engine.destroyTexture(id);
    }
    return engine::kInvalidTexture;
}

void WorldView::releaseMap(engine::Engine& engine) {
    // 烘焙图一张几 MB，离开一张图就还掉（loadTexture 的缓存项随之摘掉，回来时重读）。
    if (assets_.below != engine::kInvalidTexture) engine.destroyTexture(assets_.below);
    if (assets_.above != engine::kInvalidTexture) engine.destroyTexture(assets_.above);
    assets_.below = engine::kInvalidTexture;
    assets_.above = engine::kInvalidTexture;
}

void WorldView::ensureMap(Application& app, const core::TileMap& map) {
    if (assets_.loaded && assets_.mapId == map.id) return;
    engine::Engine& engine = app.engine();
    releaseMap(engine);
    assets_ = MapAssets{};
    assets_.mapId = map.id;
    assets_.loaded = true;
    assets_.below = loadBaked(engine, map, "below.png");
    assets_.above = loadBaked(engine, map, "above.png");
    assets_.aboveSize = engine.textureSize(assets_.above);

    const std::string dir = "art/maps/" + map.id;
    const std::string fallbackName = map.outdoor ? "昼" : "室内";
    const std::vector<std::string> meta = engine.findAssets(dir, "meta.json");
    if (meta.empty()) {
        warnOnce(dir + "/meta.json", map.id + " 没有 " + dir + "/meta.json：时辰按" + fallbackName +
                                         "，不点灯、不下粒子");
        assets_.visual = fallbackVisual(map.outdoor);
    } else if (auto loaded = io::loadMapMeta(meta.front()); loaded) {
        assets_.visual = mapVisualFrom(loaded.value);
    } else {
        warnOnce(dir + "/meta.json", loaded.error + "：时辰按" + fallbackName + "，不点灯、不下粒子");
        assets_.visual = fallbackVisual(map.outdoor);
    }

    assets_.water.assign(static_cast<std::size_t>(map.width) * static_cast<std::size_t>(map.height), 0);
    for (const core::Point& p : assets_.visual.water) {
        if (!map.inBounds(p)) continue;   // 与地图脱节的格子不画（尺寸那条已经报过警）
        assets_.water[static_cast<std::size_t>(p.y) * static_cast<std::size_t>(map.width) +
                      static_cast<std::size_t>(p.x)] = 1;
    }
    setupParticles(map);
}

void WorldView::setupParticles(const core::TileMap& map) {
    assets_.particles.clear();
    const float mapW = fl(map.width) * kCell;
    const float mapH = fl(map.height) * kCell;
    const float screens = (mapW * mapH) / (kViewW * kViewH);
    // 种子由地图 id 定：同一张图每次进来粒子都长一个样，截图可以前后对比。
    std::uint32_t seed = textHash(map.id);

    for (const ParticleLayer& layer : assets_.visual.particles) {
        if (layer.density <= 0.f) continue;
        if (layer.kind == engine::ParticleKind::Ember) {
            const int bands = std::max(1, static_cast<int>(std::ceil(fl(map.height) / kEmberBandCells)));
            const float bandH = mapH / fl(bands);
            const float rate = particleRate(layer.kind, layer.density, screens) / fl(bands);
            for (int b = 0; b < bands; ++b) {
                ParticleSlot slot{engine::ParticleSystem(seed++)};
                slot.world = true;
                const RectF area{0.f, fl(b) * bandH, mapW, bandH};
                slot.system.configure(layer.kind, rate, area, engine::ParticleSpace::World);
                slot.system.prewarm(prewarmSeconds(layer.kind));
                slot.reach = RectF{area.x, area.y - 300.f, area.w, area.h + 300.f};
                assets_.particles.push_back(std::move(slot));
            }
            continue;
        }
        ParticleSlot slot{engine::ParticleSystem(seed++)};
        const bool screen = falls(layer.kind);
        slot.world = !screen;
        slot.emissive = layer.kind == engine::ParticleKind::Firefly;
        const RectF area = screen ? RectF{0.f, 0.f, kViewW, kViewH} : RectF{0.f, 0.f, mapW, mapH};
        slot.system.configure(layer.kind, particleRate(layer.kind, layer.density, screen ? 1.f : screens),
                              area, screen ? engine::ParticleSpace::Screen : engine::ParticleSpace::World);
        slot.system.prewarm(prewarmSeconds(layer.kind));
        slot.reach = RectF{area.x - 400.f, area.y - 400.f, area.w + 800.f, area.h + 800.f};
        assets_.particles.push_back(std::move(slot));
    }

    for (const ParticleEmitter& emitter : assets_.visual.emitters) {
        const EmitterShape shape = emitterShape(emitter.kind);
        ParticleSlot slot{engine::ParticleSystem(seed++)};
        slot.world = true;
        slot.emissive = emitter.kind == engine::ParticleKind::Firefly;
        const RectF area{emitter.x * kCell - shape.w / 2.f, emitter.y * kCell - shape.h / 2.f, shape.w,
                         shape.h};
        slot.system.configure(emitter.kind, shape.rate, area, engine::ParticleSpace::World);
        slot.system.setCapacity(64);
        slot.system.prewarm(prewarmSeconds(emitter.kind));
        slot.reach = RectF{area.x - shape.margin, area.y - shape.margin, area.w + shape.margin * 2.f,
                           area.h + shape.margin * 2.f};
        assets_.particles.push_back(std::move(slot));
    }
}

WorldView::Viewport WorldView::viewport(Application& app, const core::TileMap& map) const {
    Viewport vp;
    vp.camX = snap(camera_.x);
    vp.camY = snap(camera_.y);
    vp.x0 = std::max(0, static_cast<int>(std::floor(vp.camX / kCell)));
    vp.y0 = std::max(0, static_cast<int>(std::floor(vp.camY / kCell)));
    vp.x1 = std::min(map.width, static_cast<int>(std::floor((vp.camX + kViewW) / kCell)) + 1);
    vp.y1 = std::min(map.height, static_cast<int>(std::floor((vp.camY + kViewH) / kCell)) + 1);
    vp.player = player_.position();
    const ChapterTable& chapters = app.chapterTable();
    for (std::size_t n = 1; n < vp.chapterDone.size(); ++n) {
        vp.chapterDone[n] = chapters.chapterDone(app.state(), static_cast<int>(n));
    }
    return vp;
}

ChapterDone WorldView::chapterDoneOf(const Viewport& vp) {
    return [&vp](int n) {
        return n > 0 && static_cast<std::size_t>(n) < vp.chapterDone.size() &&
               vp.chapterDone[static_cast<std::size_t>(n)];
    };
}

// ---------------------------------------------------------------------------
// 一帧
// ---------------------------------------------------------------------------

void WorldView::render(Application& app, const WorldFrame& frame) {
    const core::TileMap* map = app.currentMap();
    if (map == nullptr) return;
    engine::Engine& engine = app.engine();
    // 对话框压在世界层上时，主循环只 update 最上面那一层，世界层的 advance 轮不到：
    // 这里用引擎的帧间隔补上，树叶照落、灯照闪、那一步照样滑完——不然一开口说话世界就冻住。
    if (!advanced_) advance(app, engine.deltaSeconds());
    advanced_ = false;
    sync(app);
    ensureAtlas(app);
    ensureMap(app, *map);
    if (!fx_) fx_ = std::make_unique<engine::PostFx>(engine);

    const Viewport vp = viewport(app, *map);

    // ① – ⑥ 世界：画进后处理的场景图。
    fx_->beginScene();
    drawBelow(engine, *map, vp);
    drawWaterGlints(engine, *map, vp);
    drawPortals(engine, *map, frame, vp, false);
    drawHotspots(engine, frame, vp, false);
    drawObjectiveRing(engine, frame, vp);
    drawShadows(engine, frame, vp);
    drawSorted(engine, app, frame, vp);
    drawAbove(engine, *map, vp);
    drawParticles(engine, vp, false);

    // ⑦ 发光体另画一份进辉光通道。返回 false（无头、建不出目标）时一笔都不许画。
    if (fx_->beginEmissive()) {
        drawLightGlows(engine, vp);
        for (const WorldFrame::Facility& facility : frame.facilities) drawFacility(engine, facility, vp, true);
        drawPortals(engine, *map, frame, vp, true);
        drawHotspots(engine, frame, vp, true);
        drawObjectiveRing(engine, frame, vp);
        drawParticles(engine, vp, true);
        fx_->endEmissive();
    }

    // ⑧ 点光源，然后合成。
    addLights(vp);
    const float focusY = ((vp.player.y + 0.5f) * kCell - vp.camY) / kViewH;
    fx_->endScene(postFxFor(assets_.visual, focusY));

    // UI：不糊、不压暗、不被调色。
    const ui::Theme& theme = app.theme();
    drawNamePlates(engine, theme, frame, vp);
    drawObjectiveMarker(engine, theme, frame, vp);
    drawHud(engine, theme, frame, vp);
    const float fade = mapFadeAlpha(fadeClock_);
    if (fade > 0.f) {
        engine.fillRect(RectF{0.f, 0.f, kViewW, kViewH}, withAlpha(Color{0, 0, 0, 255}, fade),
                        BlendMode::Alpha);
    }
}

// ---------------------------------------------------------------------------
// 地图的两层
// ---------------------------------------------------------------------------

void WorldView::drawBaked(engine::Engine& engine, engine::TextureId texture, const Viewport& vp, int cx0,
                          int cy0, int cx1, int cy1) {
    if (cx1 <= cx0 || cy1 <= cy0) return;
    // 按整格取源矩形：源坐标永远落在美术像素的整数上，镜头的零头全在目标矩形那一边。
    const RectF src{fl(cx0) * kArtCell, fl(cy0) * kArtCell, fl(cx1 - cx0) * kArtCell,
                    fl(cy1 - cy0) * kArtCell};
    const RectF dst{fl(cx0) * kCell - vp.camX, fl(cy0) * kCell - vp.camY, fl(cx1 - cx0) * kCell,
                    fl(cy1 - cy0) * kCell};
    engine.drawTexture(texture, src, dst, engine::DrawOptions{});
}

void WorldView::drawTileLayer(engine::Engine& engine, const core::TileMap& map,
                              const std::vector<int>& layer, const Viewport& vp, bool fadeNearPlayer) {
    if (layer.empty()) return;
    const TilePalette& palette = tilePalette(map.outdoor);
    const Vec2f center = canopyCenter(vp.player);
    for (int y = vp.y0; y < vp.y1; ++y) {
        for (int x = vp.x0; x < vp.x1; ++x) {
            const auto index = static_cast<std::size_t>(y) * static_cast<std::size_t>(map.width) +
                               static_cast<std::size_t>(x);
            if (index >= layer.size()) continue;
            const int gid = layer[index];
            if (gid <= 0) continue;
            // 碰撞层只为特殊地表那一格分水与土，见 MapArt 的 drawTile。
            const bool blocked = index < map.collision.size() && map.collision[index] != 0;
            const engine::Rect cell{static_cast<int>(fl(x) * kCell - vp.camX),
                                    static_cast<int>(fl(y) * kCell - vp.camY), kWorldTilePx, kWorldTilePx};
            if (fadeNearPlayer) {
                const float dx = fl(x) + 0.5f - center.x;
                const float dy = fl(y) + 0.5f - center.y;
                const float alpha = canopyAlpha(std::sqrt(dx * dx + dy * dy));
                if (alpha < 1.f) {
                    TilePalette faded = palette;
                    faded.canopy = withAlpha(palette.canopy, alpha);
                    faded.canopyDetail = withAlpha(palette.canopyDetail, alpha);
                    drawTile(engine, tileRole(gid), cell, faded, x, y, blocked);
                    continue;
                }
            }
            drawTile(engine, tileRole(gid), cell, palette, x, y, blocked);
        }
    }
}

void WorldView::drawBelow(engine::Engine& engine, const core::TileMap& map, const Viewport& vp) {
    if (assets_.below != engine::kInvalidTexture) {
        drawBaked(engine, assets_.below, vp, vp.x0, vp.y0, vp.x1, vp.y1);
        return;
    }
    // 旧图元画法：按 48px 一格画三层（施工图 1.2「缺图不崩」——旧画法本来就是完整实现）。
    drawTileLayer(engine, map, map.ground, vp, false);
    drawTileLayer(engine, map, map.overlay, vp, false);
    drawTileLayer(engine, map, map.building, vp, false);
}

void WorldView::drawAbove(engine::Engine& engine, const core::TileMap& map, const Viewport& vp) {
    if (assets_.above == engine::kInvalidTexture) {
        drawTileLayer(engine, map, map.front, vp, true);
        return;
    }
    // 主角周围的窗口逐顶点算透明度，窗口外整块照常贴。窗口边上的透明度恒为 1
    // （窗口比渐隐半径大一圈），两边接缝处看不出来。
    const Vec2f c = canopyCenter(vp.player);
    const int wx0 = std::clamp(static_cast<int>(std::floor(c.x - kCanopyWindowCells)), vp.x0, vp.x1);
    const int wx1 = std::clamp(static_cast<int>(std::ceil(c.x + kCanopyWindowCells)), vp.x0, vp.x1);
    const int wy0 = std::clamp(static_cast<int>(std::floor(c.y - kCanopyWindowCells)), vp.y0, vp.y1);
    const int wy1 = std::clamp(static_cast<int>(std::ceil(c.y + kCanopyWindowCells)), vp.y0, vp.y1);
    drawBaked(engine, assets_.above, vp, vp.x0, vp.y0, vp.x1, wy0);   // 上
    drawBaked(engine, assets_.above, vp, vp.x0, wy1, vp.x1, vp.y1);   // 下
    drawBaked(engine, assets_.above, vp, vp.x0, wy0, wx0, wy1);       // 左
    drawBaked(engine, assets_.above, vp, wx1, wy0, vp.x1, wy1);       // 右
    if (wx1 <= wx0 || wy1 <= wy0 || assets_.aboveSize.x <= 0 || assets_.aboveSize.y <= 0) return;

    const float invW = 1.f / fl(assets_.aboveSize.x);
    const float invH = 1.f / fl(assets_.aboveSize.y);
    const int cols = wx1 - wx0 + 1;
    const int base = geometry_.vertexCount();
    for (int j = wy0; j <= wy1; ++j) {
        for (int i = wx0; i <= wx1; ++i) {
            const float dx = fl(i) - c.x;
            const float dy = fl(j) - c.y;
            const float alpha = canopyAlpha(std::sqrt(dx * dx + dy * dy));
            geometry_.vertex(Vertex{fl(i) * kCell - vp.camX, fl(j) * kCell - vp.camY,
                                    withAlpha(kWhite, alpha), fl(i) * kArtCell * invW,
                                    fl(j) * kArtCell * invH});
        }
    }
    for (int j = 0; j < wy1 - wy0; ++j) {
        for (int i = 0; i < wx1 - wx0; ++i) {
            const int v0 = base + j * cols + i;
            for (const int v : {v0, v0 + 1, v0 + cols + 1, v0, v0 + cols + 1, v0 + cols}) geometry_.index(v);
        }
    }
    geometry_.draw(engine, BlendMode::Alpha, assets_.above);
}

void WorldView::drawWaterGlints(engine::Engine& engine, const core::TileMap& map, const Viewport& vp) {
    if (assets_.water.empty()) return;
    for (int y = vp.y0; y < vp.y1; ++y) {
        for (int x = vp.x0; x < vp.x1; ++x) {
            const auto index = static_cast<std::size_t>(y) * static_cast<std::size_t>(map.width) +
                               static_cast<std::size_t>(x);
            if (assets_.water[index] == 0) continue;
            const std::uint32_t h = cellHash(x, y);
            // 每格两粒：各自一个相位与快慢，只在正弦波顶上那一小段亮——一闪就灭，不是常亮的点。
            for (int k = 0; k < 2; ++k) {
                const std::uint32_t hk = h >> (k * 11);
                const float phase = static_cast<float>(hk & 63u) / 64.f * kTwoPi;
                const float speed = 0.8f + static_cast<float>((hk >> 6) & 7u) * 0.15f;
                const float s = std::sin(clock_ * speed + phase);
                if (s < 0.8f) continue;
                const float a = (s - 0.8f) / 0.2f;
                // 落在美术像素的格子上（3 屏幕像素一格），与烘焙图的像素对齐。
                const float gx = fl(x) * kCell + kPixel * fl(1 + static_cast<int>((hk >> 9) % 12u)) - vp.camX;
                const float gy = fl(y) * kCell + kPixel * fl(1 + static_cast<int>((hk >> 13) % 13u)) - vp.camY;
                engine.fillRect(RectF{gx - kPixel, gy, kPixel * 3.f, kPixel}, withAlpha(kGlint, a * 0.45f),
                                BlendMode::Add);
                engine.fillRect(RectF{gx, gy, kPixel, kPixel}, withAlpha(kGlint, a * 0.9f), BlendMode::Add);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 物件
// ---------------------------------------------------------------------------

void WorldView::drawObjectSprite(engine::Engine& engine, engine::TextureId texture,
                                 const ObjectSprite& sprite, float footX, float footY, float seconds,
                                 const Color& tint, BlendMode blend) {
    const RectF src = objectFrameRect(sprite, objectFrame(sprite, seconds));
    const RectF dst{snap(footX - sprite.anchorX * kPixel), snap(footY - sprite.anchorY * kPixel),
                    src.w * kPixel, src.h * kPixel};
    engine::DrawOptions options;
    options.tint = tint;
    options.blend = blend;
    engine.drawTexture(texture, src, dst, options);
}

void WorldView::drawPortals(engine::Engine& engine, const core::TileMap& map, const WorldFrame& frame,
                            const Viewport& vp, bool emissivePass) {
    for (const WorldFrame::Portal& portal : frame.portals) {
        const core::MapObject& object = *portal.object;
        const int w = std::max(1, object.width);
        const int h = std::max(1, object.height);
        const float left = fl(object.position.x) * kCell - vp.camX;
        const float top = fl(object.position.y) * kCell - vp.camY;
        // 门光比门格高出一格（往门洞里渐淡的那一截），下边的裁剪多留一格。
        if (left + fl(w) * kCell <= 0.f || top + fl(h) * kCell <= 0.f || left >= kViewW ||
            top - kCell >= kViewH) {
            continue;
        }
        // 关着的门不发光：辉光只给「现在走得过去」的路。
        if (emissivePass && !portal.open) continue;

        const int facing = portalArrowFacing(map, object);
        const ObjectSprite* arrow = objectSprite(arrowSprite(facing));
        if (arrow == nullptr) {
            if (emissivePass) continue;
            // 旧画法：底板 + 描边 + 一个字。闸门开关与「通往目标」照样看得出来。
            const engine::Rect rect{static_cast<int>(left), static_cast<int>(top), w * kWorldTilePx,
                                    h * kWorldTilePx};
            const Color edge =
                portal.guide ? kObjectiveColor : (portal.open ? kPortalEdge : kPortalLockedEdge);
            engine.drawRect(inset(rect, 2), kPortalFill, true);
            engine.drawRect(inset(rect, 2), edge, false);
            if (portal.guide) engine.drawRect(inset(rect, 3), edge, false);
            drawGlyph(engine, portal.open ? "门" : "闭", rect, kGlyphFontSize, edge);
            continue;
        }
        const Color tint = portal.guide ? kPortalGuide : (portal.open ? kPortalOpen : kPortalLocked);
        const ObjectSprite* door = portal.open && portalIsDoorway(map, object) ? objectSprite("portal.door") : nullptr;
        for (int cy = 0; cy < h; ++cy) {
            for (int cx = 0; cx < w; ++cx) {
                const float footX = left + (fl(cx) + 0.5f) * kCell;
                const float footY = top + fl(cy + 1) * kCell;
                drawObjectSprite(engine, objectsTexture_, *arrow, footX, footY, clock_, tint, BlendMode::Add);
                if (door != nullptr) {
                    drawObjectSprite(engine, objectsTexture_, *door, footX, footY, clock_, tint, BlendMode::Add);
                }
            }
        }
    }
}

void WorldView::drawHotspots(engine::Engine& engine, const WorldFrame& frame, const Viewport& vp,
                             bool emissivePass) {
    const ObjectSprite* sparkle = objectSprite("mark.sparkle");
    for (const core::MapObject* object : frame.hotspots) {
        const int w = std::max(1, object->width);
        const int h = std::max(1, object->height);
        const float left = fl(object->position.x) * kCell - vp.camX;
        const float top = fl(object->position.y) * kCell - vp.camY;
        if (left + fl(w) * kCell <= 0.f || top + fl(h) * kCell <= 0.f || left >= kViewW || top >= kViewH) continue;
        if (sparkle == nullptr || sparkle->fps <= 0.f) {
            if (emissivePass) continue;
            // 旧画法：一圈很淡的框 + 右上角一个亮十字。
            const engine::Rect rect{static_cast<int>(left), static_cast<int>(top), w * kWorldTilePx,
                                    h * kWorldTilePx};
            engine.drawRect(rect, kHintEdge, false);
            const int markX = rect.x + rect.w - 13;
            const int markY = rect.y + 5;
            engine.drawRect(engine::Rect{markX + 3, markY, 3, 9}, kHintMark, true);
            engine.drawRect(engine::Rect{markX, markY + 3, 9, 3}, kHintMark, true);
            continue;
        }
        // 各闪各的：相位由对象名杂凑出来，一张图上八个交互点不会齐刷刷地一起闪。
        const float phase = static_cast<float>(textHash(object->name) % 1000u) / 1000.f * kSparkleCycle;
        const float t = std::fmod(clock_ + phase, kSparkleCycle);
        if (t >= static_cast<float>(sparkle->frames.size()) / sparkle->fps) continue;
        // 闪光的锚点是帧底边中点；要它落在触发区正中，就把「脚」放在中心往下半格。
        drawObjectSprite(engine, objectsTexture_, *sparkle, left + fl(w) * kCell / 2.f,
                         top + fl(h) * kCell / 2.f + kCell / 2.f, t, kSparkle, BlendMode::Add);
    }
}

void WorldView::drawObjectiveRing(engine::Engine& engine, const WorldFrame& frame, const Viewport& vp) {
    const core::MapObject* target = frame.objectiveTarget;
    if (target == nullptr) return;
    const float w = fl(std::max(1, target->width)) * kCell;
    const float h = fl(std::max(1, target->height)) * kCell;
    const float left = fl(target->position.x) * kCell - vp.camX;
    const float top = fl(target->position.y) * kCell - vp.camY;
    if (left + w <= 0.f || top + h <= 0.f || left >= kViewW || top >= kViewH) return;
    // 地上一圈金色的光：触发区本身是不画的，只有头顶一个菱形时玩家知道「在这一带」，
    // 却不知道该踩哪一格——这一圈就是那一格。人站在目标格上时它落在人的脚下。
    const float pulse = 0.55f + 0.25f * std::sin(clock_ * 3.2f);
    const bool npc = target->type == "npc";
    const float cy = npc ? top + h - 8.f : top + h / 2.f;
    geometry_.fan(left + w / 2.f, cy, w * 0.52f, std::min(h, kCell) * 0.34f, withAlpha(kObjectiveGlow, pulse * 0.55f),
                  withAlpha(kObjectiveGlow, 0.f), 28);
    geometry_.draw(engine, BlendMode::Add);
}

void WorldView::drawShadows(engine::Engine& engine, const WorldFrame& frame, const Viewport& vp) {
    const auto shadowAt = [&](float footX, float footY) {
        if (footX < -kCell || footX > kViewW + kCell || footY < -kCell || footY > kViewH + kCell) return;
        if (shadowTexture_ != engine::kInvalidTexture) {
            // shadow.png 是白色的 alpha 遮罩，按 fx/index.json 的写法「乘黑色画」（tint #000000）。
            engine::DrawOptions options;
            options.tint = Color{0, 0, 0, 230};
            engine.drawTexture(shadowTexture_, RectF{}, RectF{snap(footX - 25.f), snap(footY - 10.f), 50.f, 16.f},
                               options);
            return;
        }
        geometry_.fan(footX, footY - 3.f, 22.f, 7.f, Color{0, 0, 0, 110}, Color{0, 0, 0, 0});
    };
    for (const WorldFrame::Npc& npc : frame.npcs) {
        shadowAt((fl(npc.object->position.x) + 0.5f) * kCell - vp.camX,
                 fl(npc.object->position.y + 1) * kCell - vp.camY);
    }
    shadowAt((vp.player.x + 0.5f) * kCell - vp.camX, (vp.player.y + 1.f) * kCell - vp.camY);
    geometry_.draw(engine, BlendMode::Alpha);
}

void WorldView::drawCharacter(engine::Engine& engine, const std::string& roleId, bool player, float footX,
                              float footY, int facing, bool moving, const Viewport& vp) {
    const CharacterSheet* sheet = atlasOk_ ? sheetForRole(atlas_, roleId, chapterDoneOf(vp)) : nullptr;
    const engine::TextureId texture =
        sheet != nullptr ? sheetTexture(engine, *sheet) : engine::kInvalidTexture;
    if (sheet != nullptr && texture != engine::kInvalidTexture) {
        const int frame = walkFrame(atlas_, *sheet, facing, moving, walkClock_);
        const RectF src = sheetFrameRect(*sheet, frame);
        const float w = fl(sheet->frameW) * kPixel;
        const float h = fl(sheet->frameH) * kPixel;
        // 帧内 (frameW/2, footY+1) 对准脚底点（docs/art-sprites.md 第 4 节）。
        const RectF dst{snap(footX - w / 2.f), snap(footY - fl(sheet->footY + 1) * kPixel), w, h};
        engine.drawTexture(texture, src, dst, engine::DrawOptions{});
        return;
    }
    if (atlasOk_ && sheet == nullptr) {
        warnOnce("look:" + roleId, "角色 " + roleId + " 在 art/sprites/index.json 里没有外观：退回旧图元画法");
    }
    const engine::Rect cell{static_cast<int>(snap(footX - kCell / 2.f)), static_cast<int>(snap(footY - kCell)),
                            kWorldTilePx, kWorldTilePx};
    drawFigure(engine, cell, player ? playerStyle() : npcStyle(roleId), facing);
}

void WorldView::drawFacility(engine::Engine& engine, const WorldFrame::Facility& facility,
                             const Viewport& vp, bool emissivePass) {
    if (!facilitySpriteAtRuntime(facility.kind, assets_.below != engine::kInvalidTexture)) return;
    const core::MapObject& object = *facility.object;
    const int w = std::max(1, object.width);
    const int h = std::max(1, object.height);
    const float footX = (fl(object.position.x) + fl(w) / 2.f) * kCell - vp.camX;
    const float footY = fl(object.position.y + h) * kCell - vp.camY;
    // 精灵可能比对象矩形高出一两格（丹炉、告示板），裁剪留足余量。
    if (footX < -kCell * 2.f || footX > kViewW + kCell * 2.f || footY < -kCell || footY > kViewH + kCell * 3.f) return;

    const ObjectSprite* sprite = nullptr;
    if (const auto it = atlas_.facilities.find(facility.kind); it != atlas_.facilities.end()) {
        sprite = objectSprite(it->second);
    }
    if (sprite != nullptr) {
        if (emissivePass) {
            // 同位置的发光图：炉火、阵纹、香头那几个像素，进辉光通道化成光晕。
            if (emissiveTexture_ != engine::kInvalidTexture) {
                drawObjectSprite(engine, emissiveTexture_, *sprite, footX, footY, clock_, kWhite, BlendMode::Add);
            }
            return;
        }
        drawObjectSprite(engine, objectsTexture_, *sprite, footX, footY, clock_, kWhite,
                         sprite->additive ? BlendMode::Add : BlendMode::Alpha);
        return;
    }
    if (emissivePass) return;
    if (objectsTexture_ != engine::kInvalidTexture) {
        warnOnce("facility:" + facility.kind,
                 "设施 kind=" + facility.kind + " 在 index.json 的 facilities 里没有物件：退回旧画法");
    }
    // 旧画法：底板配色 + 一个字，两样都来自 kind（MapArt 的 facilityColor / facilityGlyph）。
    const engine::Rect rect{static_cast<int>(fl(object.position.x) * kCell - vp.camX),
                            static_cast<int>(fl(object.position.y) * kCell - vp.camY), w * kWorldTilePx,
                            h * kWorldTilePx};
    const Color plate = facilityColor(facility.kind);
    engine.drawRect(inset(rect, 2), plate, true);
    engine.drawRect(inset(rect, 2), darker(plate), false);
    drawGlyph(engine, facilityGlyph(facility.kind), rect, kGlyphFontSize, kGlyphDark);
}

void WorldView::drawSorted(engine::Engine& engine, Application& app, const WorldFrame& frame,
                           const Viewport& vp) {
    drawables_.clear();
    int order = 0;
    for (std::size_t i = 0; i < frame.facilities.size(); ++i) {
        const core::MapObject& object = *frame.facilities[i].object;
        const int w = std::max(1, object.width);
        const float footX = (fl(object.position.x) + fl(w) / 2.f) * kCell;
        const float footY = fl(object.position.y + std::max(1, object.height)) * kCell;
        drawables_.push_back(Drawable{DepthKey{footY, footX, order++}, Drawable::Kind::Facility, i});
    }
    for (std::size_t i = 0; i < frame.npcs.size(); ++i) {
        const core::MapObject& object = *frame.npcs[i].object;
        const float footX = (fl(object.position.x) + 0.5f) * kCell;
        const float footY = fl(object.position.y + 1) * kCell;
        if (footX - vp.camX < -kCell * 2.f || footX - vp.camX > kViewW + kCell * 2.f ||
            footY - vp.camY < -kCell || footY - vp.camY > kViewH + kCell * 2.f) {
            continue;
        }
        drawables_.push_back(Drawable{DepthKey{footY, footX, order++}, Drawable::Kind::Npc, i});
    }
    drawables_.push_back(Drawable{DepthKey{(vp.player.y + 1.f) * kCell, (vp.player.x + 0.5f) * kCell, order++},
                                  Drawable::Kind::Player, 0});
    std::sort(drawables_.begin(), drawables_.end(),
              [](const Drawable& a, const Drawable& b) { return drawsBefore(a.key, b.key); });

    for (const Drawable& item : drawables_) {
        switch (item.kind) {
            case Drawable::Kind::Facility:
                drawFacility(engine, frame.facilities[item.index], vp, false);
                break;
            case Drawable::Kind::Npc: {
                const WorldFrame::Npc& npc = frame.npcs[item.index];
                drawCharacter(engine, npc.roleId, false, item.key.x - vp.camX, item.key.footY - vp.camY,
                              npc.facing, false, vp);
                break;
            }
            case Drawable::Kind::Player:
                drawCharacter(engine, kPlayerRole, true, item.key.x - vp.camX, item.key.footY - vp.camY,
                              app.state().facing, walking_, vp);
                break;
        }
    }
}

void WorldView::drawParticles(engine::Engine& engine, const Viewport& vp, bool emissivePass) {
    for (ParticleSlot& slot : assets_.particles) {
        if (emissivePass && !slot.emissive) continue;
        if (slot.world) {
            const RectF& r = slot.reach;
            if (r.x + r.w < vp.camX || r.y + r.h < vp.camY || r.x > vp.camX + kViewW || r.y > vp.camY + kViewH) {
                continue;
            }
        }
        slot.system.render(engine, vp.camX, vp.camY);
    }
}

void WorldView::drawLightGlows(engine::Engine& engine, const Viewport& vp) {
    if (glowTexture_ == engine::kInvalidTexture) return;
    for (const MapLight& light : assets_.visual.lights) {
        const float sx = light.x * kCell - vp.camX;
        const float sy = light.y * kCell - vp.camY;
        const float size = glowCells(light.kind) * kCell;
        if (sx < -size || sy < -size || sx > kViewW + size || sy > kViewH + size) continue;
        const float strength = light.intensity * flickerFactor(light.flicker, clock_, lightPhase(light));
        engine::DrawOptions options;
        options.tint = withAlpha(Color{light.color.r, light.color.g, light.color.b, 255},
                                 std::clamp(strength * 0.85f, 0.f, 1.f));
        options.blend = BlendMode::Add;
        engine.drawTexture(glowTexture_, RectF{}, RectF{sx - size / 2.f, sy - size / 2.f, size, size}, options);
    }
}

void WorldView::addLights(const Viewport& vp) {
    for (const MapLight& light : assets_.visual.lights) {
        const float sx = light.x * kCell - vp.camX;
        const float sy = light.y * kCell - vp.camY;
        const float radius = light.radius * kCell;
        if (sx < -radius || sy < -radius || sx > kViewW + radius || sy > kViewH + radius) continue;
        fx_->addLight(engine::Light{sx, sy, radius, light.color,
                                    light.intensity * flickerFactor(light.flicker, clock_, lightPhase(light))});
    }
    const CarriedLight carried = carriedLightFor(assets_.visual.time);
    if (carried.on) {
        // 随行微光挂在胸口高度：精灵站在格底边，身子在格中偏上。
        fx_->addLight(engine::Light{(vp.player.x + 0.5f) * kCell - vp.camX,
                                    (vp.player.y + 0.5f) * kCell - vp.camY - 12.f,
                                    carried.radiusCells * kCell, carried.color, carried.intensity});
    }
}

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------

float WorldView::headTop(const std::string& roleId, float footY, const Viewport& vp) const {
    if (atlasOk_) {
        if (const CharacterSheet* sheet = sheetForRole(atlas_, roleId, chapterDoneOf(vp))) {
            const auto it = sheetTextures_.find(sheet->file);
            if (it != sheetTextures_.end() && it->second != engine::kInvalidTexture) {
                return footY - fl(sheet->footY + 1 - sheet->headY) * kPixel;
            }
        }
    }
    return footY - kCell;   // 旧画法的人一格高
}

void WorldView::drawNamePlates(engine::Engine& engine, const ui::Theme& theme, const WorldFrame& frame,
                               const Viewport& vp) {
    plates_.clear();
    for (const WorldFrame::Npc& npc : frame.npcs) {
        const core::MapObject& object = *npc.object;
        const float footX = (fl(object.position.x) + 0.5f) * kCell - vp.camX;
        const float footY = fl(object.position.y + 1) * kCell - vp.camY;
        if (footX < -kCell * 2.f || footX > kViewW + kCell * 2.f || footY < -kCell || footY > kViewH + kCell * 3.f) {
            continue;
        }
        // 名牌压在一切遮挡之上：树冠可以盖住人，不能盖住他是谁。
        // 并排站着的人（墨府门口那四个护院）名牌会撞在一起读成一串：撞上了就往上叠一行。
        float plateBottom = headTop(npc.roleId, footY, vp) - 4.f;
        engine::RectF plate = hud::namePlateRect(engine, npc.name, footX, plateBottom);
        for (bool moved = true; moved && plate.w > 0.f;) {
            moved = false;
            for (const engine::RectF& other : plates_) {
                const bool overlap = plate.x < other.x + other.w && other.x < plate.x + plate.w &&
                                     plate.y < other.y + other.h && other.y < plate.y + plate.h;
                if (!overlap) continue;
                plateBottom = other.y - 2.f;
                plate = hud::namePlateRect(engine, npc.name, footX, plateBottom);
                moved = true;
            }
        }
        plates_.push_back(plate);
        hud::drawNamePlate(engine, theme, npc.name, footX, plateBottom);
        if (npc.bubble == PathBubble::None) continue;
        const char* id = pathBubbleSprite(npc.bubble);
        const ObjectSprite* bubble = id != nullptr ? objectSprite(id) : nullptr;
        if (bubble == nullptr) continue;
        // 气泡的锚点是尾尖：尖对着名牌上沿，身子往右上方浮；上下轻轻漂一个美术像素。
        const float bob = std::round(std::sin(clock_ * 2.4f + fl(object.position.x))) * kPixel;
        drawObjectSprite(engine, objectsTexture_, *bubble, footX, plateBottom - 22.f + bob, 0.f, kWhite,
                         BlendMode::Alpha);
    }
}

void WorldView::drawObjectiveMarker(engine::Engine& engine, const ui::Theme& theme, const WorldFrame& frame,
                                    const Viewport& vp) {
    const core::MapObject* target = frame.objectiveTarget;
    if (target == nullptr) return;
    const float w = fl(std::max(1, target->width)) * kCell;
    const float h = fl(std::max(1, target->height)) * kCell;
    const float left = fl(target->position.x) * kCell - vp.camX;
    const float top = fl(target->position.y) * kCell - vp.camY;

    if (!(left + w <= 0.f || top + h <= 0.f || left >= kViewW || top >= kViewH)) {
        // 在视野里：一枚金菱形悬在目标头顶。目标是 NPC 时抬到名牌上面去——画在名牌那个高度上，
        // 正好戳在名字中间，两样都读不清。
        float anchor = top;
        for (const WorldFrame::Npc& npc : frame.npcs) {
            if (npc.object == target) {
                anchor = headTop(npc.roleId, top + kCell, vp) - 26.f;
                break;
            }
        }
        const float bob = std::sin(clock_ * 3.f) * 3.f;
        hud::drawObjectiveDiamond(engine, theme, geometry_, glowTexture_, left + w / 2.f,
                                  snap(anchor - 18.f + bob));
        return;
    }

    // 出了视野：箭头钉在屏幕边上，方向是从主角指向目标，并写出还有多少格。
    // 距离必须写出来：只给方向，照着走三十步还没到的玩家会开始怀疑方向是不是错的。
    const float sx = (vp.player.x + 0.5f) * kCell - vp.camX;
    const float sy = (vp.player.y + 0.5f) * kCell - vp.camY;
    const float dx = left + w / 2.f - sx;
    const float dy = top + h / 2.f - sy;
    constexpr float kMargin = 34.f;
    float t = 1e9f;
    if (dx > 0.f) t = std::min(t, (kViewW - kMargin - sx) / dx);
    if (dx < 0.f) t = std::min(t, (kMargin - sx) / dx);
    if (dy > 0.f) t = std::min(t, (kViewH - kMargin - sy) / dy);
    if (dy < 0.f) t = std::min(t, (kMargin - sy) / dy);
    if (t >= 1e9f) return;
    t = std::max(t, 0.f);
    const float targetX = fl(target->position.x) + fl(std::max(1, target->width)) / 2.f;
    const float targetY = fl(target->position.y) + fl(std::max(1, target->height)) / 2.f;
    const int cells = static_cast<int>(std::abs(targetX - (fl(lastCell_.x) + 0.5f)) +
                                       std::abs(targetY - (fl(lastCell_.y) + 0.5f)));
    hud::drawEdgeArrow(engine, theme, geometry_, glowTexture_, sx + dx * t, sy + dy * t, dx, dy,
                       std::to_string(cells) + " 格");
}

void WorldView::drawHud(engine::Engine& engine, const ui::Theme& theme, const WorldFrame& frame,
                        const Viewport& vp) {
    // 镜头在地图左上角夹住时，主角就站在横幅与目标框底下。这两样盖住他的时候淡成半透明：
    // 「现在去哪」可以读得淡一点，「我在哪」不能看不见。
    const float footX = (vp.player.x + 0.5f) * kCell - vp.camX;
    const float footY = (vp.player.y + 1.f) * kCell - vp.camY;
    const RectF hero{footX - kCell / 2.f, footY - kCell * 1.5f, kCell, kCell * 1.5f};
    const auto covers = [&hero](const RectF& r) {
        return r.w > 0.f && hero.x < r.x + r.w && r.x < hero.x + hero.w && hero.y < r.y + r.h &&
               r.y < hero.y + hero.h;
    };
    constexpr float kCoveringAlpha = 0.35f;

    banner_.set(frame.mapTitle, frame.dangerStars);
    const float banner = bannerAlpha(bannerClock_, bannerAfterFade_);
    banner_.draw(engine, theme, geometry_, covers(banner_.area()) ? banner * kCoveringAlpha : banner);
    // 目标框排在横幅下面，横幅淡出后也不往上挪：会自己换位置的框，玩家每次都得重新找。
    const float boxX = 16.f;
    const float boxY = hud::PlaceBanner::bottom();
    const RectF box = hud::objectiveBoxRect(engine, boxX, boxY, frame.objectiveText, frame.objectiveWhere);
    hud::drawObjectiveBox(engine, theme, geometry_, boxX, boxY, frame.objectiveText, frame.objectiveWhere,
                          covers(box) ? kCoveringAlpha : 1.f);
    hud::drawSaveNotice(engine, theme, geometry_, frame.saveNotice);
}

}  // namespace fanren::game
