#pragma once
// 女角色的高清立绘（docs/heroine-art-build.md 第 2、3 节）：对白时站在对话框后面的右侧，队伍页放大图。
//
// 表与图由 PortraitBook（game/Portraits.h，配 kBustSheets）管：表 <游戏根>/data/visual/busts.json，
// 图 <游戏根>/assets/art/busts/，线性缩放载入，按章生效、缺图坏表当作没有立绘——与对白头像同一套口径。
// 这里管两样不碰表与图的事：
//   · BustStage：一段对白里立绘的淡入淡出。对话框是一句一个 DialogueScene，换句时中间还空一帧，
//     「连着两句都是她不重新淡入」「换人时她淡出」这些只能记在比一句更长命的地方——Application 持有一份。
//   · 摆位的纯函数：框在下、框被挪到上面两种。
#include <optional>
#include <string>
#include <vector>

#include "engine/Engine.h"

namespace fanren::game {

// 一句话要画的那一格立绘。
struct BustShot {
    std::string role;   // 角色 id：同一个人换表情不重新淡入，靠它认
    engine::TextureId texture = engine::kInvalidTexture;
    engine::RectF src{};
};

// ---- 淡入淡出 ----
// 规则（3.1）：换成她说话时 0.15 秒淡入；换成别人说话或旁白时 0.15 秒淡出；连着两句都是她不重新淡入，
// 表情变了直接换格。两个都有立绘的人前后说话：前一个淡出、后一个淡入，两层叠着交叉。
// 刚淡出一半的那个人又开口：从她此刻的透明度接着淡入，不从零开始。
// 对白整个收起（栈里没有对话框）超过 kBustLingerSeconds 就清空：下一段戏她再开口重新淡入。
inline constexpr double kBustFadeSeconds = 0.15;
inline constexpr double kBustLingerSeconds = 0.25;

class BustStage {
public:
    struct Layer {
        BustShot shot;
        float alpha = 0.f;   // 0..1
    };

    // 新的一句开始：shot 是这一句说话人的立绘，没有（旁白、没立绘的人、缺图）给 nullopt。
    void speak(const std::optional<BustShot>& shot);
    // 走时间。dialogueShown：此刻栈里有没有对话框。有 → 淡入淡出照走；没有 → 累计空着的时长，过了
    // kBustLingerSeconds 清空（换句时那一帧空档不清）。
    void advance(double deltaSeconds, bool dialogueShown);
    void clear();

    // 要画的几层，先画的在前（淡出的那一层在下，淡入的在上）；透明度为 0 的不列。
    [[nodiscard]] std::vector<Layer> layers() const;
    // 正在淡入或已经亮着的那个人（没有为空串）。
    [[nodiscard]] const std::string& currentRole() const;
    [[nodiscard]] bool empty() const { return !current_ && !outgoing_; }

private:
    std::optional<Layer> current_;    // 这一句的说话人：往 1 走
    std::optional<Layer> outgoing_;   // 上一个说话人：往 0 走
    double absentSeconds_ = 0.0;
};

// ---- 摆位（纯函数）----
// 屏幕上画成 420×630（表里存两倍 840×1260，窗口放大到两倍时仍然清楚），站在右侧、右边距 60。
// 头顶线：格子里头顶所在的高度，屏幕上 kBustHeadLine = 30（tools/artgen/busts.py 的 HEAD_LINE 是它的两倍，60）；
// 人物从头顶到格底在屏幕上高 600（施工图 3.5.1）。
//   · 框在底部（常态）：格子顶边在 y = kBustTopBoxDown（18），头顶约在 y = 48；只画到框顶金线下 kBustTuck（框半透明，
//     框后那截不画），露出头到腰胯。框因为选项多长高了也不跟着往上抬，多出来的那截由框压住。
//   · 框被 keepHeroInSight 挪上去：底边贴屏幕底；但头顶至少落在框底以下 kBustHeadGap（不叫半透明的框压住脸），
//     这时下半截出屏。头顶线 30 时框只要底边低于 108 就是后一种，所以框贴到顶边（底边 248）时也是头顶在框下、下半截出屏。
//   · 主角的屏幕矩形与右侧立绘画出来的那一块相交：改站左侧，格子左边 x = kBustLeftX（160），不镜像（施工图 3.5.2）。
inline constexpr int kBustW = 420;
inline constexpr int kBustH = 630;
inline constexpr int kBustRightMargin = 60;
inline constexpr int kBustTopBoxDown = 18;
inline constexpr int kBustHeadLine = 30;
inline constexpr int kBustHeadGap = 12;
inline constexpr int kBustLeftX = 160;

// box：这一帧对话框实际画的位置；bottomBox：没挪之前它该在的位置（boxArea）。站右侧。
[[nodiscard]] engine::Rect bustDialogueArea(const engine::Rect& box, const engine::Rect& bottomBox);
// 立绘画出来的行数（从顶上数）：框在底部时画到框顶那道金线下 kBustTuck 为止（框半透明，框后那截不画）；
// 框被挪到上面（raised）时整幅都画。
inline constexpr int kBustTuck = 3;
[[nodiscard]] int bustVisibleRows(const engine::Rect& bust, const engine::Rect& box, bool raised);
// 躲主角：bust 画出来的那一块（顶上 rows 行，裁到屏幕里）与 hero 相交就改站左侧；hero 为空矩形（没有世界层）时不动。
[[nodiscard]] engine::Rect bustAvoidHero(const engine::Rect& bust, int rows, const engine::Rect& hero);

// 队伍页的大图：整个人（头到格底）——格子上面裁到头顶线以上 20、左右不裁（最宽的一位约 827，不能裁），按 840×1260 的比例算；
// 在 area（页面里留给大图的那一栏）里按它的宽高比放到高 kBustRosterH，底边贴 area 底、水平居中；放不下就按 area 缩。
inline constexpr int kBustRosterH = 560;
[[nodiscard]] engine::RectF bustRosterSource(const engine::RectF& cell);
[[nodiscard]] engine::RectF bustRosterArea(const engine::Rect& area, const engine::RectF& src);

}  // namespace fanren::game
