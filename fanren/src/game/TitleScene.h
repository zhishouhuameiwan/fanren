#pragma once
// 标题画面（施工图 docs/octopath-overhaul.md 第 4 节）：不带 --load / --map 启动时的第一个场景。
//
// 背景：assets/art/title/{sky,far,mid,near}.png 四层（320×180，×4 铺满），sky / far / mid
// 横向首尾相接、各按自己的速度缓慢漂移，near 不动。**四张图缺任何一张**就整套换成程序画法：
// 渐变晨天、日晕、两重远山剪影、云海、几座近峰——那是完整的第二种画法，不是占位。
// 上面再压薄雾与少量光点粒子，经 PostFx 做景深、辉光与暗角。
//
// 菜单：新的旅程 / 继续旅程（saves/quick.sav 在才可选）/ 设置 / 离开。
//   新的旅程 → 淡出 → Application::startNewJourney（第一章开篇卡 → 韩家村）；
//   继续旅程 → Application::continueJourney，读档失败**如实说出来、留在标题画面**，
//              不静默新开一局（saves/README.md 的约定）；
//   设置     → 压系统设置面板（SettingsScene，与主菜单共用，docs/settings.md 5.1）。
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "engine/Engine.h"
#include "engine/Particles.h"
#include "engine/PostFx.h"
#include "game/Scene.h"
#include "ui/Widgets.h"

namespace fanren::game {

class TitleScene : public Scene {
public:
    // 菜单四项的下标（docs/settings.md 5.1：「设置」插在「离开」之前）。
    static constexpr int kNewJourney = 0;
    static constexpr int kContinue = 1;
    static constexpr int kSettings = 2;
    static constexpr int kQuit = 3;
    static constexpr int kItemCount = 4;

    // 开场从黑里淡出来多久、选了「新的旅程」之后淡入黑多久（秒）。
    static constexpr double kFadeInSeconds = 1.2;
    static constexpr double kLeaveSeconds = 0.6;

    TitleScene();
    ~TitleScene() override;

    // 视差层 t 秒时的横向偏移：speed 像素/秒，按 period 取模，落在 [0, period)。
    // 首尾相接的图画两遍（x 与 x + period）就无缝。纯函数，测试钉着「不会漂出一个周期」。
    [[nodiscard]] static float layerOffset(float speed, double t, float period);

    // 标题画面菜单上可能出现的全部固有字（四项、置灰理由、读档失败的前缀、按键提示）。
    // 与 onEnter / render 取的是同一批 key，tests/UiStyleTests.cpp 拿禁词表扫它。
    // 不含书名「凡人修仙传」：那是书名，不是界面用词，本来就带「修仙」二字。
    [[nodiscard]] static std::vector<std::string> menuStrings(const core::GameData& data);

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    [[nodiscard]] bool opaque() const override { return true; }
    [[nodiscard]] std::string name() const override { return "Title"; }

private:
    void drawArtLayers(engine::Engine& e);
    void drawProceduralLayers(engine::Engine& e);
    void drawEmissive(engine::Engine& e);
    void drawMenu(Application& app);
    void fan(float cx, float cy, float rx, float ry, const engine::Color& center,
             const engine::Color& rim);
    void flush(engine::Engine& e, engine::BlendMode blend);

    std::array<engine::TextureId, 4> art_{};   // sky / far / mid / near
    bool artReady_ = false;
    std::unique_ptr<engine::PostFx> fx_;
    engine::ParticleSystem mist_;
    engine::ParticleSystem motes_;
    ui::ListView menu_;
    std::string error_;
    double time_ = 0.0;
    double leaving_ = -1.0;   // ≥ 0：选了「新的旅程」，正在淡入黑，已经过了多久
    // 攒一批三角形一次画掉；缓冲每帧复用，不每画一个日晕就新建两个 vector。
    std::vector<engine::Vertex> vertices_;
    std::vector<int> indices_;
};

}  // namespace fanren::game
