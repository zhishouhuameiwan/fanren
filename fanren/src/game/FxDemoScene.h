#pragma once
// 后处理与粒子的自检场景：`--scene fxdemo[:预设]` 压在世界层上拍一张。
//
// 两个用途：
//   1. 截图验收 engine::PostFx 与 engine::ParticleSystem——景深上下糊、灯笼周围亮且泛光、
//      四角压暗、粒子可见，这几样只有真画出来才看得见对不对；
//   2. 下游三路（地图、战斗、界面）照着抄的用法范例：一帧里 PostFx 的调用次序、
//      发光体怎么另画一份、光源怎么登记、粒子怎么分屏幕与世界两种坐标、UI 为什么画在
//      endScene 之后，全在 render() 里按顺序写着。
//
// 预设：night（缺省，全开：夜间环境光 + 灯笼与火盆光源 + 辉光 + 景深 + 暗角 + 调色，
// 薄雾 / 萤火 / 火星）、day（尘埃光点 / 花瓣 / 落叶）、rain、snow。
// 画面里的一切都是运行时画的（渐变、几何、程序像素图），不依赖任何美术产物。
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "engine/Engine.h"
#include "engine/Particles.h"
#include "engine/PostFx.h"
#include "game/Scene.h"

namespace fanren::game {

class FxDemoScene : public Scene {
public:
    // preset 为空等于 "night"。认不出的预设由 knowsPreset 在压栈前拦下。
    explicit FxDemoScene(std::string preset = {});
    ~FxDemoScene() override;

    [[nodiscard]] static bool knowsPreset(const std::string& preset);

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;
    [[nodiscard]] std::string name() const override { return "FxDemoScene"; }

    struct Preset;   // 定义在 .cpp：一张预设表，场景本身不分支

    // 攒一批三角形一次画掉。缓冲每帧复用——景深带、光柱、灯笼这类几何每帧都要重画，
    // 每画一次就新建两个 vector 是下游最容易照抄走的坏习惯。
    class Mesh {
    public:
        void quad(float x0, float y0, float x1, float y1, const engine::Color& top,
                  const engine::Color& bottom);
        void polygon4(const engine::Vertex& a, const engine::Vertex& b, const engine::Vertex& c,
                      const engine::Vertex& d);
        void triangle(const engine::Vertex& a, const engine::Vertex& b, const engine::Vertex& c);
        void fan(float cx, float cy, float rx, float ry, const engine::Color& center,
                 const engine::Color& rim, int segments = 28);
        void draw(engine::Engine& engine, engine::BlendMode blend,
                  engine::TextureId texture = engine::kInvalidTexture);

    private:
        std::vector<engine::Vertex> vertices_;
        std::vector<int> indices_;
    };

private:
    struct Layer {
        engine::ParticleSystem system;
        bool emissive = false;   // 发光的粒子（萤火、火星）再画一份进辉光通道
        bool front = true;       // 画在亭子与人物之前；false 画在远山之后（薄雾）
    };

    void drawBackdrop(engine::Engine& e);
    void drawGround(engine::Engine& e);
    void drawPavilion(engine::Engine& e);
    void drawLanterns(engine::Engine& e);
    void drawBrazier(engine::Engine& e);
    void drawFigures(engine::Engine& e);
    void drawShafts(engine::Engine& e);
    void drawEmissive(engine::Engine& e);
    void addLights();
    void drawOverlay(engine::Engine& e);

    std::string presetName_;
    const Preset* preset_ = nullptr;
    std::unique_ptr<engine::PostFx> fx_;
    std::vector<Layer> layers_;
    std::array<engine::OwnedTexture, 2> figures_;
    Mesh mesh_;
    float time_ = 0.f;
};

}  // namespace fanren::game
