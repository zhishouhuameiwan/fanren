#include "game/Minimap.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "game/WorldHud.h"
#include "ui/Widgets.h"

namespace fanren::game::minimap {

namespace {

using engine::BlendMode;
using engine::Color;
using engine::RectF;

constexpr float kViewW = static_cast<float>(engine::kLogicalWidth);


// 高清烘焙图缩成这几档（像素 / 格），由大到小。烘焙图本身 24 像素一格（kHdArtTilePx）。
constexpr float kHdLevels[] = {12.f, 6.f, 3.f};

Color withA(Color c, float alpha) {
    c.a = static_cast<std::uint8_t>(std::clamp(static_cast<float>(c.a) * alpha, 0.f, 255.f));
    return c;
}

engine::Vertex vtx(float x, float y, Color c) {
    engine::Vertex v;
    v.x = x;
    v.y = y;
    v.color = c;
    return v;
}

void square(hud::Geometry& g, float cx, float cy, float r, Color c) {
    g.quad(cx - r, cy - r, cx + r, cy + r, c, c);
}

}  // namespace

RectF panelRect() {
    return RectF{kViewW - kRight - kPanelW, kTop, kPanelW, kPanelH};
}

RectF mapArea(const RectF& panel) {
    return RectF{panel.x + kInset, panel.y + kInset, panel.w - kInset * 2.f, panel.h - kInset * 2.f};
}

Layout fit(const RectF& area, int mapW, int mapH, Vec2f focus, float minCellPx, float maxCellPx) {
    Layout layout;
    layout.area = area;
    layout.mapW = mapW;
    layout.mapH = mapH;
    if (mapW <= 0 || mapH <= 0 || area.w <= 0.f || area.h <= 0.f) return layout;
    const float wholePx = std::min({area.w / static_cast<float>(mapW), area.h / static_cast<float>(mapH), maxCellPx});
    const auto axis = [](float extent, float view, float focusPx) {
        // 图比窗窄：居中（偏移为正）；否则让 focus 落在正中，再夹住不越过图边。
        if (extent <= view) return (view - extent) / 2.f;
        return std::clamp(view / 2.f - focusPx, view - extent, 0.f);
    };
    if (wholePx >= minCellPx) {
        layout.cellPx = wholePx;
        layout.whole = true;
    } else {
        layout.cellPx = minCellPx;
        layout.whole = false;
    }
    const float extentW = static_cast<float>(mapW) * layout.cellPx;
    const float extentH = static_cast<float>(mapH) * layout.cellPx;
    layout.offsetX = axis(extentW, area.w, focus.x * layout.cellPx);
    layout.offsetY = axis(extentH, area.h, focus.y * layout.cellPx);
    return layout;
}

Vec2f toScreen(const Layout& layout, Vec2f cell) {
    return Vec2f{layout.area.x + layout.offsetX + cell.x * layout.cellPx,
                 layout.area.y + layout.offsetY + cell.y * layout.cellPx};
}

core::Point toCell(const Layout& layout, Vec2f point) {
    const float cx = (point.x - layout.area.x - layout.offsetX) / layout.cellPx;
    const float cy = (point.y - layout.area.y - layout.offsetY) / layout.cellPx;
    return core::Point{static_cast<int>(std::floor(cx)), static_cast<int>(std::floor(cy))};
}

std::vector<std::uint32_t> silhouettePixels(const core::TileMap& map) {
    std::vector<std::uint32_t> rgba;
    if (map.width <= 0 || map.height <= 0) return rgba;
    rgba.resize(static_cast<std::size_t>(map.width) * static_cast<std::size_t>(map.height));
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            rgba[static_cast<std::size_t>(y) * static_cast<std::size_t>(map.width) + static_cast<std::size_t>(x)] =
                map.walkable(core::Point{x, y}) ? kSilhouetteWalkable : kSilhouetteBlocked;
        }
    }
    return rgba;
}

bool contains(const Layout& layout, Vec2f point, float margin) {
    const RectF& a = layout.area;
    return point.x >= a.x + margin && point.x <= a.x + a.w - margin && point.y >= a.y + margin &&
           point.y <= a.y + a.h - margin;
}

Vec2f clampInside(const Layout& layout, Vec2f point, float inset) {
    const RectF& a = layout.area;
    return Vec2f{std::clamp(point.x, a.x + inset, std::max(a.x + inset, a.x + a.w - inset)),
                 std::clamp(point.y, a.y + inset, std::max(a.y + inset, a.y + a.h - inset))};
}

