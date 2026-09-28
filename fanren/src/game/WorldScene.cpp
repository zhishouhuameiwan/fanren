#include "game/WorldScene.h"

#include <algorithm>
#include <string>
#include <vector>

#include "core/rules/Objectives.h"
#include "game/Application.h"

namespace fanren::game {
namespace {

// 单步冷却。按住方向键时连走的节奏；点一下只走一格。
// 旧原型的移动过快，玩家在窄路上很难停在想停的格子。
// 画面上一步滑多久（WorldView 的 kStepSlideSeconds）与它是同一个数：上一步刚滑到、下一步正好起步。
constexpr double kStepCooldownSeconds = 0.12;
static_assert(static_cast<float>(kStepCooldownSeconds) == kStepSlideSeconds,
              "一步的冷却与画面上滑一步的时长要一致，不然按住方向键时人会一顿一顿地走");

// 路径行动的两个钩子（setPathActionHooks）。没接时都是 nullptr。
WorldScene::PathBubbleProbe gPathBubbleProbe = nullptr;
WorldScene::PathActionOpener gPathActionOpener = nullptr;

}  // namespace

void WorldScene::setPathActionHooks(PathBubbleProbe probe, PathActionOpener opener) {
    gPathBubbleProbe = probe;
    gPathActionOpener = opener;
}

// 触发是否可以起：前置旗标满足，且「只触发一次」的那一次还没兑现。
//
// 「兑现」的判据是 set_flag 指的那个旗标已置位，而这个旗标由**脚本自己**在演完
// 之后置——引擎一个字都不预写。这条口径是本函数存在的全部理由：
//
//   引擎此前在 startEvent 之前就把记号打上（markTriggered 先于 startEvent），
//   于是任何「查前置 → 不满足就说一句话 → return」的脚本，被踩到的那一瞬间就把
//   once 触发器永久烧掉了；玩家后来满足了条件再回来，这一格已经不响了。关卡那边
//   只能靠「凡会提前 return 的脚本一律不设 once」这条口头约定绕开，而口头约定
//   必然失效。
//
//   现在「演完了没有」只有脚本知道，就让脚本说了算：所有事件脚本本来就在末尾
//   置一个完成旗标（ch01.zhangtie_met / ch02.renyao_done …），把 set_flag 指到
//   那一个上，提前 return 的路径根本走不到那一行，触发器自然烧不掉。作者不必
//   记住任何新规矩——他照常写完成旗标就是了。
//
//   代价是「脚本起手」到「旗标落地」之间有一段空窗，这段里触发器仍然是 ready 的。
//   防重入不靠这个记号，靠 tryStep / interact 开头那道「脚本在跑就不受理」——
//   见那两处的注释。
//
// once 却没写 set_flag 的触发器会退化成可重复触发：引擎无从知道它兑现了没有。
// 这种写法由 tools/validate.py 的 once 契约检查拦在门禁上，不在这里静默兜底
// ——从前那个按对象名生造 "_trigger.<name>" 的兜底，造出来的旗标既没在
// data/flags.json 登记、也没有任何工具查得到，是另一处看不见的账。
bool WorldScene::triggerReady(const core::GameState& state, const core::MapObject& trigger) {
    const std::string guard = trigger.property("guard_flag");
    if (!guard.empty() && state.flag(guard) == 0) return false;

    if (trigger.property("once") == "true") {
        const std::string mark = trigger.property("set_flag");
        if (!mark.empty() && state.flag(mark) != 0) return false;
    }
    return true;
}

// NPC 是否在场。visible_flag 未置位、或 hidden_flag 已置位，都当作不在。
//
// 「在场」是一件事，不是三件：画不画、谈不谈得上、占不占格，全看这一个答案。
// 从前只有画与谈看它，占格那一条走的是 objectAt，于是隐身的 NPC 是一堵隐形墙。
bool WorldScene::npcVisible(const core::GameState& state, const core::MapObject& npc) {
    const std::string shown = npc.property("visible_flag");
    if (!shown.empty() && state.flag(shown) == 0) return false;
    const std::string hidden = npc.property("hidden_flag");
    if (!hidden.empty() && state.flag(hidden) != 0) return false;
    return true;
}

const core::MapObject* WorldScene::visibleNpcAt(const core::GameState& state,
                                                const core::TileMap& map, core::Point cell) {
    for (const core::MapObject& object : map.objects) {
        if (object.type != "npc") continue;
        // 与 core::TileMap::objectAt 逐字相同的矩形命中——两处的形状必须一样，
        // 差一格就等于「看得见的和站得上的不是同一个东西」。
        const bool hit = cell.x >= object.position.x &&
                         cell.x < object.position.x + object.width &&
                         cell.y >= object.position.y &&
                         cell.y < object.position.y + object.height;
        if (!hit) continue;
        if (!npcVisible(state, object)) continue;   // 多问的就是这一句
        return &object;
    }
    return nullptr;
}

void WorldScene::onEnter(Application&) {
    stepCooldown_ = 0.0;
    // 画面从「开场」算起：镜头直接就位、不淡入、地名横幅直接在（外层的开场转场自己会淡）。
    view_.reset();
}

core::Point WorldScene::facingCell(const core::GameState& state) const {
    core::Point cell = state.position;
    switch (state.facing) {
        case 0: cell.y -= 1; break;
        case 1: cell.x += 1; break;
        case 2: cell.y += 1; break;
        default: cell.x -= 1; break;
    }
    return cell;
}

bool WorldScene::tryStep(Application& app, int dx, int dy) {
    const core::TileMap* map = app.currentMap();
    if (map == nullptr) return false;
    // 脚本在跑（或正等着一条命令被演完）时，世界层一步都不受理。
    //
    // update() 那边本来就有一道同样的闸，但那道只挡键盘；tryStep / interact 是
    // 公开给无头 bot 直接调的，绕得过去。而 once 触发器的记号现在要等脚本自己
    // 置旗标才落，起手到落地之间有一段空窗——防重入全靠这一道，所以它必须钉在
    // 触发真正发生的这两个入口上，不能只钉在输入层。
    if (app.scripts().isRunning() || app.awaitingCommand()) return false;
    core::GameState& state = app.state();

    // 先转向再迈步：面朝的方向本身是有意义的输入（交互看的是前一格），
    // 撞墙时至少要把朝向转过去，否则玩家会以为按键没响应。
    if (dx > 0) state.facing = 1;
    else if (dx < 0) state.facing = 3;
    else if (dy < 0) state.facing = 0;
    else if (dy > 0) state.facing = 2;

    const core::Point target{state.position.x + dx, state.position.y + dy};
    if (!map->walkable(target)) return false;
    // NPC 与设施占格，不能走上去。
    //
    // 设施这一条此前漏了：站到蒲团那一格上之后，交互看的是「面朝的前一格」，
    // 于是玩家正站在打坐点上却怎么按都没反应。占格之后只能面朝它按确认，
    // 与 NPC 同一套手感。
    //
    // NPC 走 visibleNpcAt 而不是 objectAt：**不在场的人不挡路**。从前用 objectAt，
    // 一个还没现身（或已经离开）的 NPC 照样占着那一格，玩家撞上的是一堵看不见、
    // 没有任何反馈的墙。设施没有可见性属性，照旧一律占格。
    if (visibleNpcAt(state, *map, target) != nullptr) return false;
    if (map->objectAt(target, "facility") != nullptr) return false;

    state.position = target;

    // 踏入型触发：走到即发事件。
    //
    // 这里不再预写任何记号。once 兑现与否由脚本末尾那句 flag.set 说了算
    // （见 triggerReady 的注释）：脚本半路 return 就等于「这一幕没演」，
    // 触发器留着，玩家满足条件再走回来还能触发。
    if (const core::MapObject* trigger = map->objectAt(target, "trigger")) {
        if (trigger->property("mode") == "enter" && triggerReady(state, *trigger)) {
            app.startEvent(trigger->property("script"));
            return true;
        }
    }
    // 传送点：踏入即换图。
    if (const core::MapObject* portal = map->objectAt(target, "portal")) {
        const std::string need = portal->property("require_flag");
        if (need.empty() || state.flag(need) != 0) {
            // 开门声只在真换了图时响；被拦住的那一支说一句为什么，不响（终审 M-3）。
            app.engine().playSfx("door_open");
            app.loadMap(portal->property("target_map"), portal->property("target_spawn"));
        } else {
            // 拦住玩家时必须说明还差什么。没有这一句，被拦的人只会觉得
            // 按键没反应，而不是「我还有事没办」。
            app.showMessage(portal->property("deny_text_key"));
        }
        return true;
    }
    // 野外遭遇：没换图、没踩响剧情的这一步，才算在野地里走了一步。站没站在遭遇区里、
    // 摇不摇得中、开不开关都由 Application 判（docs/interfaces-octo-encounters.md 第 3 节）。
    app.stepEncounters();
    return true;
}

bool WorldScene::interact(Application& app) {
    const core::TileMap* map = app.currentMap();
    if (map == nullptr) return false;
    // 与 tryStep 同一道闸，理由见那里。
    if (app.scripts().isRunning() || app.awaitingCommand()) return false;
    const core::Point front = facingCell(app.state());

    // 同样走 visibleNpcAt：不在场的那个不该抢走这一次确认键。从前用 objectAt，
    // 它只返回对象表里第一个命中的，可见性是命中**之后**才问的一句——于是一个
    // 隐身的 NPC 把同格那个该出面的压得死死的，玩家按确认毫无反应。
    if (const core::MapObject* npc = visibleNpcAt(app.state(), *map, front)) {
        const std::string script = npc->property("script");
        if (!script.empty()) {
            app.startEvent(script);
            return true;
        }
    }
    if (const core::MapObject* trigger = map->objectAt(front, "trigger")) {
        if (trigger->property("mode") == "interact" && triggerReady(app.state(), *trigger)) {
            app.startEvent(trigger->property("script"));
            return true;
        }
    }
    // 设施排在剧情触发之后：同一格上两者都挂着时，剧情优先——蒲团随时能坐，
    // 剧情错过就没了。按 kind 开哪个面板由 Application 决定，世界层不认识面板。
    if (const core::MapObject* facility = map->objectAt(front, "facility")) {
        app.openFacility(*facility);
        return true;
    }
    return false;
}

bool WorldScene::pathAction(Application& app) {
    const core::TileMap* map = app.currentMap();
    if (map == nullptr || gPathActionOpener == nullptr) return false;
    // 与 tryStep / interact 同一道闸，理由见 tryStep。
    if (app.scripts().isRunning() || app.awaitingCommand()) return false;
    // 找人与 interact 同一个口径：不在场的人不应声。
    const core::MapObject* npc = visibleNpcAt(app.state(), *map, facingCell(app.state()));
    if (npc == nullptr) return false;
    return gPathActionOpener(app, *npc);
}

// 存盘提示停留多久。够看清，又不至于杵在屏幕上碍事。
constexpr double kSaveNoticeSeconds = 3.0;

bool WorldScene::update(Application& app, double deltaSeconds) {
    // 画面先走：插值、镜头、粒子、横幅计时。放在所有早退之前——脚本在跑时世界照样是活的，
    // 只是不受理输入。这一步纯视觉，一个格子都不动。
    view_.advance(app, deltaSeconds);

    // 脚本正在跑时世界层不接受输入：否则玩家能在对话中把角色走开。
    if (app.awaitingCommand() || app.scripts().isRunning()) return true;

    engine::Engine& eng = app.engine();
    stepCooldown_ = std::max(0.0, stepCooldown_ - deltaSeconds);
    saveNoticeSeconds_ = std::max(0.0, saveNoticeSeconds_ - deltaSeconds);

    // 存盘。**只在世界层受理**：对话、战斗、面板里按 F5 不该有反应，
    // 因为存档里没有场景栈，那时候存下去读回来是另一个局面。
    // 上面那句 awaitingCommand / isRunning 的早退已经把对话挡在外头了。
    if (eng.keyPressed(engine::Engine::Key::Save)) {
        auto saved = app.quickSave();
        // 说给玩家听的话：读回走标题画面的「继续旅程」，不提命令行（终审 LOW-2）。文案在 ui.json，过禁词闸。
        saveNotice_ = saved ? app.text("ui.world.save.ok") : app.text("ui.menu.save.failed") + saved.error;
        saveNoticeSeconds_ = kSaveNoticeSeconds;
        return true;
    }

    // 主菜单（施工图 1.5：Tab / Esc / X）。与存盘同一个口径：只在世界层受理——
    // 上面那道「脚本在跑就不受理」已经把对话、剧情挡在外头。
    if (eng.keyPressed(engine::Engine::Key::Menu) || eng.keyPressed(engine::Engine::Key::Cancel)) {
        app.openMainMenu();
        return true;
    }

    if (eng.keyPressed(engine::Engine::Key::Confirm)) {
        interact(app);
        return true;
    }

    // 路径行动（E / Q，施工图 1.5）。钩子没接时 pathAction 直接返回 false，这一键就是没响。
    if (eng.keyPressed(engine::Engine::Key::Action)) {
        pathAction(app);
        return true;
    }

    if (stepCooldown_ > 0.0) return true;

    int dx = 0;
    int dy = 0;
    if (eng.keyDown(engine::Engine::Key::Left)) dx = -1;
    else if (eng.keyDown(engine::Engine::Key::Right)) dx = 1;
    else if (eng.keyDown(engine::Engine::Key::Up)) dy = -1;
    else if (eng.keyDown(engine::Engine::Key::Down)) dy = 1;

    if (dx != 0 || dy != 0) {
        tryStep(app, dx, dy);
        stepCooldown_ = kStepCooldownSeconds;
    }
    return true;
}

int WorldScene::facingFromName(const std::string& name) {
    if (name == "up") return 0;
    if (name == "right") return 1;
    if (name == "left") return 3;
    // 「down」与任何写错的值都落在这里。朝下是站在原地看向玩家的姿势，
    // 一个拼错的 facing 因此只是「他没转身」，而不是一个面朝不存在方向的人。
    return 2;
}

void WorldScene::describe(Application& app, const rules::PortalLink* hop) {
    frame_.clear();
    const core::TileMap* map = app.currentMap();
    if (map == nullptr) return;
    const core::GameState& state = app.state();

    // 目标在别的图上时，通往它的**下一道门**要认得出来（portalLeadsToGoal）。
    //
    // 从前这里写着「只认直接通到目标图的那一道，不做跨图寻路」，理由是多走一张图的
    // 代价小于一条算错的路线。第 4 章复验（N-7）量出来那个代价不是一张图，是七张：
    // 第 3 章终局的谷外到七玄门各堂，中间没有一道门亮。跨图寻路现在是一个纯函数
    //（rules::nextPortalToward，只走此刻开着的门），有真地图的测试钉着它指的是哪道门。

    for (const core::MapObject& object : map->objects) {
        if (object.type == "npc") {
            // 在场判定与走位、对话共用同一个 visibleNpcAt 背后那一条：
            // 画不画、谈不谈得上、占不占格，永远是同一个答案。
            if (!npcVisible(state, object)) continue;
            WorldFrame::Npc npc;
            npc.object = &object;
            npc.roleId = object.property("role_id");
            // 名字取自 data/roles，查不到就回显 role_id——画面上一眼看出
            // 是哪个角色没进 data，这与 speakerName 在对话框里的口径相同。
            npc.name = app.speakerName(npc.roleId);
            npc.facing = facingFromName(object.property("facing"));
            npc.bubble = gPathBubbleProbe != nullptr ? gPathBubbleProbe(app, object) : PathBubble::None;
            frame_.npcs.push_back(std::move(npc));
        } else if (object.type == "portal") {
            // 闸门开着还是关着必须看得出来。走上去被拦一次才知道，
            // 在一张有三个出口的图上就是三次试错。
            const std::string need = object.property("require_flag");
            const bool open = need.empty() || state.flag(need) != 0;
            // 通往目标那张图的门用目标色（金）。**闸门关着时不高亮**：
            // 一道亮着却过不去的门，比一道不亮的门更让人反复去撞。
            frame_.portals.push_back(WorldFrame::Portal{&object, open, open && portalIsGuide(object, hop)});
        } else if (object.type == "facility") {
            // 九种设施各有各的样子（物件精灵；缺图时是底板配色 + 一个字），都来自 kind。
            frame_.facilities.push_back(WorldFrame::Facility{&object, object.property("kind")});
        } else if (object.type == "trigger") {
            // 只标「按键才响」的那一种，而且只在它当真还能响的时候标。
            // 踏入型不标：那种是走过去自然发生的，提前标出来等于剧透。
            if (object.property("mode") != "interact") continue;
            if (!triggerReady(state, object)) continue;
            frame_.hotspots.push_back(&object);
        }
    }

    if (const core::Objective* step = objectiveOf(app)) {
        frame_.objectiveText = app.text(step->textKey);
        frame_.objectiveWhere = objectiveWhereText(app, hop);
        if (step->targetMap == map->id) {
            // 目标对象不在这张图上（数据写错，或这一步指的是别处）时**什么都不标**，
            // 不要退回去在屏幕中央画个箭头——一个指着不存在的东西的箭头，比没有箭头
            // 更能把人带到沟里。门禁那边 tools/validate.py 不许出现这种目标。
            for (const core::MapObject& object : map->objects) {
                if (object.name == step->targetObject) {
                    frame_.objectiveTarget = &object;
                    break;
                }
            }
        }
    }
    frame_.mapTitle = app.text(map->displayNameKey);
    // 地名下那一行危险度（有遭遇的图才有，见 Application::dangerStars）。
    frame_.dangerStars = app.dangerStars();
    // 存盘那一句：画在屏幕右上角，不跟目标框抢地方，几秒后自己消失。
    if (saveNoticeSeconds_ > 0.0) frame_.saveNotice = saveNotice_;
}

const core::Objective* WorldScene::objectiveOf(Application& app) {
    return rules::currentObjective(app.data().objectives, app.state());
}

namespace {

// 地名，不是地图 id（见 core::mapDisplayNameKey）。推不出 key 时回显 id，
// 画面上一眼看出是哪张图没登记名字。
[[nodiscard]] std::string mapName(Application& app, const std::string& mapId) {
    const std::string key = core::mapDisplayNameKey(mapId);
    return key.empty() ? mapId : app.text(key);
}

}  // namespace

const rules::PortalLink* WorldScene::guidePortal(Application& app) {
    const core::Objective* step = objectiveOf(app);
    const core::TileMap* map = app.currentMap();
    if (step == nullptr || map == nullptr || step->targetMap == map->id) return nullptr;
    return rules::nextPortalToward(app.portalLinks(), map->id, step->targetMap, app.state());
}

bool WorldScene::portalLeadsToGoal(Application& app, const core::MapObject& portal) {
    return portalIsGuide(portal, guidePortal(app));
}

bool WorldScene::portalIsGuide(const core::MapObject& portal, const rules::PortalLink* hop) {
    if (portal.type != "portal") return false;
    // 名字与格子都要对上：同一张图上两道门同名是门禁该拦的事，但这里不靠它——
    // 靠它的话，两道同名门会一起亮，玩家看到的是两条互相矛盾的路。
    return hop != nullptr && portal.name == hop->portalName && portal.position == hop->position;
}

std::string WorldScene::objectiveWhereText(Application& app) {
    return objectiveWhereText(app, guidePortal(app));
}

std::string WorldScene::objectiveWhereText(Application& app, const rules::PortalLink* hop) {
    const core::Objective* step = objectiveOf(app);
    const core::TileMap* map = app.currentMap();
    // 目标就在这张图上时不报地名：那时地名是废话，头上那个标记才是答案。
    if (step == nullptr || map == nullptr || step->targetMap == map->id) return {};
    std::string where = "前往　" + mapName(app, step->targetMap);
    // 下一道门不直接通到目标图时，再说一句先去哪。只报下一站，不报整条路：
    // 整条路是一串七个地名，玩家读不完，读完了也记不住；下一站配上那道亮着的门就够了。
    if (hop != nullptr && hop->targetMap != step->targetMap) {
        where += "　先到　" + mapName(app, hop->targetMap);
    }
    return where;
}

void WorldScene::render(Application& app) {
    if (app.currentMap() == nullptr) return;
    // 本帧的下一道门只算一次（一次全图最短路），门的高亮与左上角那一行都用它。
    const rules::PortalLink* hop = guidePortal(app);
    describe(app, hop);
    // 怎么画（烘焙图 / 旧图元、精灵、光照、粒子、后处理、HUD、换图淡入）全在 WorldView。
    // 目标标记、名牌压在最上面（后处理之后）：它们是「现在去哪」「这是谁」的唯一答案，
    // 遮挡可以盖住人，不能盖住这两样。
    view_.render(app, frame_);
}

}  // namespace fanren::game
