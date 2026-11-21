#include "game/EventCg.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <utility>

namespace fanren::game {

// ---------------------------------------------------------------------------
// 淡入淡出
// ---------------------------------------------------------------------------

void CgStage::advance(const std::string& target, double deltaSeconds) {
    const std::string now = current_ ? current_->id : std::string{};
    if (target != now) {
        // 换了：此刻这一幅转去淡出（顶掉还没淡完的那一层），新的一幅从它淡出时的那一刻起淡入。
        if (current_) outgoing_ = current_;
        current_.reset();
        if (!target.empty()) {
            // 刚淡出一半的那一幅又要回来：从它此刻的透明度接着淡入。
            float from = 0.f;
            if (outgoing_ && outgoing_->id == target) {
                from = outgoing_->alpha;
                outgoing_.reset();
            }
            current_ = Layer{target, from};
        }
    }
    const float step = static_cast<float>(std::max(0.0, deltaSeconds) / kCgFadeSeconds);
    if (current_) current_->alpha = std::min(1.f, current_->alpha + step);
    if (outgoing_) {
        outgoing_->alpha -= step;
        if (outgoing_->alpha <= 0.f) outgoing_.reset();
    }
}

void CgStage::clear() {
    current_.reset();
    outgoing_.reset();
}

std::vector<CgStage::Layer> CgStage::layers() const {
    std::vector<Layer> out;
    if (outgoing_ && outgoing_->alpha > 0.f) out.push_back(*outgoing_);
    if (current_ && current_->alpha > 0.f) out.push_back(*current_);
    return out;
}

// ---------------------------------------------------------------------------
// 表与图
// ---------------------------------------------------------------------------

void CgBook::setRoot(const std::string& gameRoot) {
    root_ = gameRoot;
    loaded_ = false;
    table_.clear();
    warned_.clear();
}

std::string CgBook::tablePath() const {
    return (std::filesystem::path(root_) / "data" / "visual" / "cg.json").string();
}

std::string CgBook::imageDir() const { return (std::filesystem::path(root_) / "assets" / "art" / "cg").string(); }

void CgBook::warnOnce(const std::string& key, const std::string& message) {
    if (!warned_.insert(key).second) return;
    std::fprintf(stderr, "[cg] %s\n", message.c_str());
}

const io::CgTable& CgBook::table() {
    if (loaded_) return table_;
    loaded_ = true;
    const std::string path = tablePath();
    std::error_code ignored;
    if (!std::filesystem::exists(path, ignored)) {
        warnOnce("table", "事件图表 " + path + " 不存在：事件图一律不画");
        return table_;
    }
    core::Result<io::CgTable> read = io::loadCgTable(path);
    if (!read) {
        warnOnce("table", "事件图表读不了，事件图一律不画：" + read.error);
        return table_;
    }
    table_ = std::move(read.value);
    return table_;
}

engine::TextureId CgBook::texture(engine::Engine& engine, bool headless, const std::string& id) {
    if (headless || id.empty()) return engine::kInvalidTexture;
    const io::CgTable& cgs = table();
    const auto it = cgs.find(id);
    if (it == cgs.end()) {
        if (loaded_ && !warned_.count("table")) warnOnce("id:" + id, "事件图 " + id + " 没在 cg.json 里登记：这一幅不画");
        return engine::kInvalidTexture;
    }
    const std::string path = (std::filesystem::path(imageDir()) / it->second).string();
    const engine::TextureId texture = engine.loadTexture(path, engine::ScaleMode::Linear);
    if (texture == engine::kInvalidTexture) {
        warnOnce("file:" + path, "事件图 " + path + " 载不进来：" + id + " 这一幅不画");
    }
    return texture;
}

void drawCgStage(engine::Engine& engine, CgBook& book, bool headless, const CgStage& stage) {
    if (headless || stage.empty()) return;
    const engine::RectF screen{0.f, 0.f, static_cast<float>(engine::kLogicalWidth),
                               static_cast<float>(engine::kLogicalHeight)};
    for (const CgStage::Layer& layer : stage.layers()) {
        const engine::TextureId texture = book.texture(engine, headless, layer.id);
        if (texture == engine::kInvalidTexture) continue;
        const engine::Point size = engine.textureSize(texture);
        engine::DrawOptions options;
        options.tint.a = static_cast<std::uint8_t>(std::lround(std::clamp(layer.alpha, 0.f, 1.f) * 255.f));
        engine.drawTexture(texture, engine::RectF{0.f, 0.f, static_cast<float>(size.x), static_cast<float>(size.y)},
                           screen, options);
    }
}

}  // namespace fanren::game