CellRange visibleCells(const Layout& layout) {
    CellRange r;
    if (layout.cellPx <= 0.f || layout.mapW <= 0 || layout.mapH <= 0) return r;
    const float w = static_cast<float>(layout.mapW);
    const float h = static_cast<float>(layout.mapH);
    r.x0 = std::clamp(-layout.offsetX / layout.cellPx, 0.f, w);
    r.y0 = std::clamp(-layout.offsetY / layout.cellPx, 0.f, h);
    r.x1 = std::clamp((layout.area.w - layout.offsetX) / layout.cellPx, 0.f, w);
    r.y1 = std::clamp((layout.area.h - layout.offsetY) / layout.cellPx, 0.f, h);
    return r;
}

Layout panned(const Layout& layout, float dx, float dy) {
    if (layout.whole) return layout;
    Layout out = layout;
    const auto axis = [](float offset, float delta, float extent, float view) {
        if (extent <= view) return offset;
        return std::clamp(offset - delta, view - extent, 0.f);
    };
    out.offsetX = axis(layout.offsetX, dx * layout.cellPx, static_cast<float>(layout.mapW) * layout.cellPx,
                       layout.area.w);
    out.offsetY = axis(layout.offsetY, dy * layout.cellPx, static_cast<float>(layout.mapH) * layout.cellPx,
                       layout.area.h);
    return out;
}

Vec2f objectCenter(const core::MapObject& object) {
    return Vec2f{static_cast<float>(object.position.x) + static_cast<float>(std::max(1, object.width)) / 2.f,
                 static_cast<float>(object.position.y) + static_cast<float>(std::max(1, object.height)) / 2.f};
}

float chebyshev(Vec2f a, Vec2f b) {
    return std::max(std::fabs(a.x - b.x), std::fabs(a.y - b.y));
}

std::string portalTarget(const core::MapObject& portal) {
    return portal.property("target_map");
}

std::vector<Mark> collectMarks(const WorldFrame& frame, const core::WorldObjectState& objects, Vec2f hero,
                               int heroFacing, int revealCells) {
    std::vector<Mark> marks;
    // 主角的中心：传进来的是他站的那一格（左上角），加半格。
    const Vec2f heroCenter{hero.x + 0.5f, hero.y + 0.5f};
    const auto near = [&](Vec2f c) { return chebyshev(c, heroCenter) <= static_cast<float>(revealCells) + 0.5f; };

    const core::MapObject* guideDoor = nullptr;
    for (const WorldFrame::Portal& portal : frame.portals) {
        if (portal.object == nullptr) continue;
        Mark m;
        m.cell = objectCenter(*portal.object);
        m.kind = portal.guide ? MarkKind::DoorGuide : (portal.open ? MarkKind::Door : MarkKind::DoorLocked);
        if (portal.guide && guideDoor == nullptr) guideDoor = portal.object;
        marks.push_back(m);
    }
    for (const WorldFrame::Facility& facility : frame.facilities) {
        if (facility.object == nullptr) continue;
        Mark m;
        m.cell = objectCenter(*facility.object);
        m.kind = facility.kind == "save" ? MarkKind::Save : MarkKind::Facility;
        m.facility = facility.kind;
        marks.push_back(m);
    }
    for (const WorldFrame::Gather& gather : frame.gathers) {
        if (gather.object == nullptr) continue;
        const Vec2f c = objectCenter(*gather.object);
        const bool gathered = objects.gatheredDay.count(gather.object->property("id")) > 0;
        if (!gathered && !near(c)) continue;
        marks.push_back(Mark{gathered ? MarkKind::GatherDone : MarkKind::Gather, c});
    }
    for (const WorldFrame::Chest& chest : frame.chests) {
        if (chest.object == nullptr) continue;
        const Vec2f c = objectCenter(*chest.object);
        if (!chest.opened && !near(c)) continue;
        marks.push_back(Mark{chest.opened ? MarkKind::ChestOpened : MarkKind::Chest, c});
    }
    for (const WorldFrame::Foe& foe : frame.foes) {
        if (foe.object == nullptr) continue;
        const core::Point cell = foeCellOf(foe);
        const Vec2f c{static_cast<float>(cell.x) + 0.5f, static_cast<float>(cell.y) + 0.5f};
        if (!near(c)) continue;
        marks.push_back(Mark{MarkKind::Foe, c});
    }
    // 主线目标：在这张图上就是它；在别的图上就是那道通往它的门（WorldScene::portalIsGuide）。
    if (frame.objectiveTarget != nullptr) {
        marks.push_back(Mark{MarkKind::Objective, objectCenter(*frame.objectiveTarget)});
    } else if (guideDoor != nullptr) {
        marks.push_back(Mark{MarkKind::Objective, objectCenter(*guideDoor)});
    }
    Mark me{MarkKind::Hero, heroCenter};
    me.facing = heroFacing;
    marks.push_back(me);
    return marks;
}

// ---------------------------------------------------------------------------
// 底图
// ---------------------------------------------------------------------------

