#pragma once
// 事件图（第 7 章同道录 2a·路 U，施工图 docs/ch07-tongdaolu.md 3.5；施工记录 docs/ch07-tongdaolu-u.md 第 5 节）。
//
// 脚本 show_cg(id) / hide_cg() 只改 Application::currentCg()（路 K）；这里管两样：
//   · CgStage：画面上的淡入淡出（0.3 秒）。换图时旧的淡出、新的淡入，两层交叉；hide 之后淡出完才清。
//   · CgBook：表 <游戏根>/data/visual/cg.json 与图 <游戏根>/assets/art/cg/<file>（2560×1440，屏幕上画成 1280×720，
//     线性缩放）。第一次真要画时才读；表不存在、读坏了、没登记、图载不进来——一律当作没有这幅图（照样演对白），
//     各警告一次。无头模式不读表、不载图、不警告。
// 画在哪一层：紧贴着底下那一层（世界层）画，对话框压在它上面（Application::drawScenes）。
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "engine/Engine.h"
#include "io/CgLoader.h"

namespace fanren::game {

inline constexpr double kCgFadeSeconds = 0.3;

class CgStage {
public:
    struct Layer {
        std::string id;
        float alpha = 0.f;   // 0..1
    };
    // 每帧走一次：target 是此刻该显示的那一幅（空 = 不显示）。
    void advance(const std::string& target, double deltaSeconds);
    void clear();
    // 要画的几层，先画的在前（淡出的在下）；透明度为 0 的不列。
    [[nodiscard]] std::vector<Layer> layers() const;
    [[nodiscard]] bool empty() const { return !current_ && !outgoing_; }

private:
    std::optional<Layer> current_;
    std::optional<Layer> outgoing_;
};

class CgBook {
public:
    // 换根：丢掉已读的表与已打过的警告。
    void setRoot(const std::string& gameRoot);
    [[nodiscard]] std::string tablePath() const;   // <游戏根>/data/visual/cg.json
    [[nodiscard]] std::string imageDir() const;    // <游戏根>/assets/art/cg

    // 这一幅的纹理；没有（见文件头）返回 kInvalidTexture。headless 时直接返回、不读表。
    [[nodiscard]] engine::TextureId texture(engine::Engine& engine, bool headless, const std::string& id);
    [[nodiscard]] bool tableLoaded() const { return loaded_; }

private:
    void warnOnce(const std::string& key, const std::string& message);
    const io::CgTable& table();

    std::string root_;
    bool loaded_ = false;
    io::CgTable table_;
    std::set<std::string> warned_;
};

// 把台上的几层画满全屏（1280×720）。没有纹理的那一层跳过。
void drawCgStage(engine::Engine& engine, CgBook& book, bool headless, const CgStage& stage);

}  // namespace fanren::game
