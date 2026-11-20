#pragma once
// 区域地图（第 7 章可玩性改版·第一期「地基包」路 M，docs/ch07-foundation.md 3.2）。
//
// 世界层按 M（手柄 LB，Engine::Key::Map）或主菜单「地图」打开。全屏一块面板，只看当前这张图：
//   左边大图：底图与标记同小地图（game/Minimap.h：宝箱、采集、明雷的显示规矩也相同），设施旁边写名字；
//            图大时方向键平移；
//   右边一栏：危险星级、本图收集进度、图例、出口列表（每道门通向哪张图，通往主线目标的那一道高亮）。
// 世界地图与传送不在本期（3.2）。
//
// 要画的东西全从世界层这一帧拿（WorldScene::describeFrame）：门开没开、哪道门通往目标、宝箱开没开、明雷站在哪，
// 与世界画面同一个答案。压在世界层之上，打开期间世界层不 update（明雷不走、计时不走）。
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "game/Minimap.h"
#include "game/Scene.h"
#include "game/WorldHud.h"

namespace fanren::game {

class AreaMapScene : public Scene {
public:
    AreaMapScene();
    ~AreaMapScene() override;

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;
    [[nodiscard]] std::string name() const override { return "AreaMap"; }

    // 一道出口：通向哪张图（地名）、通不通往主线目标、开没开。同一张图的几道门并成一行。
    struct Exit {
        std::string mapId;
        std::string name;
        bool guide = false;
        bool open = true;
    };
    // 出口表（纯，可测）：按门在图上的次序，同一张目标图只列一次（其中有一道是 guide 就算 guide、有一道开着就算开着）。
    // nameOf 把图 id 换成地名。
    [[nodiscard]] static std::vector<Exit> exitsOf(const WorldFrame& frame,
                                                   const std::function<std::string(const std::string&)>& nameOf);

    // 面板上能出现的固有字（禁词扫描用）。
    [[nodiscard]] static std::vector<std::string> areaMapStrings(const core::GameData& data);

    // 给测试看：此刻的取景、出口表、标记。
    [[nodiscard]] const minimap::Layout& layout() const { return layout_; }
    [[nodiscard]] const std::vector<Exit>& exits() const { return exits_; }
    [[nodiscard]] const std::vector<minimap::Mark>& marks() const { return marks_; }

    // 大图那一块（逻辑像素）。
    [[nodiscard]] static engine::RectF mapRect();

private:
    void refresh(Application& app);

    minimap::Layout layout_;
    bool laidOut_ = false;
    std::vector<Exit> exits_;
    std::vector<minimap::Mark> marks_;
    std::string title_;
    std::string dangerText_;
    std::string exploreText_;
    bool exploreComplete_ = false;
    std::vector<std::pair<std::string, minimap::Mark>> facilityLabels_;
    minimap::MapThumbnail thumb_;
    hud::Geometry geometry_;
};

}  // namespace fanren::game
