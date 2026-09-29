#pragma once
// 通用界面控件：菜单、背包、商店、图鉴、炼制配方表共用这一套。
//
// 分层约定（interfaces-p2.md 第 4 节）：控件只做「画 + 处理输入 + 报告结果」，
// 一律不碰 GameState。状态变更由 game 层拿到结果后自己执行——否则每加一个
// 面板就要往 ui 层塞一次业务规则，分层就白做了。
//
// ui 只依赖 engine，不依赖 game，因此本层可以在无头模式下被完整单测。
//
// 2026-09-25 八方旅人化改造：外观换成「墨金」（施工图 docs/octopath-overhaul.md 1.6 节），
// 摆位几何（行高、内边距、标题带、列表区）一个数没动——各面板可显示的行数与对话框
// 不出屏由 tests/ListLayoutTests.cpp 钉着，换皮不许换掉它们。token 与各装饰件的用法见
// docs/interfaces-octo-ui.md。
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "engine/Engine.h"

namespace fanren::ui {

// 「墨金」色板的原值（施工图 1.6 节那张表）。Theme 的缺省颜色全部出自这里。
namespace palette {
inline constexpr engine::Color kInk{14, 17, 25, 219};          // #0E1119 α .86 面板底（渐变上端）
inline constexpr engine::Color kInkLow{14, 17, 25, 184};       // 同色 α .72：渐变下端
inline constexpr engine::Color kInkDeep{7, 9, 14, 255};        // #07090E 名牌底、投影、槽
inline constexpr engine::Color kGold{201, 164, 92, 255};       // #C9A45C 外框、角花、选中条
inline constexpr engine::Color kGoldDim{120, 98, 56, 255};     // 内缩 3px 的那道暗金线
inline constexpr engine::Color kGoldBright{242, 217, 139, 255};  // #F2D98B 高亮字、当前项
inline constexpr engine::Color kPaper{241, 234, 216, 255};     // #F1EAD8 正文
inline constexpr engine::Color kPaperDim{156, 148, 128, 255};  // #9C9480 次要字、禁用项
inline constexpr engine::Color kCinnabar{184, 65, 47, 255};    // #B8412F 危险、敌方、破势
inline constexpr engine::Color kJade{95, 174, 140, 255};       // #5FAE8C 气血条、正面状态
inline constexpr engine::Color kAzure{91, 143, 214, 255};      // #5B8FD6 法力条
inline constexpr engine::Color kSheen{255, 246, 222, 70};      // 数值条顶上那道高光
inline constexpr engine::Color kShadow{7, 9, 14, 200};         // 字的投影：压得住花底，又不糊成一团
inline constexpr engine::Color kScrim{7, 9, 14, 120};          // 模态面板底下压的那层纱

// 战斗语义色（终审 LOW-4 从 BattleHud / BattleScene 收上来，数值原样）。压在战斗背景上的小字与飘字
// 比面板字亮一档：面板那几色（kCinnabar、kJade…）落在花底上读不出来，所以是另一组，不是同一组的别名。
inline constexpr engine::Color kBreakRed{255, 120, 96, 255};        // 破势那一句、首领蓄势的标签
inline constexpr engine::Color kPoison{196, 146, 236, 255};         // 中毒：脚下状态字、飘字
inline constexpr engine::Color kPoisonDim{186, 150, 226, 255};      // 队伍面板里「中毒 · 守势」那行小字
inline constexpr engine::Color kFloatCinnabar{232, 96, 72, 255};    // 飘字：蓄势
inline constexpr engine::Color kFloatDim{196, 190, 176, 255};       // 飘字：遁走、脱身
inline constexpr engine::Color kFloatJade{140, 232, 172, 255};      // 飘字：毒解
inline constexpr engine::Color kFloatGuard{150, 200, 250, 255};     // 飘字：守势
inline constexpr engine::Color kChargeBandTop{52, 12, 10, 230};     // 首领蓄势横幅的底（上端）
inline constexpr engine::Color kChargeBandBottom{24, 6, 6, 230};    // 同上（下端）
}  // namespace palette

// 配色与间距集中在这里：将来换皮、做高对比度模式只改一处。
// 控件实现里不许再出现字面量颜色，否则这个集中点就形同虚设。
struct Theme {
    // ---- 契约字段（interfaces-p2.md 第 4 节）----
    // 契约里按用途命名的五个配色别名（panelFill / panelEdge / text / textDim / highlight）
    // 调用方已全部改用下面的墨金 token，2026-09-29 清理时删掉；取色一律用 token。
    int padding = 24;
    int lineSpacing = 8;