void MapThumbnail::ensure(engine::Engine& engine, const core::TileMap& map) {
    if (mapId_ == map.id) return;
    mapId_ = map.id;
    levels_.clear();
    hd_ = false;
    hdPath_.clear();
    if (map.width <= 0 || map.height <= 0) return;

    const std::vector<std::string> found = engine.findAssets("art/maps_hd/" + map.id, "below.png");
    if (!found.empty()) {
        const engine::TextureId below = engine.loadTexture(found.front(), engine::ScaleMode::Pixel);
        const engine::Point size = engine.textureSize(below);
        const float srcCell = static_cast<float>(kHdArtTilePx);
        if (below != engine::kInvalidTexture && size.x == map.width * kHdArtTilePx &&
            size.y == map.height * kHdArtTilePx) {
            const engine::TextureId previous = engine.renderTarget();
            // 第一档从烘焙图本身缩：临时换成线性取样（2×2 取平均），画完换回去——那张图世界画面还在用。
            engine.setTextureScaleMode(below, engine::ScaleMode::Linear);
            engine::TextureId source = below;
            float sourceCell = srcCell;
            for (const float px : kHdLevels) {
                const int w = std::max(1, static_cast<int>(std::lround(static_cast<float>(map.width) * px)));
                const int h = std::max(1, static_cast<int>(std::lround(static_cast<float>(map.height) * px)));
                const engine::TextureId target = engine.createRenderTarget(w, h, engine::ScaleMode::Linear);
                if (target == engine::kInvalidTexture) break;
                engine.setRenderTarget(target);
                engine.clear(Color{0, 0, 0, 0});
                const float sw = static_cast<float>(map.width) * sourceCell;
                const float sh = static_cast<float>(map.height) * sourceCell;
                engine::DrawOptions copy;
                copy.blend = BlendMode::None;
                engine.drawTexture(source, RectF{0.f, 0.f, sw, sh},
                                   RectF{0.f, 0.f, static_cast<float>(w), static_cast<float>(h)}, copy);
                Level level;
                level.texture = engine::OwnedTexture(engine, target);
                level.pxPerCell = px;
                levels_.push_back(std::move(level));
                source = target;
                sourceCell = px;
            }
            engine.setRenderTarget(previous);
            engine.setTextureScaleMode(below, engine::ScaleMode::Pixel);
            if (!levels_.empty()) {
                hd_ = true;
                hdPath_ = found.front();
                return;
            }
        }
    }

    // 剪影：一格一像素，走得通的旧纸色、走不通的墨色。按像素放大（一格一块，边界清楚）。
    const std::vector<std::uint32_t> rgba = silhouettePixels(map);
    const engine::TextureId silhouette = engine.createTexture(map.width, map.height, rgba, engine::ScaleMode::Pixel);
    if (silhouette == engine::kInvalidTexture) return;
    Level level;
    level.texture = engine::OwnedTexture(engine, silhouette);
    level.pxPerCell = 1.f;
    levels_.push_back(std::move(level));
}

void MapThumbnail::draw(engine::Engine& engine, const Layout& layout, float alpha) {
    if (levels_.empty() || alpha <= 0.f) return;
    const CellRange cells = visibleCells(layout);
    if (cells.x1 <= cells.x0 || cells.y1 <= cells.y0) return;
    const Vec2f a = toScreen(layout, Vec2f{cells.x0, cells.y0});
    const Vec2f b = toScreen(layout, Vec2f{cells.x1, cells.y1});
    const RectF dst{a.x, a.y, b.x - a.x, b.y - a.y};
    engine::DrawOptions options;
    options.tint = Color{255, 255, 255, static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f)};

    // 区域地图放得比 12 像素一格还大时：直接画烘焙图本身（按像素取样，与世界画面同一种颗粒）。
    if (hd_ && layout.cellPx > levels_.front().pxPerCell * 1.25f) {
        const engine::TextureId below = engine.loadTexture(hdPath_, engine::ScaleMode::Pixel);
        if (below != engine::kInvalidTexture) {
            const float s = static_cast<float>(kHdArtTilePx);
            engine.drawTexture(below, RectF{cells.x0 * s, cells.y0 * s, (cells.x1 - cells.x0) * s, (cells.y1 - cells.y0) * s},
                               dst, options);
            return;
        }
    }
    // 挑一档：不比要画的小的里面最小的那一档（缩得最少）；都比它小就用最大的那一档。
    const Level* pick = &levels_.front();
    for (const Level& level : levels_) {
        if (level.pxPerCell + 1e-3f >= layout.cellPx) pick = &level;
    }
    const float s = pick->pxPerCell;
    engine.drawTexture(pick->texture.get(),
                       RectF{cells.x0 * s, cells.y0 * s, (cells.x1 - cells.x0) * s, (cells.y1 - cells.y0) * s}, dst,
                       options);
}

