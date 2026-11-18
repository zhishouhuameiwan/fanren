#include "game/Portraits.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <set>
#include <utility>
#include <vector>

#include "io/PortraitLoader.h"

namespace fanren::game {

void PortraitBook::setRoot(const std::string& gameRoot) {
    root_ = gameRoot;
    loaded_ = false;
    table_.clear();
    warned_.clear();
}

std::string PortraitBook::tablePath() const {
    return (std::filesystem::path(root_) / "data" / "visual" / "portraits.json").string();
}

std::string PortraitBook::imageDir() const {
    return (std::filesystem::path(root_) / "assets" / "art" / "portraits").string();
}

void PortraitBook::warnOnce(const std::string& key, const std::string& message) {
    if (!warned_.insert(key).second) return;
    std::fprintf(stderr, "[portrait] WARNING: %s\n", message.c_str());
}

const core::PortraitTable& PortraitBook::table() {
    if (loaded_) return table_;
    loaded_ = true;
    const std::string path = tablePath();
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        warnOnce("table", "找不到 " + path + "：对白一律不带头像");
        return table_;
    }
    auto read = io::loadPortraits(path);
    if (!read) {
        warnOnce("table", read.error + "：对白一律不带头像");
        return table_;
    }
    table_ = std::move(read.value);
    return table_;
}

int portraitFaceSlot(const std::string& face) {
    for (std::size_t i = 0; i < core::kPortraitFaces.size(); ++i) {
        if (face == core::kPortraitFaces[i]) return static_cast<int>(i);
    }
    return -1;
}

engine::RectF portraitFrameRect(const core::PortraitSheet& sheet, int faceSlot) {
    const std::size_t slot = faceSlot >= 0 && static_cast<std::size_t>(faceSlot) < sheet.faces.size()
                                 ? static_cast<std::size_t>(faceSlot)
                                 : 0;
    const int frame = sheet.faces[slot];
    const int cols = std::max(1, sheet.cols);
    return engine::RectF{static_cast<float>((frame % cols) * sheet.frameW),
                         static_cast<float>((frame / cols) * sheet.frameH), static_cast<float>(sheet.frameW),
                         static_cast<float>(sheet.frameH)};
}

std::optional<PortraitFrame> PortraitBook::frameFor(engine::Engine& engine, bool headless, const std::string& roleId,
                                                    const std::string& face) {
    if (headless) return std::nullopt;         // 没有渲染器：图本来就载不进来，不当毛病报
    if (roleId.empty()) return std::nullopt;   // 旁白
    const core::PortraitTable& table = this->table();
    const auto it = table.find(roleId);
    if (it == table.end()) return std::nullopt;   // 没登记：照旧不带头像，不算毛病
    const core::PortraitSheet& sheet = it->second;

    int slot = portraitFaceSlot(face);
    if (slot < 0) {
        warnOnce("face:" + face, "认不出的表情「" + face + "」（只有 normal / smile / angry / hurt）：按 normal 画");
        slot = 0;
    }
    const std::string path = (std::filesystem::path(imageDir()) / sheet.file).string();
    const engine::TextureId texture = engine.loadTexture(path, engine::ScaleMode::Pixel);
    if (texture == engine::kInvalidTexture) {
        warnOnce("file:" + path, "头像图 " + path + " 载不进来：" + roleId + " 的对白不带头像");
        return std::nullopt;
    }
    const engine::RectF src = portraitFrameRect(sheet, slot);
    const engine::Point size = engine.textureSize(texture);
    if (src.x + src.w > static_cast<float>(size.x) || src.y + src.h > static_cast<float>(size.y)) {
        warnOnce("rect:" + roleId + ":" + std::to_string(slot),
                 "头像 " + roleId + " 的 " + core::kPortraitFaces[static_cast<std::size_t>(slot)] +
                     " 那一格出了图 " + path + "：这句不带头像");
        return std::nullopt;
    }
    return PortraitFrame{texture, src};
}

int portraitScale(int frameW, int frameH) {
    if (frameW <= 0 || frameH <= 0) return 1;
    return std::max(1, std::min(kPortraitSlotW / frameW, kPortraitSlotH / frameH));
}

PortraitLayout portraitLayout(const engine::Rect& textArea, int padding, int frameW, int frameH) {
    const int scale = portraitScale(frameW, frameH);
    const int w = std::max(0, frameW) * scale;
    const int h = std::max(0, frameH) * scale;
    const int innerH = textArea.h - padding * 2;
    PortraitLayout layout;
    layout.portrait = engine::Rect{textArea.x + padding, textArea.y + padding + std::max(0, (innerH - h) / 2), w, h};
    const int shift = w + kPortraitGap;
    layout.text = engine::Rect{textArea.x + shift, textArea.y, std::max(0, textArea.w - shift), textArea.h};
    return layout;
}

}  // namespace fanren::game
