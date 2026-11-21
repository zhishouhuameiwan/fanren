#include "game/ShotScenes.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <sstream>

#include "core/rules/Roster.h"
#include "game/Application.h"
#include "game/BoardScene.h"
#include "game/TutorialScene.h"
#include "game/DialogueScene.h"
#include "game/AreaMapScene.h"
#include "game/LilianPartyScene.h"
#include "game/MenuScene.h"
#include "game/ShopScene.h"
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
            case 'u': app.state().facing = 0; continue;   // 小写：只转身不迈步（「在蒲团旁面朝它」那一张靠这个）
            case 'r': app.state().facing = 1; continue;
            case 'd': app.state().facing = 2; continue;
            case 'l': app.state().facing = 3; continue;
            default:
                std::fprintf(stderr, "walk: 认不出的方向 %c（只认 U D L R、只转身的 u d l r，与按确认 Z、等一会儿 .）\n",
                             step);
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
// 女角色立绘（docs/heroine-art-build.md 3.1）加的一个：
//   dlg:say:<角色 id>:<表情>:<文案 key>[,<角色 id>:<表情>:<文案 key>…][@<帧数>]
//       按次序说这几句（与脚本 talk 同一个入口 sayAs；角色 id 空是旁白）。前几句各停半秒（立绘早已淡入）再收起，
//       末一句压上来之后空转 <帧数> 帧再拍（缺省 120 帧：淡入走完、字也显示全）；给小的帧数拍的是淡入 / 淡出的中途——
//       例：dlg:say:chen_shimei:smile:<key>,hanli:normal:<key>@4 拍的是韩立开口、她淡出到一半的那一下。
bool pushSayShot(Application& app, const std::string& spec, int& warmup) {
    const std::size_t at = spec.find('@');
    std::stringstream lines(spec.substr(0, at));
    std::vector<std::vector<std::string>> said;
    for (std::string line; std::getline(lines, line, ',');) {
        std::vector<std::string> parts;
        std::stringstream fields(line);
        for (std::string part; std::getline(fields, part, ':');) parts.push_back(part);
        if (parts.size() != 3 || parts[2].empty()) {
            std::fprintf(stderr, "dlg:say: 每一句要写成 <角色 id>:<表情>:<文案 key>，这一句是「%s」\n", line.c_str());
            return false;
        }
        said.push_back(std::move(parts));
    }
    if (said.empty()) return false;
    for (std::size_t i = 0; i < said.size(); ++i) {
        const std::vector<std::string>& line = said[i];
        app.sayAs(line[0], line[2], line[1].empty() ? std::string(script::kDefaultFace) : line[1]);
        if (i + 1 == said.size()) break;
        for (int f = 0; f < 30; ++f) frame(app);   // 落栈、淡入走完
        app.popScene();                             // 这一句收起（游戏里是按确认）
        frame(app);                                 // 换句之间那一帧空档，与游戏里一样
    }
    warmup = at == std::string::npos ? 120 : std::max(0, std::atoi(spec.c_str() + at + 1));
    return true;
}

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
    // 女角色立绘（docs/heroine-art-build.md 3.2）：menu:party[#<n>]@<角色 id>,<角色 id>…——先让这几位入队，
    // 再打开队伍页、光标停在第 n 位（0 起），拍右栏的大图（有立绘）与像素小人（没有）。
    if (kind == "menu" && arg.rfind("party", 0) == 0 && arg.find('@') != std::string::npos) {
        const std::size_t at = arg.find('@');
        const std::size_t hash = arg.find('#');
        const int row = hash != std::string::npos && hash < at ? std::atoi(arg.c_str() + hash + 1) : 0;
        std::stringstream roles(arg.substr(at + 1));
        for (std::string role; std::getline(roles, role, ',');) {
            if (app.data().findRole(role) == nullptr) {
                std::fprintf(stderr, "menu:party: 没有这个角色：%s\n", role.c_str());
                return false;
            }
            if (!app.state().partyHas(role)) (void)app.state().partyAdd(role);
        }
        app.setFixedWorldClock(kShotMenuWorldClockSeconds);
        app.pushScene(std::make_unique<MenuScene>(MenuScene::Page::Party, true, row));
        return true;
    }
    // 女角色立绘（docs/heroine-art-build.md 3.5.3）：card:<角色 id>——那个人的登场卡，空转到滑入、淡入都走完。
    if (kind == "card" && !arg.empty()) {
        if (!app.pushIntroCard(arg)) return false;   // 没有立绘：Application 已警告
        warmup = std::max(warmup, 45);
        return true;
    }
    if (kind == "walk" && !arg.empty()) return walk(app, arg);
    // ---- 第 7 章改版·路 M（docs/ch07-foundation.md 1.6、第 3 节）----
    // menu:items:<n> / menu:magic:<n>：物品 / 法术页，光标停在第 n 行（0 起），看那一样的四行式说明。
    if (kind == "menu" && (arg.rfind("items:", 0) == 0 || arg.rfind("magic:", 0) == 0)) {
        const bool items = arg.rfind("items:", 0) == 0;
        const int row = std::atoi(arg.c_str() + 6);
        app.setFixedWorldClock(kShotMenuWorldClockSeconds);
        app.pushScene(std::make_unique<MenuScene>(items ? MenuScene::Page::Items : MenuScene::Page::Magic, true, row));
        return true;
    }
    // menu:map：主菜单光标停在「地图」上（右栏那一句说明）。
    if (kind == "menu" && arg == "map") {
        app.setFixedWorldClock(kShotMenuWorldClockSeconds);
        app.pushScene(std::make_unique<MenuScene>(MenuScene::Page::Map));
        return true;
    }
    // areamap：区域地图（压在世界层上，与按 M 同一个场景）。
    if (kind == "areamap") {
        app.pushScene(std::make_unique<AreaMapScene>());
        return true;
    }
    // toast:<物品 id>,<物品 id>…：照 announceItem 推几张得物卡片，空转到它们都出场（看副行的效果行）。
    if (kind == "toast" && !arg.empty()) {
        frame(app);
        std::size_t from = 0;
        while (from <= arg.size()) {
            const std::size_t comma = arg.find(',', from);
            const std::string id = arg.substr(from, comma == std::string::npos ? std::string::npos : comma - from);
            if (!id.empty()) {
                if (app.data().findItem(id) == nullptr) {
                    std::fprintf(stderr, "toast: 没有这件东西：%s\n", id.c_str());
                    return false;
                }
                app.announceItem(id, 1);
            }
            if (comma == std::string::npos) break;
            from = comma + 1;
        }
        for (int i = 0; i < 40; ++i) frame(app);
        return true;
    }
    // shopbuy:<店 id>：商店直接停在买货页（光标在第一样货上，看左栏的四行式说明）。
    if (kind == "shopbuy" && !arg.empty()) {
        if (app.shop(arg) == nullptr) {
            std::fprintf(stderr, "shopbuy: 没有这家店：%s\n", arg.c_str());
            return false;
        }
        app.pushScene(std::make_unique<ShopScene>(arg, true));
        return true;
    }
    if (kind == "guide" && !arg.empty()) return pushGuideShot(app, arg, warmup);
    if (kind == "seam" && !arg.empty()) return pushSeamShot(app, arg, warmup);
    if (kind == "dlg" && arg.rfind("say:", 0) == 0) return pushSayShot(app, arg.substr(4), warmup);
    if (kind == "dlg" && !arg.empty()) {
        // 气泡有 0.15 秒的淡入：照缺省的几帧拍，拍到的是半透明的那一下。多空转半秒。
        if (arg.rfind("bubble:", 0) == 0) warmup = std::max(warmup, 30);
        return pushDialogueShot(app, arg);
    }
    return false;
}

