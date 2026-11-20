#include "game/ShotScenes.h"

#include <cstdio>
#include <memory>

#include "core/rules/Roster.h"
#include "game/Application.h"
#include "game/LilianPartyScene.h"
#include "game/MenuScene.h"
#include "game/WorldScene.h"

namespace fanren::game {
namespace {

// 照游戏一帧的次序走一帧：update（WorldScene 推画面、受理输入），再 render（世界逻辑填帧、世界画面排跟随者）。
// 截图口不走 run()，跟随者的滑步要在 render 里才起步，只 tick 不画的话拍照那一刻他们还没出发。
void frame(Application& app) {
    app.tick(1.0 / 60.0);
    app.drawScenes();
}

bool settled(const WorldView& view) {
    if (view.playerMoving()) return false;
    for (const FollowerTrack::Slot& slot : view.followers()) {
        if (slot.slide.moving()) return false;
    }
    return true;
}

// 一步至多等多少帧滑完（一步 0.12 秒、约 8 帧；这里只是防死循环的上限，不是步长本身）。
constexpr int kMaxFramesPerStep = 120;

WorldScene* topWorld(Application& app) { return dynamic_cast<WorldScene*>(app.topScene()); }

bool walk(Application& app, const std::string& steps) {
    frame(app);   // 压进去的世界层帧末才真正入栈
    for (const char step : steps) {
        int dx = 0;
        int dy = 0;
        switch (step) {
            case 'U': dy = -1; break;
            case 'D': dy = 1; break;
            case 'L': dx = -1; break;
            case 'R': dx = 1; break;
            case 'Z':
            case '.':
                break;
            default:
                std::fprintf(stderr, "walk: 认不出的方向 %c（只认 U D L R，与按确认 Z、等一会儿 .）\n", step);
                return false;
        }
        // Z：面朝前方按一下确认（WorldScene::interact，开箱、采集就是这一下）；「.」：空转一刻（0.25 秒），
        // 让得物卡片淡入、排队的几张出场（docs/ch07-playtest-ui.md 第 6 节的截图就是这样拍的）。
        if (step == 'Z' || step == '.') {
            WorldScene* world = topWorld(app);
            if (world == nullptr) {
                std::fprintf(stderr, "walk: %c 之前栈顶已不是世界层，停下\n", step);
                return false;
            }
            if (step == 'Z') (void)world->interact(app);
            for (int i = 0; i < (step == 'Z' ? 1 : 15); ++i) frame(app);
            continue;
        }
        // 每一步之前核栈顶仍是世界层：上一步撞上明雷、踩响触发器，压上来的是战斗或对白，
        // 再往下走就是对着被压住的那一层乱走，拍出来的也不是要的那一屏。
        WorldScene* world = topWorld(app);
        if (world == nullptr) {
            std::fprintf(stderr, "walk: 走 %c 那一步之前栈顶已不是世界层，停下\n", step);
            return false;
        }
        (void)world->tryStep(app, dx, dy);
        // 空转到这一步滑完（主角与跟随者都停下）：滑多久问画面自己，不从别处抄步长。
        for (int i = 0; i < kMaxFramesPerStep; ++i) {
            frame(app);
            if (topWorld(app) != world || settled(world->view())) break;
        }
    }
    if (topWorld(app) == nullptr) {
        std::fprintf(stderr, "walk: 走完之后栈顶不是世界层（踩响了什么），不拍\n");
        return false;
    }
    return true;
}

}  // namespace

bool pushCompanionShot(Application& app, const std::string& kind, const std::string& arg, int& warmup) {
    (void)warmup;   // 这几个入口都不用比缺省更长的空转
    if (kind == "lilian" && !arg.empty()) {
        // 认不出的历练地如实报错：组队面板照样画得出来，拍出一张看着正常、其实什么都不是的图最难查。
        if (app.lilianSite(arg) == nullptr) {
            std::fprintf(stderr, "lilian: 没有这处历练地：%s\n", arg.c_str());
            return false;
        }
        app.pushScene(std::make_unique<LilianPartyScene>(arg));
        return true;
    }
    if (kind == "menu" && (arg == "party" || arg == "roster")) {
        // 名册上没人时主菜单没有这两页（会退回状态页）：要拍的那一页根本不存在，如实报错。
        if (rules::roster(app.data().companions, app.state()).empty()) {
            std::fprintf(stderr, "menu:%s：名册上没人，主菜单没有这一页\n", arg.c_str());
            return false;
        }
        app.setFixedWorldClock(kShotMenuWorldClockSeconds);
        app.pushScene(std::make_unique<MenuScene>(arg == "party" ? MenuScene::Page::Party : MenuScene::Page::Roster));
        return true;
    }
    if (kind == "walk" && !arg.empty()) return walk(app, arg);
    return false;
}

}  // namespace fanren::game
