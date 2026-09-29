// 程序入口。
//
// 只做三件事：解析命令行、拉起 Application、把第一张地图和世界场景摆好。
// 其余一切都在场景栈里跑，这里不放任何游戏逻辑。
//
// 三个开发自检口（--map / --scene / --screenshot）也落在这里。它们与 --headless
// 是同一类东西：不改任何游戏行为，只是让人（和 CI）能不靠手动操作就看到某一屏
// 长什么样。放在 main 而不是另起一个工具，是因为另起一个就要把「找资产根、读档、
// 载图、推场景」这四步原样抄一遍，而抄出来的那一份迟早与这里走样。
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/BoardScene.h"
#include "game/ChapterCardScene.h"
#include "game/CultivationScene.h"
#include "game/DialogueScene.h"
#include "game/FxDemoScene.h"
#include "game/MenuScene.h"
#include "game/SettingsScene.h"
#include "game/ShopScene.h"
#include "game/TitleScene.h"
#include "game/WorldScene.h"
#include "io/GameRoot.h"
#include "io/SaveFile.h"
#include "io/SettingsFile.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_messagebox.h>

namespace {

// 「--flag 值」里的那个值。没给返回空串。
std::string optionValue(int argc, char** argv, const char* flag) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], flag) == 0) return argv[i + 1];
    }
    return std::string{};
}

// 资产根目录。给了 --assets 就用它；没给就从工作目录、exe 目录各往上找（io/GameRoot.h）——
// 双击 build\fanren.exe 时工作目录是 build\，从前一律取「.」，一启动就读不到 data/。
// 两处都找不到返回空串，由 main 如实报错。
std::string resolveAssetRoot(int argc, char** argv) {
    const std::string value = optionValue(argc, argv, "--assets");
    if (!value.empty()) return value;
    std::vector<std::filesystem::path> starts{std::filesystem::path(".")};
    // SDL_GetBasePath 给的是 UTF-8，按 char8_t 构造才不会被当成本机代码页。
    if (const char* base = SDL_GetBasePath()) starts.emplace_back(reinterpret_cast<const char8_t*>(base));
    const std::filesystem::path found = fanren::io::locateGameRoot(starts);
    if (found.empty()) return std::string{};
    // 工作目录本身就是根（命令行 cd 到工程根再跑）时照旧给「.」，与改动前一字不差：
    // string() 要按本机代码页把绝对路径窄化，路径里有代码页表示不了的字的话会坏掉，而「.」不经这一步。
    std::error_code ec;
    if (std::filesystem::equivalent(found, ".", ec)) return std::string(".");
    return found.string();
}

// 起不来时的报错。fanren.exe 是窗口程序，双击启动时 stderr 没有地方显示，人只看到「点了没反应」，
// 所以正常游玩时再弹一个框。--headless 与 --screenshot 不弹：那是冒烟与截图脚本在跑，没人去点那个框，
// 一个模态框会把它们卡死。调用方先 shutdown 再调它：全屏窗口还在的话会把框盖住。
void reportFatal(bool interactive, const std::string& message) {
    std::fprintf(stderr, "%s\n", message.c_str());
    if (interactive) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "凡人修仙传", message.c_str(), nullptr);
    }
}

bool hasFlag(int argc, char** argv, const char* flag) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], flag) == 0) return true;
    }
    return false;
}

// 截图前先空转几帧。pushScene 是延迟生效的（帧末才真正入栈并调 onEnter），
// 第一帧画不到刚推进去的那个场景；三帧足够，也不至于让脚本跑出别的动静。
constexpr int kShotWarmupFrames = 3;

// 有自己的淡入、逐字的画面要多等一会儿，不然拍到的是一片黑或半句话：
// 标题画面淡入 1.2 秒；章节卡淡入 0.9 秒后停 2.5 秒，拍在停住的那一段；
// 对白逐字 60 字/秒，四秒够最长的一句滚完、右下角的「▼」出来。
constexpr int kTitleShotFrames = 150;
constexpr int kCardShotFrames = 90;
constexpr int kTalkShotFrames = 240;

