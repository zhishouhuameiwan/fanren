#include "game/SpriteAtlas.h"

#include <algorithm>
#include <cmath>

namespace fanren::game {

std::string lookForRole(const SpriteIndex& index, const std::string& roleId,
                        const ChapterDone& chapterDone) {
    if (const auto it = index.variants.find(roleId); it != index.variants.end()) {
        for (const LookVariant& variant : it->second) {
            // max 那一章还没完、且 min 的前一章已经完了（0 = 这一端不限）。
            const bool beforeEnd = variant.maxChapter <= 0 || !chapterDone(variant.maxChapter);
            const bool afterStart = variant.minChapter <= 1 || chapterDone(variant.minChapter - 1);
            if (beforeEnd && afterStart) return variant.look;
        }
    }
    if (const auto it = index.roles.find(roleId); it != index.roles.end()) return it->second;
    return {};
}

const CharacterSheet* sheetForRole(const SpriteIndex& index, const std::string& roleId,
                                   const ChapterDone& chapterDone) {
    const std::string look = lookForRole(index, roleId, chapterDone);
    if (look.empty()) return nullptr;
    const auto it = index.sheets.find(look);
    return it == index.sheets.end() ? nullptr : &it->second;
}

int walkFrame(const SpriteIndex& index, const CharacterSheet& sheet, int facing, bool moving,
              float walkClock) {
    // facing 来自存档（GameState::facing），这里取模而不是直接当下标：一份写坏的存档
    // 不该变成一次越界读。
    const auto dir = static_cast<std::size_t>(((facing % 4) + 4) % 4);
    const std::vector<int>& frames = sheet.walk[dir];
    if (!moving) return frames.front();
    const auto step = static_cast<long long>(std::floor(std::max(0.f, walkClock) * index.walkFps));
    const auto slot = static_cast<std::size_t>(step % static_cast<long long>(index.walkCycle.size()));
    // 下标不越界由读取器（io/VisualLoader.cpp）那条「每一向的帧数多于 walk_cycle 的最大下标」保证。
    return frames[static_cast<std::size_t>(index.walkCycle[slot])];
}

engine::RectF sheetFrameRect(const CharacterSheet& sheet, int frame) {
    const int col = frame % sheet.cols;
    const int row = frame / sheet.cols;
    return engine::RectF{static_cast<float>(col * sheet.frameW), static_cast<float>(row * sheet.frameH),
                         static_cast<float>(sheet.frameW), static_cast<float>(sheet.frameH)};
}

int objectFrame(const ObjectSprite& sprite, float seconds) {
    if (sprite.fps <= 0.f || sprite.frames.size() <= 1) return 0;
    const auto step = static_cast<long long>(std::floor(std::max(0.f, seconds) * sprite.fps));
    return static_cast<int>(step % static_cast<long long>(sprite.frames.size()));
}

engine::RectF objectFrameRect(const ObjectSprite& sprite, int frame) {
    const core::PixelRect& r = sprite.frames[static_cast<std::size_t>(frame)];
    return engine::RectF{static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.w),
                         static_cast<float>(r.h)};
}

}  // namespace fanren::game
