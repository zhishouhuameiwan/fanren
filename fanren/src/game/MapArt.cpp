#include "game/MapArt.h"

#include <algorithm>
#include <array>

namespace fanren::game {
namespace {

// 图案的坐标一律按 32px 的格子写，再按实际格宽等比换算：行走格 32、战斗格 48，
// 两处要长得一样就不能各写一套数。
[[nodiscard]] int scaleX(const engine::Rect& cell, int v) { return cell.x + cell.w * v / 32; }
[[nodiscard]] int scaleY(const engine::Rect& cell, int v) { return cell.y + cell.h * v / 32; }
[[nodiscard]] int sizeX(const engine::Rect& cell, int v) { return std::max(1, cell.w * v / 32); }
[[nodiscard]] int sizeY(const engine::Rect& cell, int v) { return std::max(1, cell.h * v / 32); }

// 提亮 / 压暗。同一个色相上做出层次，省得配色表里为每一处阴影各存一个颜色。
[[nodiscard]] engine::Color shade(const engine::Color& base, int delta) {
    const auto clampChannel = [delta](std::uint8_t channel) {
        return static_cast<std::uint8_t>(std::clamp(static_cast<int>(channel) + delta, 0, 255));
    };
    return engine::Color{clampChannel(base.r), clampChannel(base.g), clampChannel(base.b), base.a};
}

// 每格一个稳定的杂凑。草丛与碎石因此每格长得不同、每帧却一模一样。
//
// 不用 rand()：那会让草丛每帧乱跳（玩家看到的是满地抽搐的噪点），
// 而且无头测试无从复现同一幅画面。
[[nodiscard]] std::uint32_t cellHash(int x, int y) {
    std::uint32_t h = static_cast<std::uint32_t>(x) * 73856093u ^
                      static_cast<std::uint32_t>(y) * 19349663u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return h;
}

// 从杂凑里取一个 [lo, lo + span*step] 的坐标。散点全走这一个口子，
// 免得各处各写一遍位运算再各自算错边界、把点画到格子外面去。
[[nodiscard]] int scatter(std::uint32_t hash, int shift, int lo, int span, int step) {
    return lo + static_cast<int>((hash >> shift) & static_cast<std::uint32_t>(span)) * step;
}

constexpr engine::Color kOutline{14, 12, 18, 220};      // 人物描边：任何地表上都压得住
constexpr engine::Color kPlateFill{10, 10, 14, 205};    // 名牌底条
constexpr engine::Color kPlateEdge{198, 190, 170, 120};

// 室外。gid 1 是草地、3 是水面、5 是土墙木墙、8 是篱笆摊架。
constexpr TilePalette kOutdoorPalette{
    /*indoor*/ false,
    /*ground*/ {64, 100, 58, 255},
    /*groundDetail*/ {86, 126, 70, 255},
    /*road*/ {136, 114, 78, 255},
    /*roadDetail*/ {110, 90, 60, 255},
    /*special*/ {54, 94, 126, 255},
    /*specialDetail*/ {104, 152, 184, 255},
    /*tilled*/ {104, 78, 50, 255},
    /*tilledDetail*/ {78, 56, 34, 255},
    /*wall*/ {104, 92, 78, 255},
    /*wallSeam*/ {70, 60, 50, 255},
    /*wallTop*/ {146, 132, 112, 255},
    /*fence*/ {158, 120, 72, 255},
    /*fenceDetail*/ {76, 54, 32, 255},
    /*flora*/ {118, 166, 90, 255},
    /*floraAccent*/ {214, 206, 120, 255},
    /*rubble*/ {132, 126, 118, 255},
    /*canopy*/ {40, 78, 48, 176},
    /*canopyDetail*/ {58, 106, 62, 208},
};

// 室内。同样的 gid 1 在这里是木地板，3 是石渣地，8 是书架家具。
constexpr TilePalette kIndoorPalette{
    /*indoor*/ true,
    /*ground*/ {104, 80, 56, 255},
    /*groundDetail*/ {76, 56, 38, 255},
    /*road*/ {118, 110, 98, 255},
    /*roadDetail*/ {92, 86, 76, 255},
    /*special*/ {44, 62, 82, 255},
    /*specialDetail*/ {86, 122, 150, 255},
    /*tilled*/ {74, 70, 66, 255},
    /*tilledDetail*/ {102, 98, 92, 255},
    /*wall*/ {78, 68, 60, 255},
    /*wallSeam*/ {52, 45, 40, 255},
    /*wallTop*/ {112, 100, 88, 255},
    /*fence*/ {140, 104, 64, 255},
    /*fenceDetail*/ {70, 50, 30, 255},
    /*flora*/ {96, 132, 80, 255},
    /*floraAccent*/ {198, 190, 120, 255},
    /*rubble*/ {112, 106, 100, 255},
    /*canopy*/ {32, 28, 26, 176},
    /*canopyDetail*/ {52, 46, 42, 208},
};

// NPC 的衣色。八个色相彼此拉开，且都不是主角那身亮白——屏幕上那个白衣的人
// 永远只有一个。
constexpr std::array<engine::Color, 8> kNpcRobes{{
    {84, 108, 156, 255},    // 靛
    {150, 92, 84, 255},     // 赭
    {96, 132, 104, 255},    // 松
    {148, 132, 84, 255},    // 秋香
    {120, 96, 148, 255},    // 紫
    {80, 128, 140, 255},    // 青
    {156, 116, 76, 255},    // 褐
    {108, 108, 120, 255},   // 灰
}};

constexpr std::array<engine::Color, 3> kNpcHeads{{
    {226, 196, 164, 255},
    {206, 174, 142, 255},
    {186, 152, 124, 255},
}};

[[nodiscard]] std::uint32_t textHash(const std::string& text) {
    // FNV-1a。要的只是「同一个 id 每次都落在同一格」，不需要抗碰撞。
    std::uint32_t hash = 2166136261u;
    for (const char ch : text) {
        hash ^= static_cast<std::uint32_t>(static_cast<unsigned char>(ch));
        hash *= 16777619u;
    }
    return hash;
}

void drawGroundTile(engine::Engine& engine, const engine::Rect& cell, const TilePalette& palette,
                    std::uint32_t hash, int cellX, int cellY) {
    // 棋盘式的极轻微明暗。一整片同色的地表看久了是一块死面，玩家分不清自己
    // 到底有没有在走——差值压在 ±3，近看几乎看不见，成片看就有了纹理。
    const int tint = ((cellX + cellY) % 2 == 0) ? 2 : -2;
    engine.drawRect(cell, shade(palette.ground, tint), true);

    if (palette.indoor) {
        // 木地板：两道通长的横缝 + 一道错开的竖缝。横缝在每一格的同一高度，
        // 于是成片时连成整条板，这正是地板该有的样子。
        engine.drawRect(engine::Rect{cell.x, scaleY(cell, 10), cell.w, sizeY(cell, 1)},
                        palette.groundDetail, true);
        engine.drawRect(engine::Rect{cell.x, scaleY(cell, 21), cell.w, sizeY(cell, 1)},
                        palette.groundDetail, true);
        engine.drawRect(engine::Rect{scaleX(cell, scatter(hash, 0, 4, 3, 7)), scaleY(cell, 10),
                                     sizeX(cell, 1), sizeY(cell, 11)},
                        palette.groundDetail, true);
        return;
    }

    // 草地：三簇草。位置由本格杂凑定，帧间不动。
    for (int i = 0; i < 3; ++i) {
        const int shift = i * 8;
        engine.drawRect(engine::Rect{scaleX(cell, scatter(hash, shift, 2, 7, 3)),
                                     scaleY(cell, scatter(hash, shift + 4, 2, 7, 3)),
                                     sizeX(cell, 3), sizeY(cell, 2)},
                        palette.groundDetail, true);
    }
}

void drawRoadTile(engine::Engine& engine, const engine::Rect& cell, const TilePalette& palette,
                  std::uint32_t hash) {
    engine.drawRect(cell, palette.road, true);
    // 碎石子：一深一浅交替，路面因此不是一块纯色塑料板。
    for (int i = 0; i < 4; ++i) {
        const int shift = i * 6;
        const engine::Color speck = (i % 2 == 0) ? palette.roadDetail : shade(palette.road, 22);
        engine.drawRect(engine::Rect{scaleX(cell, scatter(hash, shift, 2, 7, 3)),
                                     scaleY(cell, scatter(hash, shift + 3, 2, 7, 3)),
                                     sizeX(cell, 3), sizeY(cell, 2)},
                        speck, true);
    }
}

void drawWaterTile(engine::Engine& engine, const engine::Rect& cell, const TilePalette& palette,
                   std::uint32_t hash) {
    engine.drawRect(cell, palette.special, true);
    // 两道错开的横波。位置由本格杂凑定，成片时波纹不会排成一条直线。
    engine.drawRect(engine::Rect{scaleX(cell, scatter(hash, 0, 3, 3, 4)), scaleY(cell, 9),
                                 sizeX(cell, 13), sizeY(cell, 2)},
                    palette.specialDetail, true);
    engine.drawRect(engine::Rect{scaleX(cell, scatter(hash, 5, 6, 3, 3)), scaleY(cell, 20),
                                 sizeX(cell, 10), sizeY(cell, 2)},
                    palette.specialDetail, true);
}

void drawTilledTile(engine::Engine& engine, const engine::Rect& cell, const TilePalette& palette,
                    std::uint32_t hash) {
    // 翻土 / 碎石 / 焦土：三道通长的垄。垄在每一格的同一高度，于是成片时连成
    // 整条田垄——这与水面那两道错开的短波是两种一眼分得开的图案，而不是同一个
    // 图案换个颜色。
    engine.drawRect(cell, palette.tilled, true);
    for (const int row : {6, 15, 24}) {
        engine.drawRect(engine::Rect{cell.x, scaleY(cell, row), cell.w, sizeY(cell, 2)},
                        palette.tilledDetail, true);
    }
    // 土坷垃：打断垄的规整，免得一片田看起来像张格纸。
    engine.drawRect(engine::Rect{scaleX(cell, scatter(hash, 0, 4, 3, 7)),
                                 scaleY(cell, scatter(hash, 4, 4, 3, 7)), sizeX(cell, 3),
                                 sizeY(cell, 3)},
                    shade(palette.tilled, 18), true);
}

void drawWallTile(engine::Engine& engine, const engine::Rect& cell, const TilePalette& palette) {
    engine.drawRect(cell, palette.wall, true);
    // 顶边一道高光、底边一道暗影：墙因此有厚度，不再是贴在地上的一张色纸。
    engine.drawRect(engine::Rect{cell.x, cell.y, cell.w, sizeY(cell, 3)}, palette.wallTop, true);
    engine.drawRect(engine::Rect{cell.x, scaleY(cell, 29), cell.w, sizeY(cell, 3)},
                    shade(palette.wall, -26), true);

    // 砖缝：两道横缝，竖缝上下错开。错开是关键——对齐的竖缝看起来是格子网，
    // 错开才是砌出来的墙。
    engine.drawRect(engine::Rect{cell.x, scaleY(cell, 12), cell.w, sizeY(cell, 1)},
                    palette.wallSeam, true);
    engine.drawRect(engine::Rect{cell.x, scaleY(cell, 22), cell.w, sizeY(cell, 1)},
                    palette.wallSeam, true);
    engine.drawRect(engine::Rect{scaleX(cell, 16), scaleY(cell, 3), sizeX(cell, 1), sizeY(cell, 9)},
                    palette.wallSeam, true);
    engine.drawRect(engine::Rect{scaleX(cell, 8), scaleY(cell, 12), sizeX(cell, 1), sizeY(cell, 10)},
                    palette.wallSeam, true);
    engine.drawRect(
        engine::Rect{scaleX(cell, 24), scaleY(cell, 12), sizeX(cell, 1), sizeY(cell, 10)},
        palette.wallSeam, true);
    engine.drawRect(
        engine::Rect{scaleX(cell, 16), scaleY(cell, 22), sizeX(cell, 1), sizeY(cell, 7)},
        palette.wallSeam, true);
}

void drawFenceTile(engine::Engine& engine, const engine::Rect& cell, const TilePalette& palette) {
    // 篱笆 / 摊架 / 兵器架 / 书架：**实心挡路**，而这正是旧色板画成草地绿的那一类。
    // 深底 + 立柱 + 横档 + 描边，四样凑起来才不至于被当成地上的花纹。
    engine.drawRect(cell, palette.fenceDetail, true);
    for (const int post : {3, 14, 25}) {
        engine.drawRect(engine::Rect{scaleX(cell, post), scaleY(cell, 4), sizeX(cell, 5),
                                     sizeY(cell, 25)},
                        palette.fence, true);
    }
    engine.drawRect(engine::Rect{cell.x, scaleY(cell, 10), cell.w, sizeY(cell, 4)}, palette.fence,
                    true);
    engine.drawRect(cell, shade(palette.fenceDetail, -20), false);
}

void drawFloraTile(engine::Engine& engine, const engine::Rect& cell, const TilePalette& palette,
                   std::uint32_t hash) {
    // 装饰层**不铺满整格**。旧实现按整格不透明画，于是「地表上长了点花草」变成
    // 「地表整格被替换掉」，玩家看到的是草地上一块块补丁。
    for (int i = 0; i < 4; ++i) {
        const int shift = i * 7;
        engine.drawRect(engine::Rect{scaleX(cell, scatter(hash, shift, 3, 7, 3)),
                                     scaleY(cell, scatter(hash, shift + 3, 3, 7, 3)),
                                     sizeX(cell, 3), sizeY(cell, 4)},
                        palette.flora, true);
    }
    engine.drawRect(engine::Rect{scaleX(cell, scatter(hash, 13, 6, 3, 5)),
                                 scaleY(cell, scatter(hash, 17, 6, 3, 5)), sizeX(cell, 3),
                                 sizeY(cell, 3)},
                    palette.floraAccent, true);
}

void drawRubbleTile(engine::Engine& engine, const engine::Rect& cell, const TilePalette& palette,
                    std::uint32_t hash) {
    for (int i = 0; i < 5; ++i) {
        const int shift = i * 5;
        engine.drawRect(engine::Rect{scaleX(cell, scatter(hash, shift, 2, 7, 3)),
                                     scaleY(cell, scatter(hash, shift + 3, 2, 7, 3)),
                                     sizeX(cell, 4), sizeY(cell, 2)},
                        palette.rubble, true);
    }
}

void drawCanopyTile(engine::Engine& engine, const engine::Rect& cell, const TilePalette& palette,
                    std::uint32_t hash) {
    // 树冠画在人物之上，所以**必须半透明**：旧实现用不透明的水蓝，主角走到树下
    // 就整个消失，玩家只能凭记忆猜自己站在哪。
    engine.drawRect(cell, palette.canopy, true);
    for (int i = 0; i < 3; ++i) {
        const int shift = i * 8;
        engine.drawRect(engine::Rect{scaleX(cell, scatter(hash, shift, 2, 3, 6)),
                                     scaleY(cell, scatter(hash, shift + 4, 2, 3, 6)),
                                     sizeX(cell, 9), sizeY(cell, 8)},
                        palette.canopyDetail, true);
    }
}

void drawUnknownTile(engine::Engine& engine, const engine::Rect& cell) {
    // 约定之外的编号画成刺眼的洋红十字。**故意难看**：这是给关卡作者看的报错，
    // 不是给玩家看的地表，而悄悄兜底成某个正常颜色正是旧实现的那个坑。
    engine.drawRect(cell, engine::Color{200, 0, 160, 255}, true);
    engine.drawRect(engine::Rect{scaleX(cell, 14), scaleY(cell, 4), sizeX(cell, 4), sizeY(cell, 24)},
                    engine::Color{255, 255, 255, 255}, true);
    engine.drawRect(engine::Rect{scaleX(cell, 4), scaleY(cell, 14), sizeX(cell, 24), sizeY(cell, 4)},
                    engine::Color{255, 255, 255, 255}, true);
}

}  // namespace

TileRole tileRole(int gid) {
    switch (gid) {
        case 0: return TileRole::Empty;
        case 1: return TileRole::Ground;
        case 2: return TileRole::Road;
        case 3: return TileRole::Special;
        case 4: return TileRole::Canopy;
        case 5: return TileRole::Wall;
        case 6: return TileRole::Flora;
        case 7: return TileRole::Rubble;
        case 8: return TileRole::Fence;
        default: break;
    }
    // 负数也走这里。Tiled 用高位存翻转标志，出现负 gid 说明地图导出时开了
    // 翻转——那同样是「约定之外」，该被看见而不是被模运算抹平。
    return TileRole::Unknown;
}

const TilePalette& tilePalette(bool outdoor) {
    return outdoor ? kOutdoorPalette : kIndoorPalette;
}

void drawTile(engine::Engine& engine, TileRole role, const engine::Rect& cell,
              const TilePalette& palette, int cellX, int cellY, bool blocked) {
    if (cell.w <= 0 || cell.h <= 0 || role == TileRole::Empty) return;
    const std::uint32_t hash = cellHash(cellX, cellY);
    switch (role) {
        case TileRole::Ground:  drawGroundTile(engine, cell, palette, hash, cellX, cellY); return;
        case TileRole::Road:    drawRoadTile(engine, cell, palette, hash); return;
        case TileRole::Special:
            // 判据见头文件：挡路的是水，踩得上的是地。
            if (blocked) {
                drawWaterTile(engine, cell, palette, hash);
            } else {
                drawTilledTile(engine, cell, palette, hash);
            }
            return;
        case TileRole::Canopy:  drawCanopyTile(engine, cell, palette, hash); return;
        case TileRole::Wall:    drawWallTile(engine, cell, palette); return;
        case TileRole::Flora:   drawFloraTile(engine, cell, palette, hash); return;
        case TileRole::Rubble:  drawRubbleTile(engine, cell, palette, hash); return;
        case TileRole::Fence:   drawFenceTile(engine, cell, palette); return;
        case TileRole::Unknown: drawUnknownTile(engine, cell); return;
        case TileRole::Empty:
        default: return;
    }
}

void drawFigure(engine::Engine& engine, const engine::Rect& cell, const FigureStyle& style,
                int facing) {
    if (cell.w <= 0 || cell.h <= 0) return;

    // 影子。人与地面之间有这一抹暗，人才像是站在地上而不是贴在墙上。
    engine.drawRect(engine::Rect{scaleX(cell, 7), scaleY(cell, 25), sizeX(cell, 18), sizeY(cell, 4)},
                    engine::Color{0, 0, 0, 90}, true);

    // 人占格子的六成宽。更小的话（初版是四成）在一格草地上就是一小撮色点，
    // 「地图上有个人」这件事得凑近看才成立。
    const engine::Rect body{scaleX(cell, 8), scaleY(cell, 13), sizeX(cell, 16), sizeY(cell, 14)};
    const engine::Rect head{scaleX(cell, 10), scaleY(cell, 3), sizeX(cell, 12), sizeY(cell, 11)};
    engine.drawRect(body, style.robe, true);
    engine.drawRect(head, style.head, true);
    // 衣领：身与头之间的一道亮边，把两块色分开。没有它，深色衣服配深色头发
    // 就糊成一个方块——那正是现在这个问题的源头。
    engine.drawRect(engine::Rect{body.x, body.y, body.w, sizeY(cell, 2)}, style.trim, true);
    // 描边压在最上层：浅色地表上的浅色人、深色墙前的深色人，靠这一圈才分得出来。
    engine.drawRect(body, kOutline, false);
    engine.drawRect(head, kOutline, false);

    if (facing == kNoFacing) return;
    // 朝向凸起。交互看的是「面朝的前一格」，玩家得先看得见自己朝哪。
    switch (facing) {
        case 0:
            engine.drawRect(engine::Rect{scaleX(cell, 14), scaleY(cell, 0), sizeX(cell, 4),
                                         sizeY(cell, 3)},
                            style.trim, true);
            return;
        case 1:
            engine.drawRect(engine::Rect{scaleX(cell, 24), scaleY(cell, 16), sizeX(cell, 4),
                                         sizeY(cell, 5)},
                            style.trim, true);
            return;
        case 2:
            engine.drawRect(engine::Rect{scaleX(cell, 14), scaleY(cell, 27), sizeX(cell, 4),
                                         sizeY(cell, 3)},
                            style.trim, true);
            return;
        default:
            engine.drawRect(engine::Rect{scaleX(cell, 4), scaleY(cell, 16), sizeX(cell, 4),
                                         sizeY(cell, 5)},
                            style.trim, true);
            return;
    }
}

FigureStyle npcStyle(const std::string& roleId) {
    const std::uint32_t hash = textHash(roleId);
    const engine::Color robe = kNpcRobes[hash % kNpcRobes.size()];
    const engine::Color head = kNpcHeads[(hash >> 8) % kNpcHeads.size()];
    return FigureStyle{robe, head, shade(robe, 46)};
}

const FigureStyle& playerStyle() {
    static const FigureStyle style{engine::Color{228, 230, 238, 255},
                                   engine::Color{238, 214, 184, 255},
                                   engine::Color{246, 204, 110, 255}};
    return style;
}

void drawNamePlate(engine::Engine& engine, const std::string& label, int centerX, int bottomY,
                   int fontSize, const engine::Color& textColor) {
    if (label.empty() || fontSize <= 0) return;
    const engine::Point size = engine.measureText(label, fontSize);
    if (size.x <= 0 || size.y <= 0) return;   // 无头模式量不出字，画了也是空操作

    const engine::Rect plate{centerX - size.x / 2 - 4, bottomY - size.y - 4, size.x + 8,
                             size.y + 6};
    engine.drawRect(plate, kPlateFill, true);
    engine.drawRect(plate, kPlateEdge, false);
    engine.drawText(label, plate.x + 4, plate.y + 3, fontSize, textColor);
}

void drawGlyph(engine::Engine& engine, const std::string& glyph, const engine::Rect& cell,
               int fontSize, const engine::Color& color) {
    if (glyph.empty() || fontSize <= 0) return;
    const engine::Point size = engine.measureText(glyph, fontSize);
    if (size.x <= 0 || size.y <= 0) return;
    engine.drawText(glyph, cell.x + (cell.w - size.x) / 2, cell.y + (cell.h - size.y) / 2, fontSize,
                    color);
}

std::string facilityGlyph(const std::string& kind) {
    if (kind == "alchemy") return "丹";
    if (kind == "forge") return "器";
    if (kind == "talisman") return "符";
    if (kind == "formation") return "阵";
    if (kind == "field") return "田";
    if (kind == "meditate") return "坐";
    if (kind == "shop") return "市";
    if (kind == "board") return "榜";
    if (kind == "save") return "存";
    return "?";
}

engine::Color facilityColor(const std::string& kind) {
    if (kind == "alchemy") return engine::Color{186, 104, 80, 255};
    if (kind == "forge") return engine::Color{148, 96, 64, 255};
    if (kind == "talisman") return engine::Color{176, 158, 96, 255};
    if (kind == "formation") return engine::Color{120, 104, 176, 255};
    if (kind == "field") return engine::Color{96, 150, 88, 255};
    if (kind == "meditate") return engine::Color{92, 140, 164, 255};
    if (kind == "shop") return engine::Color{198, 160, 76, 255};
    if (kind == "board") return engine::Color{140, 128, 108, 255};
    if (kind == "save") return engine::Color{104, 168, 140, 255};
    return engine::Color{200, 0, 160, 255};
}

}  // namespace fanren::game