    // 以下三项是对契约的追加（追加在末尾，按契约字段顺序的聚合初始化不受影响）：
    // 面板标题与列表行高都要知道字号，而字号和配色一样属于「换皮时一起改」的
    // 东西，散在各控件里迟早走样。
    int bodyFontSize = 22;
    int titleFontSize = 24;
    // 选中行那道渐隐金条左端的透明度（右端淡到零）。放这里而不是写死在控件里，是为了
    // 守住上面那条「控件实现里不许出现字面量颜色」——透明度也是配色的一部分。
    std::uint8_t cursorAlpha = 110;

    // ---- 墨金 token（施工图 1.6 节），2026-09-25 追加在末尾 ----
    engine::Color ink = palette::kInk;
    engine::Color inkLow = palette::kInkLow;
    engine::Color inkDeep = palette::kInkDeep;
    engine::Color gold = palette::kGold;
    engine::Color goldDim = palette::kGoldDim;
    engine::Color goldBright = palette::kGoldBright;
    engine::Color paper = palette::kPaper;
    engine::Color paperDim = palette::kPaperDim;
    engine::Color cinnabar = palette::kCinnabar;
    engine::Color jade = palette::kJade;
    engine::Color azure = palette::kAzure;
    engine::Color sheen = palette::kSheen;
    engine::Color scrim = palette::kScrim;

    // 两种字。正文压在半透明面板上、面板又压在花哨的地图上，一道投影就读得出来；
    // 标题与大字（名签、章节卡、标题画面）再加一圈描边，远看也不糊。
    // 描边用 engine 的真描边（interfaces-octo-engine.md 2.5），不是八方向叠画。
    engine::TextStyle bodyStyle{true, palette::kShadow, 2, false, palette::kInkDeep, 1};
    engine::TextStyle titleStyle{true, palette::kShadow, 2, true, palette::kInkDeep, 2};

    // ---- 战斗语义色（终审 LOW-4），2026-09-26 追加在末尾；取值见 palette 同名常量 ----
    engine::Color breakRed = palette::kBreakRed;
    engine::Color poison = palette::kPoison;
    engine::Color poisonDim = palette::kPoisonDim;
    engine::Color floatCinnabar = palette::kFloatCinnabar;
    engine::Color floatDim = palette::kFloatDim;
    engine::Color floatJade = palette::kFloatJade;
    engine::Color floatGuard = palette::kFloatGuard;
    engine::Color chargeBandTop = palette::kChargeBandTop;
    engine::Color chargeBandBottom = palette::kChargeBandBottom;
};

// 列表控件。菜单、背包、商店、图鉴都用它，不要各写一份。
struct ListItem {
    std::string label;
    std::string detail;      // 右侧次要信息（数量、价格）
    bool enabled = true;

    // 追加字段：禁用理由。配方材料不够时条目要显示但不可选，玩家得看得见
    // 「为什么」，只把字变灰他只会当成 bug。留空则右栏仍显示 detail。
    std::string disabledReason;
};

// 无有效选择。空列表与「全部条目都被禁用」都落在这个值上，
// 调用方只需判一次 < 0，不必分别处理两种空。
inline constexpr int kNoSelection = -1;

// 一页默认显示多少行。调用方应当按自己面板的高度调 setPageSize。
inline constexpr int kDefaultPageSize = 8;

class ListView {
public:
    // ---- 数据 ----
    // 换表后原下标多半越界（背包里刚卖掉一半东西），内部会把 selection
    // 夹回合法范围并吸附到最近的可选项，而不是粗暴归零。
    void setItems(std::vector<ListItem> items);
    void setPageSize(int rows);
    // 空列表时显示的提示。背包空了留一片空白，玩家只会以为界面坏了。
    void setEmptyHint(std::string hint);

    // ---- 输入 ----
    // 返回 true 表示本帧确认了选择。
    bool update(engine::Engine& engine);

    // 逻辑动作与按键解耦：无头测试能直接调，将来接手柄或鼠标也不用改 update。
    // 上下移动回绕（列表短，回绕省按键，也与 P1 对话框选项一致）；
    // 翻页夹紧不回绕（翻页是「扫长表」的动作，跳到另一头只会让人迷失位置）。
    void moveUp();
    void moveDown();
    void pageUp();
    void pageDown();
    // 确认当前选择。无有效选择（空表 / 全禁用）时返回 false 且不置位：
    // 让玩家「确认了一个不存在的东西」比不响应危险得多。
    // 确认与取消互斥：两者都会把对方的标志打掉，否则商店面板可能同时
    // 「关闭」并「买下选中的法器」。
    bool confirm();
    void cancel();

