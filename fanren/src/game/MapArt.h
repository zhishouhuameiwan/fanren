#pragma once
// 占位美术：没有图集的这段时间里，瓦片、人物、设施靠什么被认出来。
//
// 为什么要单独一个模块，而不是继续留在 WorldScene 的匿名 namespace 里：
//
//   这里的表与 tools/mapgen/genmaps.py 文件头那张「瓦片 gid 约定」是同一件事的
//   两半。两半分家之后就错位了整整八个编号也没人发现——gid 8（篱笆 / 摊架 /
//   兵器架，实心挡路）在旧色板里落到 `8 % 8 == 0` 的兜底上，画成了草地绿，于是
//   全 18 张图共 390 格实心障碍长得和可走的草地一模一样；gid 4（树冠 / 屋檐，
//   画在人物之上）画成不透明水蓝，1081 格，主角走到树下就整个被盖住。
//
//   现在语义落在 tileRole 上、配色落在 TilePalette 上，且由 tests/MapArtTests.cpp
//   拿全部 18 张真地图逐格核对：任何一张图用了约定之外的编号，或者哪天有人把
//   语义与编号的对应改错，门禁上就红。
//
// 图集接入后本模块整体换成 drawTexture，调用点（WorldScene / BattleScene）不动
// ——这正是它该是一个模块而不是几个散落函数的理由。
#include <cstdint>
#include <string>

#include "engine/Engine.h"

namespace fanren::game {

// 瓦片语义。编号约定见 tools/mapgen/genmaps.py 文件头，全 18 张图共用一套，
// 室内外只换配色不换语义。
enum class TileRole {
    Empty,     // 0：这一层这一格什么也没有
    Ground,    // 1 主地表：草地 / 泥地 / 木地板 / 石地
    Road,      // 2 道路：土路 / 石板路 / 石阶
    Special,   // 3 特殊地表：水面 / 药圃翻土 / 碎石崖面
    Canopy,    // 4 front 遮挡：树冠 / 屋檐，画在人物之上
    Wall,      // 5 building 主体：墙体 / 屋身 / 岩壁
    Flora,     // 6 overlay 装饰：花草
    Rubble,    // 7 overlay 装饰：碎石 / 裂纹
    Fence,     // 8 building 次体：篱笆 / 摊架 / 石栏 / 家具（实心，挡路）
    Unknown,   // 约定之外的编号
};

// gid → 语义。约定之外的编号一律 Unknown，**不做任何就近兜底**：
// 旧实现那句 `index == 0 ? 1 : index` 就是一次就近兜底，它把 gid 8 悄悄画成了
// 草地，代价是一堵看不见的墙。Unknown 会被画成刺眼的洋红十字，一眼看得出
// 「这张图用了没人认识的编号」。
[[nodiscard]] TileRole tileRole(int gid);

// 一套配色。室内外各一份：同一个 gid 1 在村口是草地、在居所里是木地板，
// 语义相同而颜色必须不同，否则室内一片绿。
struct TilePalette {
    // 图案也分两路（草丛 vs 木纹、水波 vs 碎石），跟着配色一起切。
    bool indoor = false;

