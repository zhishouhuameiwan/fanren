#include "game/ShotScenes.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>

#include "core/rules/Roster.h"
#include "game/Application.h"
#include "game/DialogueScene.h"
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

// 地基包路 D（docs/ch07-foundation.md 4.2–4.4）的四张图：
//   dlg:auto:<文案 key>[:<说话人>]      自动播放开着的对话框（框上沿的「自动」小标）。只改这一局的设置，不写盘
//   dlg:skipask:<文案 key>[:<说话人>]   整段跳过的确认框
//   dlg:skipped:<句数>                  跳完之后那一条「已跳过 N 句 · 按 Tab 回看」
//   dlg:bubble:<角色 id>:<文案 key>     插话气泡：这个角色若不在队里先让她入队（跟在主角身后），气泡浮在她头顶
bool pushDialogueShot(Application& app, const std::string& arg) {
    std::vector<std::string> parts;
    for (std::size_t from = 0;;) {
        const std::size_t at = arg.find(':', from);
        parts.push_back(arg.substr(from, at == std::string::npos ? std::string::npos : at - from));
        if (at == std::string::npos) break;
        from = at + 1;
    }
    const std::string& what = parts[0];
    if ((what == "auto" || what == "skipask") && parts.size() >= 2) {
        const std::string speaker = parts.size() > 2 ? app.speakerName(parts[2]) : std::string{};
        auto scene = std::make_unique<DialogueScene>(speaker, app.text(parts[1]));
        if (parts.size() > 2) scene->setPortrait(parts[2], script::kDefaultFace);
        scene->revealAtOnce();
        if (what == "auto") {
            io::Settings next = app.settings();
            next.autoAdvance = true;
            app.setSettings(next);
        } else {
            scene->openSkipAsk(app);
        }
        app.pushScene(std::move(scene));
        return true;
    }
    if (what == "skipped" && parts.size() == 2) {
        app.noteDialogueSkipped(std::atoi(parts[1].c_str()));
        return true;
    }
    if (what == "bubble" && parts.size() == 3) {
        if (!app.state().partyHas(parts[1])) (void)app.state().partyAdd(parts[1]);
        // 跟随者要等世界层排开才找得到：先挂在主角身后一格，排开之后每帧按角色重找。
        const core::Point hero = app.state().position;
        app.pushSpeechBubble(parts[1], parts[2], 30.f,
                             Vec2f{static_cast<float>(hero.x), static_cast<float>(hero.y + 1)});
        return true;
    }
    std::fprintf(stderr, "dlg: 认不出的 %s\n", arg.c_str());
    return false;
}

}  // namespace

bool pushCompanionShot(Application& app, const std::string& kind, const std::string& arg, int& warmup) {
    // 这几个入口都不用比缺省更长的空转；地基包路 D 的气泡图例外（见下）。
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
    if (kind == "dlg" && !arg.empty()) {
        // 气泡有 0.15 秒的淡入：照缺省的几帧拍，拍到的是半透明的那一下。多空转半秒。
        if (arg.rfind("bubble:", 0) == 0) warmup = std::max(warmup, 30);
        return pushDialogueShot(app, arg);
    }
    return false;
}

}  // namespace fanren::game
