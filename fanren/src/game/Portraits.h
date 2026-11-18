#pragma once
// 对白头像：从头像表里取「这个角色、这个表情」那一格，以及它在对白框里的摆法（伙伴改版，
// docs/interfaces-companions.md 3.9；表的形状见 docs/art-sprites.md 5.3）。
//
// 读在 io 层（io/PortraitLoader.h）；这里管三件事：进程里只读一次表（第一次问到时）、经引擎载图并核尺寸、
// 摆位的纯函数。DialogueScene 只问 portraitFor 与 portraitLayout，不自己碰表与图。
//
// 「缺图不崩」：表没有、读坏了、图载不进来、格子出了图——一律当作没有头像（对白照旧），各打一行警告。
#include <optional>
#include <string>

#include "core/model/Portraits.h"
#include "engine/Engine.h"

namespace fanren::game {

// 头像从哪读。两项都空 = 缺省：表是资源根（assets/）上一层的 data/visual/portraits.json，
// 图相对资源根的 art/portraits/。测试拿它指向夹具（setPortraitSource）。
struct PortraitSource {
    std::string tablePath;
    std::string imageDir;
};

// 换来源并丢掉已读的表（下次问到时按新来源重读）。只给测试用；游戏里从不调。
void setPortraitSource(PortraitSource source);

// 当前来源的表在哪（缺省来源经引擎的资源根往上一层找 data/visual/portraits.json）；找不到为空串。
[[nodiscard]] std::string portraitTablePath(engine::Engine& engine);

// 当前来源的头像表（第一次调时读盘；没有文件、读坏了都是空表）。
[[nodiscard]] const core::PortraitTable& portraitTable(engine::Engine& engine);

// 表情名 → kPortraitFaces 里的下标；认不出返回 -1。
[[nodiscard]] int portraitFaceSlot(const std::string& face);

// 一格头像在图集里的像素矩形。face 认不出时取 normal（调用方负责警告）。
[[nodiscard]] engine::RectF portraitFrameRect(const core::PortraitSheet& sheet, int faceSlot);

// 这一句要画的那一格：纹理、源矩形、格子大小。说话人没登记、旁白（roleId 空）、图载不进来、
// 格子越出了图 → nullopt。表情认不出时退回 normal 并警告一次。
struct PortraitFrame {
    engine::TextureId texture = engine::kInvalidTexture;
    engine::RectF src{};
};
[[nodiscard]] std::optional<PortraitFrame> portraitFor(engine::Engine& engine, const std::string& roleId,
                                                       const std::string& face);

// ---- 摆位（纯函数，不开窗口可测）----
// 头像放在对白框正文区的左侧，按「放得进 kPortraitSlotW × kPortraitSlotH 的最大整数倍」放大
// （定稿头像原生约 56×64，取 ×2 = 112×128），像素美术不按小数倍缩放。正文往右让开头像宽 + kPortraitGap。
inline constexpr int kPortraitSlotW = 128;
inline constexpr int kPortraitSlotH = 144;
inline constexpr int kPortraitGap = 20;

[[nodiscard]] int portraitScale(int frameW, int frameH);

struct PortraitLayout {
    engine::Rect portrait{};   // 头像画在这里（屏幕像素，恰好是格子 × 倍数）
    engine::Rect text{};       // 正文让开之后的区域
};
// textArea 是不带头像时交给 ui::drawTextBlock 的那一块，padding 是它四周自己缩进的内边距（Theme::padding）：
// 头像贴着缩进后的左边、在缩进后的那一块里竖直居中（高过它时顶对齐）；让开之后的 text 仍按同一个内边距缩进，
// 于是正文的第一个字落在「头像右边 + kPortraitGap」处。
[[nodiscard]] PortraitLayout portraitLayout(const engine::Rect& textArea, int padding, int frameW, int frameH);

}  // namespace fanren::game
