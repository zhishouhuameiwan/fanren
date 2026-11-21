#pragma once
// 小地图与区域地图共用的那一层（第 7 章可玩性改版·第一期「地基包」路 M，docs/ch07-foundation.md 3.1、3.2）。
//
// 2026-11-20 用户试玩第 7 章：「各种信息指引不够」。右上角一块约 200×150 的小地图、M 键打开的区域地图，回答的是
// 「我在哪、门在哪、目标在哪、哪儿还有东西没拿」。
//
// 分三层：
//   · 纯函数（不开窗口可测）：换算（格 → 小地图上的点）、取景（整张图放得下就整张，放不下就跟着主角开一扇窗、
//     贴着图边夹住）、标记（从 WorldScene 判好的那一帧 WorldFrame 里挑出要画的东西，按 3.1 的规矩筛）；
//   · MapThumbnail：底图。有高清烘焙图（art/maps_hd/<图>/below.png）的，用渲染目标逐级减半缩成 12 / 6 / 3 像素一格
//     （一次缩一半、线性采样，就是 2×2 取平均，缩得再小也不起摩尔纹）；没有的，用通行格画一张一格一像素的剪影。
//     换图才重建。无头时建不出渲染目标，什么也不画；
//   · 画标记（drawMarks）：小地图与区域地图同一套形状和颜色，图例只有一份说法。
//
// **判据一个都不在这里另写**：门开没开、宝箱开没开、明雷在不在、哪道门通往目标、目标在哪，全是 WorldFrame 里
// WorldScene 判好的（与画面、走位同一个答案）。这里只多两条显示规矩：没开的宝箱、没采的采集点、明雷只在主角
// kRevealCells 格以内才画（3.1：开过的宝箱、采过的采集点一直画成灰的）。
// 不做战争迷雾、不记「去过哪儿」，所以不加任何存档字段。
#include <cstdint>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "engine/Engine.h"
#include "game/WorldView.h"

