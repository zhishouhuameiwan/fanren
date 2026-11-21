#include "game/Busts.h"

#include <algorithm>
#include <utility>

namespace fanren::game {

void BustStage::speak(const std::optional<BustShot>& shot) {
    if (!shot) {
        // 旁白或没立绘的人：亮着的那位淡出（已经在淡出的那位继续淡出，压在更下面的直接丢掉）。
        if (current_) {
            outgoing_ = std::move(current_);
            current_.reset();
        }
        return;
    }
    if (current_ && current_->shot.role == shot->role) {
        current_->shot = *shot;   // 同一个人接着说：不重新淡入，表情变了直接换格
        return;
    }
    if (outgoing_ && outgoing_->shot.role == shot->role) {
        // 刚淡出一半的那位又开口：从此刻的透明度接着淡入；亮着的那位换去淡出。
        Layer back{*shot, outgoing_->alpha};
        outgoing_ = std::move(current_);
        current_ = std::move(back);
        return;
    }
    if (current_) outgoing_ = std::move(current_);
    current_ = Layer{*shot, 0.f};
}

void BustStage::advance(double deltaSeconds, bool dialogueShown) {
    if (!dialogueShown) {
        absentSeconds_ += deltaSeconds;
        if (absentSeconds_ > kBustLingerSeconds) clear();
        return;
    }
    absentSeconds_ = 0.0;
    const auto step = static_cast<float>(std::max(0.0, deltaSeconds) / kBustFadeSeconds);
    if (current_) current_->alpha = std::min(1.f, current_->alpha + step);
    if (outgoing_) {
        outgoing_->alpha = std::max(0.f, outgoing_->alpha - step);
        if (outgoing_->alpha <= 0.f) outgoing_.reset();
    }
}

void BustStage::clear() {
    current_.reset();
    outgoing_.reset();
    absentSeconds_ = 0.0;
}

std::vector<BustStage::Layer> BustStage::layers() const {
    std::vector<Layer> out;
    if (outgoing_ && outgoing_->alpha > 0.f) out.push_back(*outgoing_);
    if (current_ && current_->alpha > 0.f) out.push_back(*current_);
    return out;
}

const std::string& BustStage::currentRole() const {
    static const std::string kNone;
    return current_ ? current_->shot.role : kNone;
}

engine::Rect bustDialogueArea(const engine::Rect& box, const engine::Rect& bottomBox) {
    const int x = engine::kLogicalWidth - kBustRightMargin - kBustW;
    if (box.y != bottomBox.y) {
        // 框被挪上去了：哪边地方大站哪边。站框下时头顶落在框底以下 kBustHeadGap，下半截出屏。
        const int above = box.y - kBustHeadTop;
        const int below = engine::kLogicalHeight - (box.y + box.h + kBustHeadGap);
        if (above < below) return engine::Rect{x, box.y + box.h + kBustHeadGap - kBustHeadLine, kBustW, kBustH};
    }
    return engine::Rect{x, kBustTopBoxDown, kBustW, kBustH};
}

int bustVisibleRows(const engine::Rect& bust, const engine::Rect& box) {
    const bool boxBelowTop = box.y > bust.y && box.y < bust.y + bust.h;
    const bool overlapsX = box.x < bust.x + bust.w && bust.x < box.x + box.w;
    if (!boxBelowTop || !overlapsX) return bust.h;
    return std::min(bust.h, box.y + kBustTuck - bust.y);
}

engine::Rect bustAvoidHero(const engine::Rect& bust, int rows, const engine::Rect& hero) {
    if (hero.w <= 0 || hero.h <= 0 || rows <= 0) return bust;
    const int top = std::max(bust.y, 0);
    const int bottom = std::min(bust.y + std::min(rows, bust.h), engine::kLogicalHeight);
    const bool hit = hero.x < bust.x + bust.w && bust.x < hero.x + hero.w && hero.y < bottom && top < hero.y + hero.h;
    if (!hit) return bust;
    return engine::Rect{kBustLeftX, bust.y, bust.w, bust.h};
}

engine::RectF bustRosterSource(const engine::RectF& cell) {
    // 840×1260 的格子：头顶线 60，上面留 20；左右不裁。别的尺寸按比例。
    const float top = cell.h * static_cast<float>(kBustHeadLine * 2 - 40) / 1260.f;
    return engine::RectF{cell.x, cell.y + top, cell.w, cell.h - top};
}

engine::RectF bustRosterArea(const engine::Rect& area, const engine::RectF& src) {
    if (src.w <= 0.f || src.h <= 0.f || area.w <= 0 || area.h <= 0) return engine::RectF{};
    float h = static_cast<float>(std::min(kBustRosterH, area.h));
    float w = h * src.w / src.h;
    if (w > static_cast<float>(area.w)) {
        w = static_cast<float>(area.w);
        h = w * src.h / src.w;
    }
    const float x = static_cast<float>(area.x) + (static_cast<float>(area.w) - w) / 2.f;
    const float y = static_cast<float>(area.y + area.h) - h;
    return engine::RectF{x, y, w, h};
}

}  // namespace fanren::game
