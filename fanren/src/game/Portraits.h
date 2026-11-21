#pragma once
// 对白头像：从头像表里取「这个角色、这个表情」那一格，以及它在对白框里的摆法（伙伴改版，
// docs/interfaces-companions.md 3.9；表的形状见 docs/art-sprites.md 5.3）。
//
// 读在 io 层（io/PortraitLoader.h）；这里管三件事：一局里只读一次表（第一次问到时，PortraitBook）、经引擎载图并核尺寸、
// 摆位的纯函数。DialogueScene 只问 Application::portraits().frameFor 与 portraitLayout，不自己碰表与图。
//
// 「缺图不崩」：表没有、读坏了、图载不进来、格子出了图——一律当作没有头像（对白照旧），各打一行警告。
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "core/Result.h"
#include "core/model/Portraits.h"
#include "engine/Engine.h"
#include "game/SpriteAtlas.h"   // ChapterDone：「第 n 章演完了没有」，世界画面、主菜单、战斗取外观用的同一个判据

namespace fanren::game {

// 表情名 → kPortraitFaces 里的下标；认不出返回 -1。
[[nodiscard]] int portraitFaceSlot(const std::string& face);

// 这一条头像在这一章生效没有：maxChapter 那一章还没演完、且 minChapter 的前一章已经演完（0 = 这一端不限）。
// 与 lookForRole 挑 role_variants 同一种判法（game/SpriteAtlas.cpp），chapterDone 由调用方按章节表的 done_flag 给；
// chapterDone 是空的当作哪一章都没演完。
[[nodiscard]] bool portraitInEffect(const core::PortraitSheet& sheet, const ChapterDone& chapterDone);

// 一个角色的头像列表里，这一章生效的第一条；都不生效返回 nullptr（这一章说话不带头像）。
[[nodiscard]] const core::PortraitSheet* portraitForChapter(const std::vector<core::PortraitSheet>& sheets,
                                                            const ChapterDone& chapterDone);

// 一格头像在图集里的像素矩形。face 认不出时取 normal（调用方负责警告）。
[[nodiscard]] engine::RectF portraitFrameRect(const core::PortraitSheet& sheet, int faceSlot);

struct PortraitFrame {
    engine::TextureId texture = engine::kInvalidTexture;
    engine::RectF src{};
};

// 一局游戏的头像：表与图都从**游戏根**（Application::init 收的那个目录，与 data/、maps/ 同一层）取——
// 表是 <游戏根>/data/visual/portraits.json，图是 <游戏根>/assets/art/portraits/<file>。
// 用 --assets 指到别处时两样跟着一起走：头像与 data 同根（游戏根）；别的美术仍从引擎找到的 assets 目录取。
// Application 持有一份，init 时给根。
//
// 表在第一次真要画头像时才读（headless 时永远不读）；读不到、读坏了都当空表，各打一行警告。
// 警告按这一份去重：同一个问题一局只报一次。
// 同一套「按章取格、缺图不崩」管两份表：对白头像（像素图，最近邻）与女角色的高清立绘（docs/heroine-art-build.md，
// 线性缩放）。两份只差这几样，PortraitBook 构造时给定。
struct SheetKind {
    const char* tableFile;      // <游戏根>/data/visual/<tableFile>
    const char* imageSubdir;    // <游戏根>/assets/art/<imageSubdir>
    engine::ScaleMode scale;
    const char* tag;            // 警告行的前缀：[portrait] / [bust]
    const char* noun;           // 警告里的叫法：头像 / 立绘
    core::Result<core::PortraitTable> (*load)(const std::string& path);
};
extern const SheetKind kPortraitSheets;   // portraits.json、assets/art/portraits、Pixel
extern const SheetKind kBustSheets;       // busts.json、assets/art/busts、Linear

class PortraitBook {
public:
    explicit PortraitBook(const SheetKind& kind = kPortraitSheets) : kind_(&kind) {}

    // 换根：丢掉已读的表与已打过的警告，下次用到时按新根重读。
    void setRoot(const std::string& gameRoot);

    [[nodiscard]] std::string tablePath() const;   // <游戏根>/data/visual/portraits.json（不管在不在）
    [[nodiscard]] std::string imageDir() const;    // <游戏根>/assets/art/portraits

    // 这一句要画的那一格：纹理、源矩形。说话人没登记、这一章没有生效的那一条（portraitForChapter）、
    // 旁白（roleId 空）、图载不进来、格子越出了图 → nullopt。chapterDone 同 portraitForChapter。
    // 表情认不出时退回 normal 并警告一次。headless（没有渲染器）时一律静默返回 nullopt、不读表也不警告：
    // 那时图本来就载不进来，不是图坏了。
    [[nodiscard]] std::optional<PortraitFrame> frameFor(engine::Engine& engine, bool headless,
                                                        const std::string& roleId, const std::string& face,
                                                        const ChapterDone& chapterDone);

    // 当前的表（第一次调时读盘）。给测试与诊断用。
    [[nodiscard]] const core::PortraitTable& table();
    // 表读过没有（无头模式永远不读：测试钉这一条）。
    [[nodiscard]] bool tableLoaded() const { return loaded_; }

private:
    void warnOnce(const std::string& key, const std::string& message);

    const SheetKind* kind_;
    std::string root_;
    bool loaded_ = false;
    core::PortraitTable table_;
    std::set<std::string> warned_;
};

// ---- 摆位（纯函数，不开窗口可测）----
// 头像放在对白框正文区的左侧，按「放得进 kPortraitSlotW × kPortraitSlotH 的最大整数倍」放大
// （定稿头像打包成 128×136 的格子，取 ×1），像素美术不按小数倍缩放。正文往右让开头像宽 + kPortraitGap。
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