    // ---- 绘制 ----
    void render(engine::Engine& engine, const engine::Rect& area, const Theme& theme) const;

    // 同一张表，但**不画选中行**：菜单里光标还停在左栏时，右栏只是「预览」，
    // 两处都亮着光标的话玩家分不清方向键此刻在推哪一边。
    void renderPreview(engine::Engine& engine, const engine::Rect& area, const Theme& theme) const;

    // render 的纯计算部分：这块区域这一帧实际画得满几行。
    // 单独暴露出来是为了能不开窗口就断言「N 个选项一个都不会被藏起来」——
    // 与 gaugeFillWidth 同一套路数。无头 engine 的 drawText 是空操作，不把这段算式拎出来，
    // 这类漏显示的 bug 就只能靠肉眼在游戏里发现。
    [[nodiscard]] int visibleRows(const engine::Rect& area, const Theme& theme) const;

    // ---- 结果 ----
    [[nodiscard]] int selection() const { return selection_; }
    // 粘滞：一旦按过取消就保持为真，直到 reset()。这样 game 层不必与控件的
    // 帧节奏对齐，可以在本帧任意位置查。
    [[nodiscard]] bool cancelled() const { return cancelled_; }
    // 只反映最近一次 update()，与 update 的返回值同义。
    [[nodiscard]] bool confirmed() const { return confirmed_; }

    [[nodiscard]] int count() const { return static_cast<int>(items_.size()); }
    [[nodiscard]] int pageSize() const { return pageSize_; }
    [[nodiscard]] int topRow() const { return top_; }      // 当前滚动窗口首行
    [[nodiscard]] bool hasSelectable() const;
    [[nodiscard]] const std::vector<ListItem>& items() const { return items_; }
    // 无有效选择时返回 nullptr，省得调用方自己拿 -1 去索引。
    // 指针指进内部的条目数组，下一次 setItems() 就会失效：
    // 只许在同一帧内即取即用，不要跨帧缓存。
    [[nodiscard]] const ListItem* selectedItem() const;

    // 只清交互状态（选择、滚动、确认/取消标志），不动条目。
    // 换数据请用 setItems——两件事分开，面板复用时才不会互相误伤。
    void reset();

private:
    [[nodiscard]] bool selectable(int index) const;
    // 从 from 出发按 step 找下一个可选项。步数硬上界为条目数，
    // 因此「全部禁用」时必然退出而不是空转。
    [[nodiscard]] int nextSelectable(int from, int step) const;
    void selectNearest(int desired);
    void scrollToSelection();
    // render 与 renderPreview 的共同实现；showCursor 为假时选中行照常排，只是不亮。
    void renderRows(engine::Engine& engine, const engine::Rect& area, const Theme& theme,
                    bool showCursor) const;
    // 「第 i / 共 n 项」的位置刻度，只在一页装不下时画。
    void drawScrollMarker(engine::Engine& engine, const engine::Rect& area,
                          const Theme& theme) const;