// ---------------------------------------------------------------------------
// 标记
// ---------------------------------------------------------------------------

void drawMarkShape([[maybe_unused]] engine::Engine& engine, const ui::Theme& theme, hud::Geometry& g, MarkKind kind, float x, float y,
                   float size, float alpha, int facing) {
    const Color ink = withA(theme.inkDeep, alpha);
    const Color grey = withA(Color{150, 144, 130, 230}, alpha);
    switch (kind) {
        case MarkKind::Door:
            square(g, x, y, size * 0.75f + 1.f, ink);
            square(g, x, y, size * 0.75f, withA(Color{120, 200, 230, 255}, alpha));
            break;
        case MarkKind::DoorLocked:
            square(g, x, y, size * 0.75f + 1.f, ink);
            square(g, x, y, size * 0.75f, withA(Color{110, 112, 120, 255}, alpha));
            break;
        case MarkKind::DoorGuide:
            square(g, x, y, size * 0.9f + 1.5f, ink);
            square(g, x, y, size * 0.9f, withA(theme.goldBright, alpha));
            break;
        case MarkKind::Save:
            g.diamond(x, y, size + 1.5f, size + 1.5f, ink);
            g.diamond(x, y, size, size, withA(theme.azure, alpha));
            break;
        case MarkKind::Facility:
            square(g, x, y, size * 0.7f + 1.f, ink);
            square(g, x, y, size * 0.7f, withA(theme.jade, alpha));
            break;
        case MarkKind::Chest:
            square(g, x, y, size * 0.65f + 1.f, ink);
            square(g, x, y, size * 0.65f, withA(Color{236, 176, 72, 255}, alpha));
            break;
        case MarkKind::ChestOpened:
            square(g, x, y, size * 0.6f, grey);
            break;
        case MarkKind::Gather:
            g.fan(x, y, size * 0.75f + 1.f, size * 0.75f + 1.f, ink, ink, 12);
            g.fan(x, y, size * 0.75f, size * 0.75f, withA(Color{140, 232, 120, 255}, alpha),
                  withA(Color{90, 190, 90, 255}, alpha), 12);
            break;
        case MarkKind::GatherDone:
            g.fan(x, y, size * 0.6f, size * 0.6f, grey, grey, 12);
            break;
        case MarkKind::Foe:
            g.fan(x, y, size * 0.85f + 1.f, size * 0.85f + 1.f, ink, ink, 12);
            g.fan(x, y, size * 0.85f, size * 0.85f, withA(Color{255, 110, 90, 255}, alpha),
                  withA(theme.cinnabar, alpha), 12);
            break;
        case MarkKind::Objective:
            g.diamond(x, y, size * 1.4f + 2.f, size * 1.7f + 2.f, ink);
            g.diamond(x, y, size * 1.4f, size * 1.7f, withA(theme.goldBright, alpha));
            break;
        case MarkKind::Hero: {
            // 朝向的三角：尖朝 facing（0 上 1 右 2 下 3 左）。
            const float r = size * 1.3f;
            const float dx[] = {0.f, 1.f, 0.f, -1.f};
            const float dy[] = {-1.f, 0.f, 1.f, 0.f};
            const int f = (facing >= 0 && facing < 4) ? facing : 2;
            const float fx = dx[f];
            const float fy = dy[f];
            const float px = -fy;
            const float py = fx;
            const auto tri = [&](float rr, Color c) {
                g.triangle(vtx(x + fx * rr * 1.2f, y + fy * rr * 1.2f, c),
                           vtx(x - fx * rr * 0.8f + px * rr, y - fy * rr * 0.8f + py * rr, c),
                           vtx(x - fx * rr * 0.8f - px * rr, y - fy * rr * 0.8f - py * rr, c));
            };
            tri(r + 1.6f, ink);
            tri(r, withA(Color{255, 250, 236, 255}, alpha));
            break;
        }
    }
}

void drawMarks(engine::Engine& engine, const ui::Theme& theme, hud::Geometry& geometry, const Layout& layout,
               const std::vector<Mark>& marks, float size, float alpha) {
    if (alpha <= 0.f) return;
    for (const Mark& mark : marks) {
        Vec2f at = toScreen(layout, mark.cell);
        if (!contains(layout, at, 1.f)) {
            // 主线目标、主角出了取景就贴边（3.1）；别的出了就不画。
            if (mark.kind != MarkKind::Objective && mark.kind != MarkKind::Hero) continue;
            at = clampInside(layout, at, size * 1.8f);
        }
        drawMarkShape(engine, theme, geometry, mark.kind, at.x, at.y, size, alpha, mark.facing);
    }
    geometry.draw(engine, BlendMode::Alpha);
}

}  // namespace fanren::game::minimap