namespace fanren::game {

namespace hud {
class Geometry;
}

namespace minimap {

// 右上角那一块（逻辑像素）。存盘提示挪到它下面（kSaveNoticeTop），得物卡片从 172 起往下排（game/ItemToasts.cpp）。
inline constexpr float kPanelW = 200.f;
inline constexpr float kPanelH = 150.f;
inline constexpr float kRight = 16.f;      // 离屏幕右边
inline constexpr float kTop = 12.f;        // 离屏幕上边
inline constexpr float kInset = 5.f;       // 面板外框到图的边距
inline constexpr float kSaveNoticeTop = kTop + kPanelH + 6.f;
// 一格至少画这么大（像素）。整张图缩到放得下要比这还小时，改成跟着主角开窗（3.1：目标不在范围里时贴边）。
inline constexpr float kMinCellPx = 3.f;
// 没开的宝箱、没采的采集点、明雷：主角这么多格以内才画（3.1）。按切比雪夫距离（横竖斜都算一步）。
inline constexpr int kRevealCells = 8;
// 盖住主角时淡成这样（与目标框同一个数，WorldView::drawHud）。
inline constexpr float kCoveringAlpha = 0.35f;

// 小地图面板的矩形；图画在它里面缩进 kInset 的那一块（mapArea）。
[[nodiscard]] engine::RectF panelRect();
[[nodiscard]] engine::RectF mapArea(const engine::RectF& panel);

// 取景。屏幕上的点 = (area.x + offsetX + 格 × cellPx, area.y + offsetY + 格 × cellPx)。
struct Layout {
    engine::RectF area{};
    float cellPx = 1.f;
    float offsetX = 0.f;
    float offsetY = 0.f;
    int mapW = 0;
    int mapH = 0;
    bool whole = true;   // 整张图都在里面（没开窗）
};

// 整张图缩到放得下（cellPx = 两向较小的那个比例，至多 maxCellPx）且不小于 minCellPx 时：整张、居中。
// 否则 cellPx = minCellPx，focus（格坐标，可带小数，通常是主角）落在正中，再夹住不越过图边；
// 某一向图比窗还窄时那一向照样居中。mapW / mapH ≤ 0 时 cellPx 为 1、什么也不偏。
[[nodiscard]] Layout fit(const engine::RectF& area, int mapW, int mapH, Vec2f focus, float minCellPx = kMinCellPx,
                         float maxCellPx = 1e9f);

// 格坐标（可带小数；一格的中心是 x + 0.5）→ 屏幕上的点。
[[nodiscard]] Vec2f toScreen(const Layout& layout, Vec2f cell);
// 反过来：屏幕上的点落在哪一格（向下取整）。toCell(toScreen(c)) 落回 c 所在的那一格。
[[nodiscard]] core::Point toCell(const Layout& layout, Vec2f point);
// 一个点在不在图那一块里（margin > 0 往里缩）。
[[nodiscard]] bool contains(const Layout& layout, Vec2f point, float margin = 0.f);
// 贴边：夹进图那一块里、离边 inset。点本来就在里面时原样返回。
[[nodiscard]] Vec2f clampInside(const Layout& layout, Vec2f point, float inset);
// 画面上看得见的格子范围 [x0, x1) × [y0, y1)（夹在图里）。
struct CellRange {
    float x0 = 0.f, y0 = 0.f, x1 = 0.f, y1 = 0.f;
};
[[nodiscard]] CellRange visibleCells(const Layout& layout);
// 平移（区域地图的方向键）：窗往 (dx, dy) 格挪，夹住不越过图边；whole 时不动。
[[nodiscard]] Layout panned(const Layout& layout, float dx, float dy);

// 一个对象占的那几格的中心（格坐标）：矩形左上 + (宽, 高) / 2。
[[nodiscard]] Vec2f objectCenter(const core::MapObject& object);
// 两点之间的切比雪夫距离（格）。
[[nodiscard]] float chebyshev(Vec2f a, Vec2f b);

enum class MarkKind {
    Door,          // 开着的门
    DoorLocked,    // 关着的门（require_flag 没置）
    DoorGuide,     // 通往主线目标的那一道
    Save,          // 存档点
    Facility,      // 灵田、打坐、炼丹、制符、炼器、商店、告示、历练入口
    Chest,         // 没开的宝箱（主角 kRevealCells 格以内才有）
    ChestOpened,   // 开过的（一直画，灰）
    Gather,        // 没采过的采集点（同上，近才有）
    GatherDone,    // 采过的（一直画，灰）
    Foe,           // 明雷（近才有）
    Objective,     // 主线目标（目标在别的图上时就是那道通往它的门）
    Hero,          // 主角（带朝向）
};

struct Mark {
    MarkKind kind = MarkKind::Door;
    Vec2f cell{};            // 中心（格坐标）
    int facing = 2;          // 主角的朝向（同 GameState::facing）
    std::string facility;    // Facility 的 kind（区域地图的图例按它分）
};

// 从这一帧里挑要画的标记（画的次序 = 返回的次序：门、设施、采集、宝箱、明雷、目标、主角）。
// objects：此刻用的那一份对象状态（采集点「采过」看 gatheredDay 里有没有它，与收集进度同一个口径）。
// hero：主角此刻的格（可带小数：滑步中）。
[[nodiscard]] std::vector<Mark> collectMarks(const WorldFrame& frame, const core::WorldObjectState& objects,
                                             Vec2f hero, int heroFacing, int revealCells = kRevealCells);

// 这一道门通到哪张图（target_map 属性）。
[[nodiscard]] std::string portalTarget(const core::MapObject& portal);

// ---- 底图 ----
// 剪影的像素（一格一像素，0xRRGGBBAA）：走得通的旧纸色、走不通的墨色。没有高清烘焙图的图用它。
inline constexpr std::uint32_t kSilhouetteWalkable = 0xC8BC98E6u;
inline constexpr std::uint32_t kSilhouetteBlocked = 0x262A32D2u;
[[nodiscard]] std::vector<std::uint32_t> silhouettePixels(const core::TileMap& map);

class MapThumbnail {
public:
    MapThumbnail() = default;
    ~MapThumbnail() = default;
    MapThumbnail(const MapThumbnail&) = delete;
    MapThumbnail& operator=(const MapThumbnail&) = delete;

    // 换了图才重建。无头时什么也不建。
    void ensure(engine::Engine& engine, const core::TileMap& map);
    // 画看得见的那一块。alpha 整体淡（盖住主角时）。
    void draw(engine::Engine& engine, const Layout& layout, float alpha);
    // 用的是高清烘焙图（否则是剪影）。
    [[nodiscard]] bool hd() const { return hd_; }
    [[nodiscard]] const std::string& mapId() const { return mapId_; }

private:
    struct Level {
        engine::OwnedTexture texture;
        float pxPerCell = 1.f;
    };
    std::string mapId_;
    std::string hdPath_;          // 高清烘焙图的路径（区域地图放大时直接画它）
    bool hd_ = false;
    std::vector<Level> levels_;   // 由大到小
};

// 画一批标记。size 是标记的基准尺寸（像素）：小地图约 4，区域地图约 8。
void drawMarks(engine::Engine& engine, const ui::Theme& theme, hud::Geometry& geometry, const Layout& layout,
               const std::vector<Mark>& marks, float size, float alpha);
// 画一个标记（图例用）：在 (x, y) 画 kind 那个形状。
void drawMarkShape(engine::Engine& engine, const ui::Theme& theme, hud::Geometry& geometry, MarkKind kind, float x,
                   float y, float size, float alpha, int facing = 2);

}  // namespace minimap

}  // namespace fanren::game
