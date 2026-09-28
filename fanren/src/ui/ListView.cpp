#include "ui/Widgets.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "engine/TextLayout.h"

namespace fanren::ui {
namespace {

// 把可能溢出的目标下标安全地夹回合法区间。
// 翻页的 base ± pageSize 在 pageSize 取极端值时会让 int 加法溢出（UB），
// 先抬到 64 位再夹，和 gaugeFillWidth 是同一套防线。
[[nodiscard]] int clampIndex(long long desired, int count) {
    if (count <= 0) return kNoSelection;
    const long long clamped = std::clamp<long long>(desired, 0, static_cast<long long>(count) - 1);
    return static_cast<int>(clamped);
}

// 右栏（数量、价格、禁用理由），右边缘对齐到 right。
// 放得下就与正文同字号；与左边的名字撞上了就退一号字；再放不下就从尾巴上截、补一个「…」。
// 从前是一律右对齐、左边夹到正文起点——修炼面板那句「卡在这一层了，火候再足也推不上去」
// 于是整句压在「试下一层」四个字上，两句都读不出来。
void drawRightColumn(engine::Engine& engine, const std::string& text, int labelEnd, int right,
                     int y, const Theme& theme) {
    constexpr int kGap = 16;   // 名字与右栏之间至少留这么宽
    constexpr int kStepDown = 6;
    const int room = right - (labelEnd + kGap);
    const int full = engine.measureText(text, theme.bodyFontSize).x;
    if (full <= room) {
        engine.drawText(text, right - full, y, theme.bodyFontSize, theme.paperDim, theme.bodyStyle);
        return;
    }
    const int small = std::max(1, theme.bodyFontSize - kStepDown);
    std::string fitted = text;
    int width = engine.measureText(fitted, small).x;
    if (width > room) {
        std::vector<std::string> glyphs = engine::splitGraphemes(text);
        while (width > room && !glyphs.empty()) {
            glyphs.pop_back();
            fitted.clear();
            for (const std::string& glyph : glyphs) fitted += glyph;
            fitted += "…";
            width = engine.measureText(fitted, small).x;
        }
        if (glyphs.empty()) return;   // 一个字都放不下：宁可不画，也不压在名字上
    }
    engine.drawText(fitted, right - width, y + (theme.bodyFontSize - small) / 2 + 1, small,
                    theme.paperDim, theme.bodyStyle);
}

}  // namespace

bool ListView::selectable(int index) const {
    return index >= 0 && index < count() && items_[static_cast<std::size_t>(index)].enabled;
}

bool ListView::hasSelectable() const {
    return std::any_of(items_.begin(), items_.end(),
                       [](const ListItem& item) { return item.enabled; });
}

const ListItem* ListView::selectedItem() const {
    if (!selectable(selection_)) return nullptr;
    return &items_[static_cast<std::size_t>(selection_)];
}

int ListView::nextSelectable(int from, int step) const {
    const int n = count();
    if (n == 0) return kNoSelection;

    // 最多走 n 步：正好绕回 from，所以「只有一项可选」时会停在原地，
    // 「一项都不可选」时走满 n 步后老实返回 kNoSelection 而不是空转。
    int index = from;
    for (int guard = 0; guard < n; ++guard) {
        index += step;
        if (index < 0 || index >= n) {
            index = ((index % n) + n) % n;
        }
        if (items_[static_cast<std::size_t>(index)].enabled) return index;
    }
    return kNoSelection;
}

void ListView::selectNearest(int desired) {
    const int n = count();
    if (n == 0) {
        selection_ = kNoSelection;
        top_ = 0;
        return;
    }
    const int start = std::clamp(desired, 0, n - 1);
    if (items_[static_cast<std::size_t>(start)].enabled) {
        selection_ = start;
        scrollToSelection();
        return;
    }
    // 先往后找再往前找：翻页与换表都是「往下走」的语境，
    // 落在禁用项上时顺势继续往下比倒回去更符合手感。
    for (int i = start + 1; i < n; ++i) {
        if (items_[static_cast<std::size_t>(i)].enabled) {
            selection_ = i;
            scrollToSelection();
            return;
        }
    }
    for (int i = start - 1; i >= 0; --i) {
        if (items_[static_cast<std::size_t>(i)].enabled) {
            selection_ = i;
            scrollToSelection();
            return;
        }
    }
    selection_ = kNoSelection;   // 全部禁用
    top_ = 0;
}

void ListView::scrollToSelection() {
    const int n = count();
    const int rows = std::max(1, pageSize_);
    if (n <= rows || selection_ == kNoSelection) {
        top_ = 0;   // 一页装得下就没有滚动可言，永远从第一行画起
        return;
    }
    // 只在选中项跑出窗口时才挪窗口，逐格移动时列表不会整屏乱跳。
    top_ = std::clamp(top_, selection_ - rows + 1, selection_);
    top_ = std::clamp(top_, 0, n - rows);
}

void ListView::setItems(std::vector<ListItem> items) {
    const int previous = selection_;
    items_ = std::move(items);
    selectNearest(previous == kNoSelection ? 0 : previous);
    // confirmed_ 与 cancelled_ 刻意不清：它们描述的是「上一次输入」而不是
    // 「这批数据」。背包、商店这类面板每帧都会 setItems 重建列表，
    // 在这里清标志会让玩家按下的取消键被自己吃掉。要清请用 reset()。
}

void ListView::setPageSize(int rows) {
    pageSize_ = std::max(1, rows);   // 0 与负数没有定义良好的含义，按一行处理
    scrollToSelection();
}

void ListView::setEmptyHint(std::string hint) {
    emptyHint_ = std::move(hint);
}

void ListView::moveDown() {
    if (selection_ == kNoSelection) {
        selectNearest(0);
        return;
    }
    const int next = nextSelectable(selection_, +1);
    if (next == kNoSelection) return;
    selection_ = next;
    scrollToSelection();
}

void ListView::moveUp() {
    if (selection_ == kNoSelection) {
        selectNearest(count() - 1);
        return;
    }
    const int next = nextSelectable(selection_, -1);
    if (next == kNoSelection) return;
    selection_ = next;
    scrollToSelection();
}

void ListView::pageDown() {
    // 整张表就一页时没有「下一页」，翻页键应当毫无动静。
    // 若改成夹到末项，三行的小菜单里按一下右键会莫名跳到最后一行。
    if (count() <= pageSize_) return;
    const long long base = (selection_ == kNoSelection) ? 0 : selection_;
    selectNearest(clampIndex(base + pageSize_, count()));
}

void ListView::pageUp() {
    if (count() <= pageSize_) return;
    const long long base = (selection_ == kNoSelection) ? 0 : selection_;
    selectNearest(clampIndex(base - pageSize_, count()));
}

// 确认与取消互斥。两个标志同时为真时，上层会既关掉面板又执行选中项，
// 在商店/炼制面板上就是「关窗口的同时把材料烧了」。
bool ListView::confirm() {
    if (selection_ == kNoSelection) return false;
    confirmed_ = true;
    cancelled_ = false;
    return true;
}

void ListView::cancel() {
    cancelled_ = true;
    confirmed_ = false;
}

void ListView::reset() {
    confirmed_ = false;
    cancelled_ = false;
    top_ = 0;
    selectNearest(0);
}

bool ListView::update(engine::Engine& engine) {
    using Key = engine::Engine::Key;
    confirmed_ = false;   // 「本帧是否确认」，不是粘滞状态

    // 无头模式里 keyPressed 恒为 false，于是整个 update 退化成空操作：
    // 界面逻辑因此能被无头测试驱动，而不必伪造一套输入设备。
    if (engine.keyPressed(Key::Up)) moveUp();
    if (engine.keyPressed(Key::Down)) moveDown();
    if (engine.keyPressed(Key::Left)) pageUp();
    if (engine.keyPressed(Key::Right)) pageDown();

    // 取消优先于确认：同一帧两个键都下来时，退出比误买一件法器安全。
    if (engine.keyPressed(Key::Cancel)) {
        cancel();
        return false;
    }
    if (engine.keyPressed(Key::Confirm)) {
        return confirm();
    }
    return false;
}

int ListView::visibleRows(const engine::Rect& area, const Theme& theme) const {
    if (area.w <= 0 || area.h <= 0) return 0;
    // 宽度不够放内边距时整块区域都没法用：与 render 的前置判断保持一致，
    // 否则会出现「这个函数说画得下三行、render 却一行都没画」。
    if (area.w - theme.padding * 2 <= 0) return 0;
    return std::min(std::min(pageSize_, listRowsThatFit(area.h, theme)), count());
}

void ListView::render(engine::Engine& engine, const engine::Rect& area, const Theme& theme) const {
    renderRows(engine, area, theme, /*showCursor=*/true);
}

void ListView::renderPreview(engine::Engine& engine, const engine::Rect& area,
                             const Theme& theme) const {
    renderRows(engine, area, theme, /*showCursor=*/false);
}

void ListView::renderRows(engine::Engine& engine, const engine::Rect& area, const Theme& theme,
                          bool showCursor) const {
    if (area.w <= 0 || area.h <= 0) return;

    const int innerX = area.x + theme.padding;
    const int innerY = area.y + theme.padding;
    const int innerW = area.w - theme.padding * 2;
    const int innerH = area.h - theme.padding * 2;
    if (innerW <= 0 || innerH <= 0) return;

    if (items_.empty()) {
        engine.drawText(emptyHint_, innerX, innerY, theme.bodyFontSize, theme.paperDim,
                        theme.bodyStyle);
        return;
    }

    const int rowHeight = listRowHeight(theme);
    const int rows = visibleRows(area, theme);
    if (rows <= 0) return;

    // top_ 由输入逻辑维护，但调用方给的区域可能比 pageSize 还矮。
    // 这里再算一次起始行，保证选中项无论如何都在画面内。
    // render 是 const，因此只改局部量，不回写 top_。
    const int maxStart = std::max(0, count() - rows);
    int start = std::clamp(top_, 0, maxStart);
    if (selection_ != kNoSelection) {
        if (selection_ < start) {
            start = selection_;
        } else if (selection_ >= start + rows) {
            start = selection_ - rows + 1;
        }
        start = std::clamp(start, 0, maxStart);
    }

    int y = innerY;
    for (int i = start; i < start + rows && i < count(); ++i) {
        const ListItem& item = items_[static_cast<std::size_t>(i)];
        const bool selected = showCursor && (i == selection_);

        if (selected) {
            // 光标用「菱形指针 + 渐隐金条」而不只是变字色：禁用项本来就是暗的，
            // 只靠字色玩家分不清「选中的禁用项」和「没选中的可用项」，
            // 而配方表里这两种行天天挨在一起。
            // 行距大于内边距的皮肤会把第一行的光标条顶出区域上沿，夹一下。
            const int barY = std::max(area.y, y - theme.lineSpacing / 2);
            drawSelection(engine,
                          engine::Rect{area.x + theme.padding / 2, barY, area.w - theme.padding,
                                       rowHeight},
                          theme);
        }

        const engine::Color labelColor =
            !item.enabled ? theme.paperDim : (selected ? theme.goldBright : theme.paper);
        engine.drawText(item.label, innerX, y, theme.bodyFontSize, labelColor, theme.bodyStyle);

        // 禁用时右栏改写成禁用理由：玩家要看得出「不是坏了，是材料不够」。
        const std::string& right =
            (!item.enabled && !item.disabledReason.empty()) ? item.disabledReason : item.detail;
        if (!right.empty()) {
            const int labelEnd = innerX + engine.measureText(item.label, theme.bodyFontSize).x;
            drawRightColumn(engine, right, labelEnd, area.x + area.w - theme.padding, y, theme);
        }
        y += rowHeight;
    }

    // 长表要给个「我在哪」的刻度，否则滚过几屏之后玩家完全没有位置感。
    if (count() > rows) {
        drawScrollMarker(engine, area, theme);
    }
}

void ListView::drawScrollMarker(engine::Engine& engine, const engine::Rect& area,
                                const Theme& theme) const {
    const int ordinal = (selection_ == kNoSelection) ? 0 : selection_ + 1;
    const std::string marker = std::to_string(ordinal) + "/" + std::to_string(count());
    const int markerW = engine.measureText(marker, theme.bodyFontSize).x;
    const int markerX = area.x + area.w - theme.padding - markerW;
    const int markerY = area.y + area.h - theme.padding - theme.bodyFontSize;
    if (markerY >= area.y + theme.padding) {
        engine.drawText(marker, std::max(area.x + theme.padding, markerX), markerY,
                        theme.bodyFontSize, theme.paperDim, theme.bodyStyle);
    }
}

}  // namespace fanren::ui