bool pushSeamShot(Application& app, const std::string& spec, int& warmup) {
    const std::size_t at = spec.find('@');
    if (at != std::string::npos && !pushGuideShot(app, "world" + spec.substr(at), warmup)) return false;
    frame(app);   // 世界层入栈、目标框排好
    std::stringstream parts(spec.substr(0, at));
    for (std::string part; std::getline(parts, part, ',');) {
        if (part == "lore") {
            const core::LoreEntry* entry = app.data().lore.empty() ? nullptr : &app.data().lore.back();
            if (entry == nullptr) return false;
            app.loreNotices().push(app.data().lookupText("ui.guide.lore.unlocked") + app.data().lookupText(entry->titleKey));
        } else if (part == "skip") {
            app.noteDialogueSkipped(12);
        } else if (part == "save") {
            WorldScene* world = app.worldScene();
            if (world == nullptr) return false;
            world->showSaveNotice(app.text("ui.world.save.ok"), 30.0);
        } else if (part == "toast") {
            app.announceItem("pill_huiqi_dan", 1);
            app.announceItem("material_lingshi", 3);
        } else {
            std::fprintf(stderr, "seam: 认不出的部件：%s\n", part.c_str());
            return false;
        }
    }
    warmup = std::max(warmup, 30);   // 见闻提示、卡片都要淡进来
    return true;
}