// "a:b:c" 按 ':' 切开。--scene 的参数里文案 key 用点、选项用逗号，冒号只作分隔。
std::vector<std::string> splitSpec(const std::string& spec, char separator) {
    std::vector<std::string> parts;
    std::size_t from = 0;
    for (;;) {
        const std::size_t at = spec.find(separator, from);
        parts.push_back(spec.substr(from, at == std::string::npos ? std::string::npos : at - from));
        if (at == std::string::npos) return parts;
        from = at + 1;
    }
}

// 八方旅人化改造（界面路）加的几个截图口。认得就压场景、定好空转帧数并返回 true。
//   title                                   标题画面
//   chapter:<n>[:end]                       第 n 章开篇卡 / 章末那一串（终卡接下一章）
//   menu[:status|items|magic|board|settings] 在当前地图上打开主菜单（物品、法术两页光标进列表；
//                                           settings 是光标停在「设置」、右栏写摘要）
//   settings[:keys]                         系统设置面板 / 改键面板，压在世界层上（docs/settings.md 5.5）
//   talk:<文案 key>[:<说话人角色 id>]       对话框；带说话人就有名签
//   choice:<选项 key,选项 key…>[:<上一句 key>[:<说话人>]]   选项对话框
//   ending:<标题 key>:<正文 key>            结局卡
//   cultivate / shop:<店 id>                修炼面板 / 商店面板
bool pushUiShot(fanren::game::Application& app, const std::string& kind, const std::string& arg,
                int& warmup) {
    namespace game = fanren::game;
    const std::vector<std::string> parts = splitSpec(arg, ':');
    if (kind == "title") {
        app.pushScene(std::make_unique<game::TitleScene>());
        warmup = kTitleShotFrames;
        return true;
    }
    if (kind == "chapter" && !arg.empty()) {
        const game::ChapterEntry* entry = app.chapterTable().find(std::atoi(parts[0].c_str()));
        if (entry == nullptr) return false;
        const bool closing = parts.size() > 1 && parts[1] == "end";
        app.presentCards(closing ? game::cardsForFlagChange(app.chapterTable(), entry->doneFlag, 0, 1)
                                 : std::vector<game::CardRequest>{
                                       {game::CardKind::Opening, entry->number}});
        warmup = kCardShotFrames;
        return true;
    }
    if (kind == "menu") {
        using Page = game::MenuScene::Page;
        const std::string page = arg.empty() ? std::string("status") : arg;
        if (page == "status") app.pushScene(std::make_unique<game::MenuScene>(Page::Status));
        else if (page == "items") app.pushScene(std::make_unique<game::MenuScene>(Page::Items, true));
        else if (page == "magic") app.pushScene(std::make_unique<game::MenuScene>(Page::Magic, true));
        else if (page == "board") app.pushScene(std::make_unique<game::MenuScene>(Page::Journal, true));
        else if (page == "settings") app.pushScene(std::make_unique<game::MenuScene>(Page::Settings));
        else {
            return false;
        }
        return true;
    }
    if (kind == "settings" && (arg.empty() || arg == "keys")) {
        // settings:keys：设置面板一打开就翻开改键面板（与游戏里从「按键设置」进是同一条路）。
        app.pushScene(std::make_unique<game::SettingsScene>(arg == "keys"));
        return true;
    }
    if (kind == "talk" && !arg.empty()) {
        const std::string speaker = parts.size() > 1 ? app.speakerName(parts[1]) : std::string{};
        app.pushScene(std::make_unique<game::DialogueScene>(speaker, app.text(parts[0])));
        warmup = kTalkShotFrames;
        return true;
    }
    if (kind == "choice" && !arg.empty()) {
        std::vector<std::string> options;
        for (const std::string& key : splitSpec(parts[0], ',')) options.push_back(app.text(key));
        const std::string prompt = parts.size() > 1 ? app.text(parts[1]) : std::string{};
        const std::string speaker = parts.size() > 2 ? app.speakerName(parts[2]) : std::string{};
        auto scene = std::make_unique<game::DialogueScene>(speaker, prompt);
        scene->setChoices(std::move(options));
        scene->revealAtOnce();
        app.pushScene(std::move(scene));
        return true;
    }
    if (kind == "ending" && parts.size() == 2) {
        app.pushScene(game::ChapterCardScene::ending(app.text(parts[0]), app.text(parts[1])));
        warmup = kCardShotFrames;
        return true;
    }
    if (kind == "cultivate") {
        app.pushScene(std::make_unique<game::CultivationScene>());
        return true;
    }
    if (kind == "shop" && !arg.empty()) {
        app.pushScene(std::make_unique<game::ShopScene>(arg));
        return true;
    }
    return false;
}