    std::vector<ListItem> items_;
    std::string emptyHint_ = "（空空如也）";
    int pageSize_ = kDefaultPageSize;
    int selection_ = kNoSelection;
    int top_ = 0;
    bool confirmed_ = false;
    bool cancelled_ = false;
};

// ---- 行高与区域高的换算 ----
// 这三个函数是同一件事的三个方向，摆位时一律走它们，不要各自拿字号加行距去凑。
// 「区域高 = 行数 × 行高」正是 P2 对话框漏显示选项的原因：ListView 自己还要
// 吃掉上下各一个 padding，按裸行高摆位的话三个选项只画得出一个。

// 列表一行占多高（含行距）。
[[nodiscard]] int listRowHeight(const Theme& theme);

// 要完整显示 rows 行，列表区至少得多高（含 ListView 的上下内边距）。
// rows <= 0 返回 0，表示「不需要列表区」，调用方可以直接拿去摆位。
[[nodiscard]] int listAreaHeight(int rows, const Theme& theme);

// 高度为 areaHeight 的列表区画得满几行，listAreaHeight 的反函数。
// 面板按自己的高度设 pageSize 时走这里，免得各写一个魔数。
// 宽度不影响行数，因此只收高度：调用方少构造一个假矩形。
[[nodiscard]] int listRowsThatFit(int areaHeight, const Theme& theme);

// 标题带占掉的高度。panelContentArea 内部用的就是它；面板要在 render 之外
// （比如按面板高度反推页大小）复算正文高度时也用它，两处才不会算成两个数。
[[nodiscard]] int panelTitleBandHeight(bool hasTitle, const Theme& theme);

// 面板：带标题与边框的容器，负责摆位，不管内容。
// 墨金画法：竖向渐变底（上深下浅）+ 1px 金线外框 + 内缩 3px 的 1px 暗金线 +
// 四角小菱形角花；有标题时标题下再一道向右渐隐的金线。
void drawPanel(engine::Engine& engine, const engine::Rect& area, const std::string& title,
               const Theme& theme);

// 面板正文可用的矩形（让出边框、内边距与标题行）。
// 每个面板都要算这一下，集中在这里免得各家算得不一样。
[[nodiscard]] engine::Rect panelContentArea(const engine::Rect& area, const std::string& title,
                                            const Theme& theme);

// 多行文本块，内部走 engine::layoutText，自动断行与禁则。
// 本项目只许有一套断行算法：各面板各写一份必然走样（P1 的对话框已经证明过）。
void drawTextBlock(engine::Engine& engine, const std::string& utf8, const engine::Rect& area,
                   int fontSize, const Theme& theme);

// 数值条（气血、法力、修为进度）：深色底槽 + 竖向渐变的填充 + 顶上一道高光 + 暗金边。
void drawGauge(engine::Engine& engine, const engine::Rect& area, int current, int maximum,
               const engine::Color& fill, const Theme& theme);

// drawGauge 的纯计算部分，单独暴露出来是为了能不开窗口就断言
// 「任何输入都画不出框外」：current 超上限、上限为 0、current 为负都在这里夹死。
[[nodiscard]] int gaugeFillWidth(int width, int current, int maximum);

// ---- 墨金的装饰件（2026-09-25 追加）----
// 面板、列表、对话框、章节卡、标题画面、主菜单都拿这几样拼，不各画一份——
// 角花大一圈、金线淡一档，放在一屏里就是两套皮。

// 1px 描边框（四条边各一笔）。
void drawFrame(engine::Engine& engine, const engine::RectF& rect, const engine::Color& color);

// 实心菱形：(cx, cy) 为中心，radius 为中心到顶点的距离。角花、选中行指针、饰线的结都是它。
void drawDiamond(engine::Engine& engine, float cx, float cy, float radius,
                 const engine::Color& color);

// 选中行：左端一颗金色菱形指针 + 一道向右渐隐的金条。row 是整行的矩形，
// 指针落在它的左边缘上（所以行的左边要留出半个内边距给它）。
void drawSelection(engine::Engine& engine, const engine::Rect& row, const Theme& theme);

// 饰线：中间一颗菱形，两侧金线向外渐隐。章节卡、标题画面、菜单页眉用它。
void drawOrnament(engine::Engine& engine, float centerX, float y, float halfWidth,
                  const Theme& theme);

// 小牌：深底、金边、左右两端各一颗小菱形。对话框的名签都是它。
void drawPlate(engine::Engine& engine, const engine::Rect& area, const Theme& theme);

// 模态面板（修炼、炼制、商店、灵田、告示板）打开时先在整屏压一层纱：面板本身是半透明的，
// 不压的话地图上的名牌、左上角的目标框会从面板底下透上来，与面板上的字叠成一团。
void drawScrim(engine::Engine& engine, const Theme& theme);

// 两头渐隐的暗金细线，横竖各一。面板里「左栏状态、右栏列表」的分界、正文与选项的分界用它：
// 一道到头的实线会把面板切成两块，两头淡掉的线读起来只是「这里换一件事」。
void drawRuleH(engine::Engine& engine, float left, float y, float width, const Theme& theme);
void drawRuleV(engine::Engine& engine, float x, float top, float height, const Theme& theme);

// 拉开字距的一行字，以 centerX 居中。章节卡与标题画面的大字用：六十几像素的方块字
// 不拉开字距就挤成一团。spacing 是字与字之间额外的像素。
void drawSpacedText(engine::Engine& engine, const std::string& utf8, float centerX, int y,
                    int size, const engine::Color& color, const engine::TextStyle& style,
                    int spacing);
[[nodiscard]] int spacedTextWidth(const engine::Engine& engine, const std::string& utf8, int size,
                                  int spacing);

// 两色按比例混合（t = 0 取 a，t = 1 取 b，夹在 [0,1]），alpha 取 a 的。
// 数值条的上亮下暗就是拿填充色往 paper / inkDeep 各拉一截，不另起字面量颜色。
[[nodiscard]] engine::Color mixColor(const engine::Color& a, const engine::Color& b, float t);

// 同一个颜色换一个透明度。
[[nodiscard]] constexpr engine::Color withAlpha(const engine::Color& c, std::uint8_t alpha) {
    return engine::Color{c.r, c.g, c.b, alpha};
}

}  // namespace fanren::ui