bool pushGuideShot(Application& app, const std::string& spec, int& warmup) {
    // guide:<那一屏>[@记号,记号…]。记号先办（演示用的存档状态），再压那一屏：
    //   旗标名          置这一面旗（ch07.chaibao、tutorial.seen.boost、ch07.pojin……）
    //   weak.<角色 id>  把这个角色的破绽全记下（见闻页的「已知破绽」）
    const std::size_t at = spec.find('@');
    const std::string what = spec.substr(0, at);
    if (at != std::string::npos) {
        std::stringstream marks(spec.substr(at + 1));
        for (std::string mark; std::getline(marks, mark, ',');) {
            if (mark.rfind("weak.", 0) == 0) {
                const core::RoleTemplate* role = app.data().findRole(mark.substr(5));
                if (role == nullptr) {
                    std::fprintf(stderr, "guide: 没有这个角色：%s\n", mark.c_str());
                    return false;
                }
                app.state().learnWeaknesses(role->id, role->weaknesses);
            } else if (!mark.empty()) {
                app.state().setFlag(mark);
            }
        }
    }
    if (what == "world") return true;   // 只要底下那张世界层（目标框、头顶标记）
    // <页>#<n>：光标停在第 n 行（第一期地基包集成加的，拍见闻页最长的那一条与翻页）。
    const std::size_t hash = what.find('#');
    const std::string pageName = what.substr(0, hash);
    const int startRow = hash == std::string::npos ? 0 : std::atoi(what.c_str() + hash + 1);
    if (pageName == "main" || pageName == "side" || pageName == "guide" || pageName == "lore") {
        app.setFixedWorldClock(kShotMenuWorldClockSeconds);
        const BoardScene::Page page = pageName == "main"    ? BoardScene::Page::Main
                                      : pageName == "side"  ? BoardScene::Page::Side
                                      : pageName == "guide" ? BoardScene::Page::Guide
                                                            : BoardScene::Page::Lore;
        app.pushScene(std::make_unique<BoardScene>(page, startRow));
        return true;
    }
    if (what.rfind("tutorial=", 0) == 0) {
        app.pushScene(std::make_unique<TutorialScene>(what.substr(9)));
        return true;
    }
    if (what.rfind("lorenote=", 0) == 0) {
        // 见闻提示要淡进来：多空转几帧（世界层露着时它才走时钟）。
        for (const core::LoreEntry& entry : app.data().lore) {
            if (entry.id == what.substr(9)) {
                app.loreNotices().push(app.data().lookupText("ui.guide.lore.unlocked") + app.data().lookupText(entry.titleKey));
                warmup = std::max(warmup, 30);
                return true;
            }
        }
        std::fprintf(stderr, "guide: 没有这条见闻：%s\n", what.c_str());
        return false;
    }
    std::fprintf(stderr, "guide: 认不出的一屏：%s\n", what.c_str());
    return false;
}

}  // namespace fanren::game