// path:<npc 对象名>[:<旗标>,<旗标>…]：在当前地图上对那个 NPC 打开路径行动菜单，走的就是
// 按 E 的那条路（Application::openPathActions）。
//
// 条目要 when 成立才挂得出来，而第 1、2 章没有停在那些时段里的存档（tests/fixtures 只有
// 第 3 章起的章末，章末时本章条目都已收起），所以可以先置几面旗标（置 1）再开。
// 一条都没挂着就如实失败，不拍一张空的世界层。
// 对象名空着（path::<旗标>…）：只置旗标、不开菜单，拍行走画面上的头顶气泡（契约 5.2）。
bool pushPathShot(fanren::game::Application& app, const std::string& arg) {
    const std::vector<std::string> parts = splitSpec(arg, ':');
    if (parts.size() > 1) {
        for (const std::string& flag : splitSpec(parts[1], ',')) app.state().setFlag(flag);
    }
    return parts[0].empty() ? parts.size() > 1 : app.openPathActions(parts[0]);
}

}  // namespace

int main(int argc, char** argv) {
    // --headless 供 CI 与冒烟脚本使用：不开窗口，跑几帧就退出。
    const bool headless = hasFlag(argc, argv, "--headless");
    const bool interactive = !headless && optionValue(argc, argv, "--screenshot").empty();

    const std::string assetRoot = resolveAssetRoot(argc, argv);
    if (assetRoot.empty()) {
        reportFatal(interactive,
                    "找不到游戏目录（同时有 data 与 maps 的那一层）。\n"
                    "请从工程的构建目录（如 fanren\\build）里启动 fanren.exe，或用 --assets 指定。");
        return 1;
    }

    fanren::game::Application app;
    auto ready = app.init(assetRoot, headless);
    if (!ready) {
        app.shutdown();
        reportFatal(interactive, "初始化失败：" + ready.error + "\n游戏目录：" + assetRoot);
        return 1;
    }
    // 野外遭遇：Application 缺省关着（几百条无头测试不该冷不丁被拖进一场仗），上线的游戏打开。
    app.setEncountersEnabled(true);

    // ---- 系统设置（docs/settings.md 第 2 节「谁读、谁写」）----
    //
    // 同一个套路：Application 缺省不碰文件，这里按启动方式决定开不开。
    //   · 正常游玩（标题流程、--load、--map）读写 --settings 指的那份，没给就是 <资产根>/saves/settings.json；
    //   · --headless 永远不读：冒烟要的是确定的默认值；
    //   · --screenshot 只在显式给了 --settings 时读，而且**只读不写**：拍出来的图不受开发机上的偏好左右，
    //     要拍某种设置下的画面就显式指一份；那一份是截图夹具，不许被改——全屏被窗口系统驳回时，每帧对账会把
    //     fullscreen 改回 false，enable 了的话就写回夹具里去了。所以这里不 enable，只读来 setSettings。
    // 读在压任何场景之前：全屏、音量从第一帧起就是玩家要的样子。坏文件只打一行警告，不挡启动。
    const std::string settingsFile = optionValue(argc, argv, "--settings");
    const bool shooting = !optionValue(argc, argv, "--screenshot").empty();
    if (!headless && !shooting) {
        const std::string path = settingsFile.empty() ? app.defaultSettingsPath() : settingsFile;
        auto read = app.enableSettingsFile(path);
        if (!read) std::fprintf(stderr, "[settings] %s（%s）\n", read.error.c_str(), path.c_str());
    } else if (shooting && !settingsFile.empty()) {
        const fanren::io::SettingsRead read = fanren::io::loadSettings(settingsFile);
        for (const std::string& warning : read.warnings) {
            std::fprintf(stderr, "[settings] %s：%s\n", settingsFile.c_str(), warning.c_str());
        }
        if (!read.error.empty()) std::fprintf(stderr, "[settings] %s（%s）\n", read.error.c_str(), settingsFile.c_str());
        app.setSettings(read.settings);
    }

    // ---- 读档（可选）----
    //
    // `io::saveGame` / `io::loadGame` 造好了、有版本号、有 1→5 的迁移表、有测试，
    // 但在这一段出现之前**游戏里一个调用方都没有**：main 永远从第 1 章新开一局。
    // 又是 docs/README.md 那张「一个模块可用需要三样齐备」的表——规则层与加载器
    // 都在，缺的是游戏层的入口。这里补上最小的那一段。
    //
    // **存档里带着地图 id 与坐标**，所以读档成功时不要再去载第 1 章那张图：
    // 那会把人摆回韩家村，而旗标与背包却是后面章节的——那种半截状态比读档失败
    // 更难查。
    const std::string savePath = optionValue(argc, argv, "--load");
    const bool loaded = !savePath.empty();
    // --map <地图 id>：直接开在指定的那张图上，开发自检用（配合 --screenshot
    // 一张张拍过去）。给了它就走那张图的出生点，不再沿用存档里的坐标——
    // 存档的坐标属于另一张图，套过来就是把人摆到一个说不清的位置。
    const std::string mapOverride = optionValue(argc, argv, "--map");
    const std::string sceneSpec = optionValue(argc, argv, "--scene");

    // ---- 标题画面（八方旅人化改造）----
    //
    // 不带 --load / --map 启动时的第一个场景。两种情形不走它：无头（没人按键，冒烟要的是
    // 直接进世界跑几帧），与 --scene（截图口要拍的是它指名的那一屏，底下垫世界层）。
    const bool titleFlow = !loaded && mapOverride.empty() && sceneSpec.empty() && !headless;
    if (titleFlow) {
        app.pushScene(std::make_unique<fanren::game::TitleScene>());
    } else if (loaded && mapOverride.empty()) {
        // 与标题画面的「继续旅程」同一条路：读档 → 载存档里那张图 → 站回那一格 → 世界层。
        // 失败如实报错并退出，**不静默退回新开局**：玩家指名要读这一份，
        // 悄悄给他一个新档是最糟的那种「成功」。
        auto resumed = app.continueJourney(savePath);
        if (!resumed) {
            app.shutdown();
            reportFatal(interactive, "读档失败：" + resumed.error);
            return 1;
        }
    } else {
        if (loaded) {
            // --load 与 --map 同给：状态取存档，站位取 --map 那张图的出生点。
            auto save = fanren::io::loadGame(savePath);
            if (!save) {
                app.shutdown();
                reportFatal(interactive, "读档失败：" + save.error);
                return 1;
            }
            app.state() = save.value;
        }
        const std::string firstMap =
            !mapOverride.empty() ? mapOverride : std::string(fanren::game::kNewGameMap);
        auto mapLoaded = app.loadMap(firstMap, std::string{});
        if (!mapLoaded) {
            app.shutdown();
            reportFatal(interactive, "载入地图失败：" + mapLoaded.error);
            return 1;
        }
        app.pushScene(std::make_unique<fanren::game::WorldScene>());
    }

    // ---- 截图自检口（可选）----
    //
    // 改了渲染却看不见自己改成什么样，就只能靠「应该好看了」交付，而那在本
    // 工程里不算证据。这一段摆好场景、空转几帧、抓一张 PNG 就退出。
    // --scene 再往栈上压一层，用来拍面板与战斗板。
    const std::string shotPath = optionValue(argc, argv, "--screenshot");
    if (!shotPath.empty()) {
        if (headless) {
            // 无头模式压根没有渲染器。这里如实拒绝，而不是写出一个全黑的
            // PNG ——那种「成功」比失败难查得多。
            std::fprintf(stderr, "--screenshot 需要渲染器，不能与 --headless 同用\n");
            app.shutdown();
            return 1;
        }
        // --scene battle:<编成 id> / board / talk:<文案 key> / fxdemo[:night|day|rain|snow]
        //
        // 一个口子而不是三个开关：面板种类只会越来越多，每种加一个 --xxx
        // 到最后 main 的命令行比游戏还长。认不出来的 spec 如实报错退出，
        // 不要悄悄拍一张世界层——那种「成功」会让人以为面板画出来了。
        // 标题画面自己有 1.2 秒的淡入：照三帧拍就是一片黑。
        int warmup = titleFlow ? kTitleShotFrames : kShotWarmupFrames;
        const std::string& scene = sceneSpec;
        if (!scene.empty()) {
            const std::size_t colon = scene.find(':');
            const std::string kind = scene.substr(0, colon);
            const std::string arg =
                colon == std::string::npos ? std::string{} : scene.substr(colon + 1);
            std::string battleId;
            fanren::game::BattleShot battleShot = fanren::game::BattleShot::None;
            if (kind == "battle" && fanren::game::parseBattleShot(arg, battleId, battleShot)) {
                // battle:<编成 id>[:intro|mid|charge|victory]：开局 / 碎屏中途 / 头一次破势 / 首领蓄势 / 战果卡。
                app.pushScene(std::make_unique<fanren::game::BattleScene>(battleId, battleShot));
            } else if (kind == "board") {
                app.pushScene(std::make_unique<fanren::game::BoardScene>());
            } else if (pushUiShot(app, kind, arg, warmup)) {
                // 界面路的截图口（标题、章节卡、主菜单、对白、选项、结局卡、两块面板），见上。
            } else if (kind == "path" && pushPathShot(app, arg)) {
                // 路径行动菜单（docs/interfaces-octo-pathactions.md 第 5 节），见 pushPathShot。
            } else if (kind == "fxdemo" && fanren::game::FxDemoScene::knowsPreset(arg)) {
                // 后处理与粒子的自检画面（不依赖任何美术产物），见 FxDemoScene.h。
                app.pushScene(std::make_unique<fanren::game::FxDemoScene>(arg));
            } else {
                std::fprintf(stderr, "认不出的 --scene：%s\n", scene.c_str());
                app.shutdown();
                return 1;
            }
        }
        for (int frame = 0; frame < warmup; ++frame) app.tick(1.0 / 60.0);

        app.engine().beginFrame();
        app.drawScenes();
        auto shot = app.engine().captureFrame(shotPath);
        app.engine().endFrame();
        if (!shot) {
            std::fprintf(stderr, "截图失败：%s\n", shot.error.c_str());
            app.shutdown();
            return 1;
        }
        std::printf("SHOT_OK %s map=%s\n", shotPath.c_str(), app.state().mapId.c_str());
        app.shutdown();
        return 0;
    }

    if (headless) {
        // 冒烟：推进若干帧确认不崩，然后正常退出。
        for (int frame = 0; frame < 120 && !app.quitRequested(); ++frame) {
            app.tick(1.0 / 60.0);
        }
        std::printf("HEADLESS_OK map=%s pos=%d,%d\n", app.state().mapId.c_str(),
                    app.state().position.x, app.state().position.y);
        app.shutdown();
        return 0;
    }

    const int code = app.run();
    app.shutdown();
    return code;
}
