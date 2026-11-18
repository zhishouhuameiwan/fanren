#include "game/ShotScenes.h"

#include <algorithm>
#include <memory>

#include "game/Application.h"
#include "game/LilianPartyScene.h"
#include "game/MenuScene.h"
#include "game/WorldScene.h"

namespace fanren::game {
namespace {

// 一步走完（滑步 0.12 秒、走路冷却 0.12 秒）再走下一步要的帧数，按 1/60 秒一帧往宽里取。
constexpr int kFramesPerStep = 12;

bool walk(Application& app, const std::string& steps, int& warmup) {
    app.tick(1.0 / 60.0);   // 压进去的世界层帧末才真正入栈
    auto* world = dynamic_cast<WorldScene*>(app.topScene());
    if (world == nullptr) return false;
    for (const char step : steps) {
        int dx = 0;
        int dy = 0;
        switch (step) {
            case 'U': dy = -1; break;
            case 'D': dy = 1; break;
            case 'L': dx = -1; break;
            case 'R': dx = 1; break;
            default: return false;
        }
        (void)world->tryStep(app, dx, dy);
        for (int frame = 0; frame < kFramesPerStep; ++frame) app.tick(1.0 / 60.0);
    }
    warmup = std::max(warmup, kFramesPerStep);
    return true;
}

}  // namespace

bool pushCompanionShot(Application& app, const std::string& kind, const std::string& arg, int& warmup) {
    if (kind == "lilian" && !arg.empty()) {
        app.pushScene(std::make_unique<LilianPartyScene>(arg));
        return true;
    }
    if (kind == "menu" && (arg == "party" || arg == "roster")) {
        app.setFixedWorldClock(kShotMenuWorldClockSeconds);
        app.pushScene(std::make_unique<MenuScene>(arg == "party" ? MenuScene::Page::Party : MenuScene::Page::Roster));
        return true;
    }
    if (kind == "walk" && !arg.empty()) return walk(app, arg, warmup);
    return false;
}

}  // namespace fanren::game
