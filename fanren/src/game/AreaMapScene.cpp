#include "game/AreaMapScene.h"

#include <algorithm>
#include <utility>

#include "game/Application.h"
#include "game/Exploration.h"
#include "game/WorldScene.h"
#include "ui/Widgets.h"

namespace fanren::game {

namespace {

using engine::RectF;

// 面板与两栏（逻辑像素，1280×720）。
constexpr engine::Rect kPanel{36, 24, 1208, 672};
constexpr float kMapX = 60.f;
constexpr float kMapY = 84.f;
constexpr float kMapW = 860.f;
constexpr float kMapH = 560.f;
constexpr int kSideX = 944;          // 右栏起笔
constexpr int kSideW = 280;
constexpr int kSmallFont = 15;
constexpr int kBodyFont = 18;
// 区域地图上一格至少这么大：小于它就开窗、方向键平移。至多 32（小屋子不至于放成一格一块砖）。
constexpr float kMinCellPx = 8.f;
constexpr float kMaxCellPx = 32.f;
constexpr float kPanStep = 3.f;      // 方向键一下平移几格
constexpr float kMarkSize = 6.f;

constexpr const char* kSfxOpen = "ui_open";
constexpr const char* kSfxClose = "ui_close";

// 图例的次序与说法（ui_map.json）。
struct LegendRow {
    minimap::MarkKind kind;
    const char* key;
};
constexpr LegendRow kLegend[] = {
    {minimap::MarkKind::Hero, "ui.map.legend.hero"},         {minimap::MarkKind::Objective, "ui.map.legend.objective"},
    {minimap::MarkKind::Door, "ui.map.legend.door"},         {minimap::MarkKind::DoorGuide, "ui.map.legend.door_guide"},
    {minimap::MarkKind::DoorLocked, "ui.map.legend.door_locked"}, {minimap::MarkKind::Save, "ui.map.legend.save"},
    {minimap::MarkKind::Facility, "ui.map.legend.facility"}, {minimap::MarkKind::Chest, "ui.map.legend.chest"},
    {minimap::MarkKind::ChestOpened, "ui.map.legend.chest_opened"}, {minimap::MarkKind::Gather, "ui.map.legend.gather"},
    {minimap::MarkKind::GatherDone, "ui.map.legend.gather_done"}, {minimap::MarkKind::Foe, "ui.map.legend.foe"},
};

constexpr const char* kFacilityKinds[] = {"save", "meditate", "shop", "field", "alchemy",
                                          "lilian", "board", "talisman", "forge"};

constexpr const char* kOtherKeys[] = {
    "ui.map.title",       "ui.map.danger.none", "ui.map.explore.none", "ui.map.explore.lilian",
    "ui.map.legend",      "ui.map.exits",       "ui.map.exits.none",   "ui.map.exit.guide",
    "ui.map.exit.locked", "ui.map.hint",        "ui.map.hint.pan",     "ui.map.reveal_note",
};

std::string facilityKey(const std::string& kind) {
    return "ui.map.facility." + kind;
}

}  // namespace

AreaMapScene::AreaMapScene() = default;
AreaMapScene::~AreaMapScene() = default;

RectF AreaMapScene::mapRect() {
    return RectF{kMapX, kMapY, kMapW, kMapH};
}

std::vector<AreaMapScene::Exit> AreaMapScene::exitsOf(const WorldFrame& frame,
                                                      const std::function<std::string(const std::string&)>& nameOf) {
    std::vector<Exit> exits;
    for (const WorldFrame::Portal& portal : frame.portals) {
        if (portal.object == nullptr) continue;
        const std::string target = minimap::portalTarget(*portal.object);
        if (target.empty()) continue;
        auto it = std::find_if(exits.begin(), exits.end(), [&](const Exit& e) { return e.mapId == target; });
        if (it == exits.end()) {
            exits.push_back(Exit{target, nameOf(target), portal.guide, portal.open});
            continue;
        }
        it->guide = it->guide || portal.guide;
        it->open = it->open || portal.open;
    }
    return exits;
}

std::vector<std::string> AreaMapScene::areaMapStrings(const core::GameData& data) {
    std::vector<std::string> out;
    for (const LegendRow& row : kLegend) out.push_back(data.lookupText(row.key));
    for (const char* kind : kFacilityKinds) out.push_back(data.lookupText(facilityKey(kind)));
    for (const char* key : kOtherKeys) out.push_back(data.lookupText(key));
    return out;
}

void AreaMapScene::onEnter(Application& app) {
    app.engine().playSfx(kSfxOpen);
    laidOut_ = false;
    refresh(app);
}

void AreaMapScene::refresh(Application& app) {
    const core::TileMap* map = app.currentMap();
    WorldScene* world = app.worldScene();
    exits_.clear();
    marks_.clear();
    facilityLabels_.clear();
    if (map == nullptr || world == nullptr) return;
    const WorldFrame& frame = world->describeFrame(app);
    const core::GameState& state = app.state();
    const Vec2f hero{static_cast<float>(state.position.x), static_cast<float>(state.position.y)};
    if (!laidOut_) {
        layout_ = minimap::fit(mapRect(), map->width, map->height, Vec2f{hero.x + 0.5f, hero.y + 0.5f}, kMinCellPx,
                               kMaxCellPx);
        laidOut_ = true;
    }
    marks_ = minimap::collectMarks(frame, app.worldObjects(), hero, state.facing);
    for (const minimap::Mark& mark : marks_) {
        if (mark.kind != minimap::MarkKind::Facility && mark.kind != minimap::MarkKind::Save) continue;
        const std::string key = facilityKey(mark.facility);
        const auto it = app.data().text.find(key);
        if (it != app.data().text.end()) facilityLabels_.emplace_back(it->second, mark);
    }
    exits_ = exitsOf(frame, [&app](const std::string& mapId) {
        const std::string key = core::mapDisplayNameKey(mapId);
        if (key.empty()) return mapId;
        const auto it = app.data().text.find(key);
        return it == app.data().text.end() ? mapId : it->second;
    });
    title_ = frame.mapTitle;
    dangerText_ = frame.dangerStars > 0 ? hud::PlaceBanner::dangerLine(frame.dangerStars)
                                        : app.text("ui.world.danger") + "　" + app.text("ui.map.danger.none");
    if (app.inLilian()) {
        exploreText_ = app.text("ui.map.explore.lilian");
    } else {
        exploreText_ = frame.exploreLine.empty() ? app.text("ui.map.explore.none") : frame.exploreLine;
    }
    exploreComplete_ = frame.exploreComplete;
}

bool AreaMapScene::update(Application& app, double) {
    engine::Engine& e = app.engine();
    using Key = engine::Engine::Key;
    if (e.keyPressed(Key::Cancel) || e.keyPressed(Key::Map) || e.keyPressed(Key::Menu) || e.keyPressed(Key::Confirm)) {
        e.playSfx(kSfxClose);
        return false;
    }
    float dx = 0.f;
    float dy = 0.f;
    if (e.keyPressed(Key::Left)) dx -= kPanStep;
    if (e.keyPressed(Key::Right)) dx += kPanStep;
    if (e.keyPressed(Key::Up)) dy -= kPanStep;
    if (e.keyPressed(Key::Down)) dy += kPanStep;
    if (dx != 0.f || dy != 0.f) layout_ = minimap::panned(layout_, dx, dy);
    return true;
}

void AreaMapScene::render(Application& app) {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    refresh(app);
    const core::TileMap* map = app.currentMap();

    e.fillRect(RectF{0.f, 0.f, static_cast<float>(engine::kLogicalWidth), static_cast<float>(engine::kLogicalHeight)},
               theme.inkDeep, engine::BlendMode::Alpha);
    const std::string heading = title_.empty() ? app.text("ui.map.title") : title_;
    ui::drawPanel(e, kPanel, heading, theme);

    // 左：大图。
    const RectF area = mapRect();
    e.fillRect(area, ui::withAlpha(theme.ink, 230), engine::BlendMode::Alpha);
    ui::drawFrame(e, RectF{area.x - 2.f, area.y - 2.f, area.w + 4.f, area.h + 4.f}, theme.goldDim);
    if (map != nullptr) {
        thumb_.ensure(e, *map);
        thumb_.draw(e, layout_, 1.f);
    }
    minimap::drawMarks(e, theme, geometry_, layout_, marks_, kMarkSize, 1.f);
    for (const auto& [label, mark] : facilityLabels_) {
        const Vec2f at = minimap::toScreen(layout_, mark.cell);
        if (!minimap::contains(layout_, at, 2.f)) continue;
        e.drawText(label, static_cast<int>(at.x + kMarkSize + 4.f), static_cast<int>(at.y - 8.f), kSmallFont, theme.paper,
                   theme.bodyStyle);
    }

    // 右栏：凶险、收集、图例、出口。
    int y = static_cast<int>(kMapY);
    e.drawText(dangerText_, kSideX, y, kBodyFont, theme.paper, theme.bodyStyle);
    y += 30;
    e.drawText(exploreText_, kSideX, y, kBodyFont, exploreComplete_ ? theme.goldBright : theme.paper, theme.bodyStyle);
    y += 40;

    e.drawText(app.text("ui.map.legend"), kSideX, y, kBodyFont, theme.gold, theme.bodyStyle);
    y += 28;
    constexpr int kLegendRowH = 22;
    for (std::size_t i = 0; i < std::size(kLegend); ++i) {
        const int col = static_cast<int>(i % 2);
        const int row = static_cast<int>(i / 2);
        const float mx = static_cast<float>(kSideX + col * (kSideW / 2) + 8);
        const float my = static_cast<float>(y + row * kLegendRowH + 9);
        minimap::drawMarkShape(e, theme, geometry_, kLegend[i].kind, mx, my, 5.f, 1.f, 1);
        geometry_.draw(e, engine::BlendMode::Alpha);
        e.drawText(app.text(kLegend[i].key), static_cast<int>(mx + 14.f), static_cast<int>(my - 9.f), kSmallFont,
                   theme.paper, theme.bodyStyle);
    }
    y += static_cast<int>((std::size(kLegend) + 1) / 2) * kLegendRowH + 4;
    ui::drawTextBlock(e, app.text("ui.map.reveal_note"), engine::Rect{kSideX - theme.padding, y - 6, kSideW + theme.padding * 2, 52},
                      kSmallFont - 1, theme);
    y += 46;

    e.drawText(app.text("ui.map.exits"), kSideX, y, kBodyFont, theme.gold, theme.bodyStyle);
    y += 28;
    const int exitsBottom = kPanel.y + kPanel.h - 56;
    if (exits_.empty()) {
        e.drawText(app.text("ui.map.exits.none"), kSideX, y, kSmallFont + 1, theme.paperDim, theme.bodyStyle);
    }
    for (const Exit& exit : exits_) {
        if (y + 22 > exitsBottom) break;
        std::string line = exit.name;
        engine::Color color = theme.paper;
        if (exit.guide) {
            line = app.text("ui.map.exit.guide") + line;
            color = theme.goldBright;
        } else if (!exit.open) {
            line += app.text("ui.map.exit.locked");
            color = theme.paperDim;
        }
        e.drawText(line, kSideX, y, kSmallFont + 1, color, theme.bodyStyle);
        y += 24;
    }

    const std::string hint = app.text(layout_.whole ? "ui.map.hint" : "ui.map.hint.pan");
    const engine::Point size = e.measureText(hint, kSmallFont + 1);
    e.drawText(hint, kPanel.x + kPanel.w - theme.padding - size.x, kPanel.y + kPanel.h - theme.padding - size.y + 2,
               kSmallFont + 1, theme.paperDim, theme.bodyStyle);
}

}  // namespace fanren::game