    engine::Color ground{};
    engine::Color groundDetail{};
    engine::Color road{};
    engine::Color roadDetail{};
    // 特殊地表分两张脸，判据是碰撞层，见 drawTile 的 blocked 参数。
    engine::Color special{};        // 挡路的那一种：水面
    engine::Color specialDetail{};
    engine::Color tilled{};         // 踩得上的那一种：翻土 / 碎石 / 焦土
    engine::Color tilledDetail{};
    engine::Color wall{};
    engine::Color wallSeam{};
    engine::Color wallTop{};
    engine::Color fence{};
    engine::Color fenceDetail{};
    engine::Color flora{};
    engine::Color floraAccent{};
    engine::Color rubble{};
    engine::Color canopy{};        // 带 alpha：底下的人要透得出来
    engine::Color canopyDetail{};
};

// 取配色。判据是地图自己的 outdoor 属性——它一直在 TileMap 里，
// 只是从来没有任何渲染代码看过它。
[[nodiscard]] const TilePalette& tilePalette(bool outdoor);

// 画一格。cell 是屏幕矩形（战斗格 48px、行走格 32px 都走这里），
// (cellX, cellY) 是格坐标，只用来取一个稳定的杂凑：草丛与碎石因此每格长得
// 不同、每帧却一模一样。不要在这里用 rand()——那会让草丛每帧乱跳，
// 而且无头测试无从复现。
//
// blocked 是这一格在碰撞层上通不通，**只有 Special 看它**：
//
//   gid 3 在生成器里同时是水面、药圃翻土与碎石崖面三样东西，一种颜色服侍不了
//   三件事——韩家村那块「菜畦」此前就被画成了一潭蓝汪汪的水。而生成器对水面
//   的写法是固定的：「水面不画墙，只挡路」（genmaps.py 与 genmaps_ch02.py 各
//   一处），于是碰撞层就是那个判据：挡路的是水，踩得上的是地。
//
//   这条区分同时修掉一个更要命的观感：一块看起来能走的地其实走不过去。
void drawTile(engine::Engine& engine, TileRole role, const engine::Rect& cell,
              const TilePalette& palette, int cellX, int cellY, bool blocked);

// 一个「人」。
//
// 没有美术时，方块与人的区别全在轮廓：脚下一抹影子、身子、头，再加一个朝向
// 的小凸起。四件事加起来就够玩家一眼认出这是个活物而不是地上一块砖——而这
// 正是「人物和地图都是方块」这句抱怨的正中间。
struct FigureStyle {
    engine::Color robe{};    // 身
    engine::Color head{};    // 头
    engine::Color trim{};    // 衣领与朝向凸起（外描边是固定的深色，不跟着配色走）
};

// facing 用 core::GameState::facing 那一套：0 上 / 1 右 / 2 下 / 3 左。
// 传 kNoFacing 表示这个单位没有朝向（战斗单位就是），此时不画凸起。
inline constexpr int kNoFacing = -1;
void drawFigure(engine::Engine& engine, const engine::Rect& cell, const FigureStyle& style,
                int facing);

// NPC 的配色由 role_id 的杂凑决定：同一个人每次进图都是同一身衣服，不同的人
// 多半不同色。占位期这条比「三十个 NPC 全是同一个土黄方块」强得多，而它不需要
// 任何新数据——等美术进来，这里换成按 role 查图集即可。
[[nodiscard]] FigureStyle npcStyle(const std::string& roleId);

// 主角。固定一身亮色，且刻意不落在 npcStyle 的色相集合里：
// 屏幕上永远有且只有一个这种颜色的人，那个人就是玩家自己。
[[nodiscard]] const FigureStyle& playerStyle();

// 名牌：深色底条 + 文字。底条是必需的，不是装饰——没有它，浅色地表上的浅色字
// 与深色墙上的深色字都读不出来，而地表颜色由内容决定，改不动。
// centerX 是要对齐的中线，bottomY 是名牌底边。
void drawNamePlate(engine::Engine& engine, const std::string& label, int centerX, int bottomY,
                   int fontSize, const engine::Color& textColor);

// 一个字，画在 cell 的正中。设施与传送点靠它区分彼此。
void drawGlyph(engine::Engine& engine, const std::string& glyph, const engine::Rect& cell,
               int fontSize, const engine::Color& color);

// 设施按 kind 给一个字。九种 kind 见 docs/map_spec.md 第 4.6 节；表外的 kind
// 回显「?」而不是悄悄画成和别人一样的框——一眼看出地图写了个没人认识的 kind。
[[nodiscard]] std::string facilityGlyph(const std::string& kind);

// 设施底板的颜色。同上，表外的 kind 给洋红。
[[nodiscard]] engine::Color facilityColor(const std::string& kind);

}  // namespace fanren::game
