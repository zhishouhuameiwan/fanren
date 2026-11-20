#include "game/WorldScene.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "core/rules/Companions.h"
#include "core/rules/Objectives.h"
#include "game/Application.h"
#include "game/DialogueScene.h"
#include "game/PathActionScene.h"
#include "game/WorldObjects.h"

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

bool WorldScene::triggerReady(const core::GameState& state, const core::MapObject& trigger,
                              const Companions& companions) {
    // 插话（契约 3.8）：只收伙伴 id。她用的是哪个角色 id 都算在队里——经 companionOfRole 认人。
    // 写成角色 id 的由门禁拦（companionInParty 在这里找不到那位「伙伴」，触发器就一直不响）。
    const std::string need = trigger.property("require_party");
    if (!need.empty() && !worldobj::companionInParty(state, companions, need)) return false;
    return triggerReady(state, trigger);
}

bool WorldScene::npcPresent(const core::GameState& state, const core::MapObject& npc,
                            const Companions& companions) {
    if (!npcVisible(state, npc)) return false;
    // 一人一身（契约 3.11）：她已在队里，图上就不能再站着一个她。只对伙伴生效——
    // companionOfRole 对傀儡、化身、路人给空串，第 10 章化身既是 NPC 又入队的现状不变。
    const std::string who = rules::companionOfRole(companions, npc.property("role_id"));
    return who.empty() || !worldobj::companionInParty(state, companions, who);
}

const core::MapObject* WorldScene::visibleNpcAt(const core::GameState& state,
                                                const core::TileMap& map, core::Point cell) {
    return visibleNpcAt(state, map, cell, Companions{});
}

const core::MapObject* WorldScene::visibleNpcAt(const core::GameState& state, const core::TileMap& map,
                                                core::Point cell, const Companions& companions) {
    for (const core::MapObject& object : map.objects) {
        if (object.type != "npc") continue;
        // 与 core::TileMap::objectAt 逐字相同的矩形命中——两处的形状必须一样，
        // 差一格就等于「看得见的和站得上的不是同一个东西」。
        const bool hit = cell.x >= object.position.x &&
                         cell.x < object.position.x + object.width &&
                         cell.y >= object.position.y &&
                         cell.y < object.position.y + object.height;
        if (!hit) continue;
        if (!npcPresent(state, object, companions)) continue;   // 多问的就是这一句
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
    const Companions& companions = app.data().companions;
    syncTrail(app);

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
    if (visibleNpcAt(state, *map, target, companions) != nullptr) return false;
    if (map->objectAt(target, "facility") != nullptr) return false;
    // 宝箱、采集点、明雷占格；机关关着时占格、开了不占（契约 2.4）。在不在场、开没开问 worldobj，
    // 状态经 Application::worldObjects()——历练里是「这一趟」那份。会走的明雷按它此刻那一格（foePoses）。
    const worldobj::FoePoses* poses = foePoses(app);
    if (worldobj::blockerAt(state, app.worldObjects(), *map, target, poses) != nullptr) return false;

    const core::Point from = state.position;
    state.position = target;
    // 足迹**紧跟着**换格记（契约 3.7、第 4 节）：下面明雷、触发器、传送三条分支都会提前返回，
    // 记晚了，踩响剧情的那一步、过门的那一步就漏了。
    recordStep(from, target);

    // 明雷：踏进它面前那一格就被看见，直接开打，不加不减（契约 3.4）。排在触发器与传送点前面：
    // 被看见是这一步最先发生的事。逃跑成功退回 from——开打前站的那一格。
    // 会走的明雷按它此刻的格与朝向；它们不站在传送点、踏入触发区旁边（FoePatrol），面前那一格压不着它们。
    if (const core::MapObject* foe = worldobj::foeWatching(state, app.worldObjects(), *map, target, poses)) {
        engageFoe(app, *foe, /*preemptive=*/false, from);
        return true;
    }

    // 踏入型触发：走到即发事件。
    //
    // 这里不再预写任何记号。once 兑现与否由脚本末尾那句 flag.set 说了算
    // （见 triggerReady 的注释）：脚本半路 return 就等于「这一幕没演」，
    // 触发器留着，玩家满足条件再走回来还能触发。
    if (const core::MapObject* trigger = map->objectAt(target, "trigger")) {
        if (trigger->property("mode") == "enter" && triggerReady(state, *trigger, companions)) {
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
    const Companions& companions = app.data().companions;

    // 同样走 visibleNpcAt：不在场的那个不该抢走这一次确认键。从前用 objectAt，
    // 它只返回对象表里第一个命中的，可见性是命中**之后**才问的一句——于是一个
    // 隐身的 NPC 把同格那个该出面的压得死死的，玩家按确认毫无反应。
    if (const core::MapObject* npc = visibleNpcAt(app.state(), *map, front, companions)) {
        const std::string script = npc->property("script");
        if (!script.empty()) {
            app.startEvent(script);
            return true;
        }
    }
    if (const core::MapObject* trigger = map->objectAt(front, "trigger")) {
        if (trigger->property("mode") == "interact" && triggerReady(app.state(), *trigger, companions)) {
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
    // 四种新对象排在最后，依次宝箱 → 采集点 → 明雷 → 机关（门禁 validate.py 的 INTERACT_ORDER 同此）。
    // 每一类只取这一格上第一个在场的（开了的机关不算）。
    const core::GameState& state = app.state();
    core::WorldObjectState& objects = app.worldObjects();
    const worldobj::FoePoses* poses = foePoses(app);
    if (const core::MapObject* chest = worldobj::presentAt(state, objects, *map, front, worldobj::kChest)) {
        return openChest(app, *chest);
    }
    if (const core::MapObject* gather = worldobj::presentAt(state, objects, *map, front, worldobj::kGather)) {
        return gatherAt(app, *gather);
    }
    if (const core::MapObject* foe = worldobj::presentAt(state, objects, *map, front, worldobj::kFoe, poses)) {
        // 站在它的侧面或背后、面朝它按确认是偷袭（契约 3.4）。站在它面前那一格的人照理进格时
        // 就被看见了；读档、逃跑、它重生时人恰好站在那里，按确认就是一场普通的仗。
        // 会走的明雷按它此刻的朝向：它背对着你走、或张望转开了头，从后面摸上去照样是偷袭。
        const bool preemptive = !(worldobj::foeFrontCell(*foe, poses) == state.position);
        engageFoe(app, *foe, preemptive, state.position);
        return true;
    }
    if (const core::MapObject* gate = worldobj::presentAt(state, objects, *map, front, worldobj::kGate)) {
        return operateGate(app, *gate);
    }
    // 面前什么都没有时，再看脚下：按键型的触发区常常画在一片空地上（崖下两尸那一格），
    // 人走到目标圈里、面朝哪边都有可能。只认面前那一格，玩家站在金圈正中按确认毫无反应，
    // 只会以为卡关了（2026-11-20 用户试玩第 7 章）。
    if (const core::MapObject* here = map->objectAt(state.position, "trigger")) {
        if (here->property("mode") == "interact" && triggerReady(state, *here, companions)) {
            app.startEvent(here->property("script"));
            return true;
        }
    }
    return false;
}

void WorldScene::syncTrail(Application& app) {
    const core::TileMap* map = app.currentMap();
    const core::Point hero = app.state().position;
    if (map == trailMap_ && hero == trailHead_) return;
    trail_.clear();
    trailMap_ = map;
    trailHead_ = hero;
}

void WorldScene::recordStep(core::Point from, core::Point to) {
    trail_.insert(trail_.begin(), from);
    if (trail_.size() > worldobj::kFieldedCompanions) trail_.resize(worldobj::kFieldedCompanions);
    trailHead_ = to;
}

namespace {

// 一句旁白：与 Application::showMessage 同一个样子（空说话人、进对话记录），只是正文是拼好的——
// 宝箱里有什么、机关要谁来开，名字要填进去，showMessage 只收文案 key。
void narrate(Application& app, const std::string& body) {
    app.pushDialogueLine(std::string{}, body);
    app.pushScene(std::make_unique<DialogueScene>(std::string{}, body));
}

// 把文案里的 slot（{items} / {name}）换成 value。
std::string fill(std::string text, const std::string& slot, const std::string& value) {
    for (std::size_t at = text.find(slot); at != std::string::npos; at = text.find(slot, at + value.size())) {
        text.replace(at, slot.size(), value);
    }
    return text;
}

// 「火灵石×2、清灵草」：物品名取自 data/items，查不到回显 id（画面上一眼看出谁没进 data）。
std::string describeItems(Application& app, const std::vector<worldobj::ItemStack>& items) {
    std::string out;
    for (const worldobj::ItemStack& stack : items) {
        if (!out.empty()) out += "、";
        const core::Item* item = app.data().findItem(stack.itemId);
        out += item != nullptr && !item->name.empty() ? item->name : stack.itemId;
        if (stack.count > 1) out += "×" + std::to_string(stack.count);
    }
    return out;
}

// 不在队里的伙伴此刻的名字：她现在该用的那个角色的名字（问名之前叫「白衣女子」，之后叫「南宫婉」）。
std::string companionName(Application& app, const std::string& companionId) {
    const std::string role = rules::currentRole(app.data().companions, companionId, app.state());
    return app.speakerName(role.empty() ? companionId : role);
}

}  // namespace

bool WorldScene::openChest(Application& app, const core::MapObject& chest) {
    core::WorldObjectState& objects = app.worldObjects();
    if (worldobj::chestOpened(objects, chest)) {
        app.showMessage("ui.world.chest.empty");
        return true;
    }
    bool ok = false;
    const std::vector<worldobj::ItemStack> loot = worldobj::parseItemList(chest.property("loot"), ok);
    // 背包装不下：说一句，箱子照旧关着（契约 3.5）。loot 写坏了同样不开——门禁拦这种写法，
    // 这里不发半张清单、也不把箱子记成开过（记了玩家就永远拿不到那份东西了）。
    if (!ok || !worldobj::bagCanTake(app.state(), app.data(), loot)) {
        app.showMessage("ui.world.bag.full");
        return true;
    }
    worldobj::giveItems(app.state(), app.data(), loot);
    objects.openedChests.insert(chest.property("id"));
    app.noteItemsGiven();
    narrate(app, fill(app.text("ui.world.chest.got"), "{items}", describeItems(app, loot)));
    return true;
}

bool WorldScene::gatherAt(Application& app, const core::MapObject& gather) {
    core::WorldObjectState& objects = app.worldObjects();
    const int today = app.state().day;
    if (worldobj::gatherDepleted(objects, gather, today)) {
        app.showMessage("ui.world.gather.spent");
        return true;
    }
    bool ok = false;
    const std::vector<worldobj::ItemStack> haul = worldobj::parseItemList(gather.property("item"), ok);
    if (!ok || !worldobj::bagCanTake(app.state(), app.data(), haul)) {
        app.showMessage("ui.world.bag.full");
        return true;
    }
    worldobj::giveItems(app.state(), app.data(), haul);
    objects.gatheredDay[gather.property("id")] = today;
    app.noteItemsGiven();
    narrate(app, fill(app.text("ui.world.gather.got"), "{items}", describeItems(app, haul)));
    return true;
}

bool WorldScene::operateGate(Application& app, const core::MapObject& gate) {
    const Companions& companions = app.data().companions;
    const std::string skill = gate.property("skill");
    // 队里任意一位伙伴会这手（契约 3.6），上不上场都算：场地技不是战斗。
    // 名字取她在队里实际用的那个角色（图上跟着的就是她），不经 currentRole。
    if (const std::string role = worldobj::partyFieldSkillRole(app.state(), companions, skill); !role.empty()) {
        app.worldObjects().openedGates.insert(gate.property("id"));
        // opens 是永久旗标：历练里只记在「这一趟」，不许落进旗标（门禁也不许历练图写 opens）。
        if (const std::string opens = gate.property("opens"); !opens.empty() && !app.inLilian()) {
            app.state().setFlag(opens);
        }
        // 「一小段演出」：开门声 + 一句她出手的旁白。
        app.engine().playSfx("door_open");
        narrate(app, fill(app.text("ui.world.gate.open"), "{name}", app.speakerName(role)));
        return true;
    }
    // 没人会：名册里已有会这手的人，就点她的名；还没有，就只说要别的法子。她此刻不在队里，
    // 没有「实际的角色 id」可问，只能问 currentRole（契约 2.1 许可的第三处）。
    if (const std::string who = worldobj::rosterFieldSkillHolder(app.state(), companions, skill); !who.empty()) {
        narrate(app, fill(app.text("ui.world.gate.need"), "{name}", companionName(app, who)));
        return true;
    }
    // hint（可选）是这道机关自己的说法，替换掉那句通用的「似乎要用别的法子」。
    const std::string hint = gate.property("hint");
    app.showMessage(hint.empty() ? std::string{"ui.world.gate.unknown"} : hint);
    return true;
}

void WorldScene::startFoeBattle(Application& app, const core::MapObject& foe, bool preemptive,
                                core::Point returnCell) {
    // 回调里不留任何指向地图或场景的指针：仗打完时图可能已经换了（打输回出生点、历练整趟退出）。
    const std::string foeId = foe.property("id");
    const std::string mapId = app.state().mapId;
    app.engine().playSfx("encounter");
    // foe.battle 原样递过去：历练里按境界拼档的后缀是 Application::startBattle 的事（契约 3.12）。
    app.startBattle(BattleStart{foe.property("battle"), preemptive},
                    [&app, foeId, mapId, returnCell](const script::CommandResult& result) {
                        if (result.battleWon) {
                            // 记进此刻的那一份：历练里是「这一趟」的账，出来就扔。
                            app.worldObjects().clearedFoes[foeId] = app.state().day;
                            return;
                        }
                        if (result.code == "escaped") {
                            // 逃跑成功：它留在原地，主角退回开打前站的那一格（契约 3.4）。
                            app.state().position = returnCell;
                            return;
                        }
                        if (result.code != "lost") return;   // 敌人跑了：它还在，人就地不动
                        // 打输：在历练里整趟退出（3.12）；不在历练里回这张图的缺省出生点（3.4，
                        // 明雷的编成一律不致命，门禁钉着）。退出失败（原图读不回来）时也退到出生点，
                        // 不把人留在一场打输了的仗的原地。
                        if (app.inLilian()) {
                            const auto left = app.leaveLilian();
                            if (left.ok) return;
                            std::fprintf(stderr, "[world] WARNING: 历练里打输，退出历练失败（%s），退到 %s 的出生点\n",
                                         left.error.c_str(), mapId.c_str());
                        }
                        if (const auto back = app.loadMap(mapId, std::string{}); !back.ok) {
                            std::fprintf(stderr, "[world] WARNING: 明雷打输后回不了 %s 的出生点：%s\n", mapId.c_str(),
                                         back.error.c_str());
                        }
                    });
}

void WorldScene::engageFoe(Application& app, const core::MapObject& foe, bool preemptive, core::Point returnCell) {
    // 先让会走的明雷收手：在追的放弃回去，回到世界层之后几秒内谁也不看人、不因走到面前开打——
    // 逃跑回来、敌人跑了，人都还站在原地，不给这一段，同一只（或旁边那只）转眼又是一仗。
    foes_.onBattle();
    startFoeBattle(app, foe, preemptive, returnCell);
}

const worldobj::FoePoses* WorldScene::foePoses(const Application& app) const {
    return foes_.posesFor(app.currentMap());
}

bool WorldScene::advanceFoes(Application& app, double deltaSeconds) {
    const core::TileMap* map = app.currentMap();
    if (map == nullptr) return false;
    foes_.sync(map);
    const core::GameState& state = app.state();
    const core::WorldObjectState& objects = app.worldObjects();
    const Companions& companions = app.data().companions;
    // 明雷走不走得上去、看不看得穿，与主角走位问的是同一组问题（tryStep）：在场的 NPC、设施、四种新对象
    // （明雷按此刻站位）。墙、主角那一格、传送点与踏入触发区那一圈由 FoePatrol 自己管。跟随的伙伴不占格，不算。
    foepatrol::Surroundings around;
    around.hero = state.position;
    around.solid = [&](core::Point cell) {
        return visibleNpcAt(state, *map, cell, companions) != nullptr || map->objectAt(cell, "facility") != nullptr ||
               worldobj::blockerAt(state, objects, *map, cell, foes_.posesFor(map)) != nullptr;
    };
    around.present = [&](const core::MapObject& foe) { return worldobj::foePresent(state, objects, foe); };
    const core::MapObject* caught = foes_.advance(around, deltaSeconds);
    if (caught == nullptr) return false;
    // 追上了：主角正站在它面前那一格——与「迈进面前那一格」同一判据，不算先手。逃跑成功就站回原地（这一格）。
    engageFoe(app, *caught, /*preemptive=*/false, state.position);
    return true;
}

bool WorldScene::pathAction(Application& app) {
    const core::TileMap* map = app.currentMap();
    if (map == nullptr || gPathActionOpener == nullptr) return false;
    // 与 tryStep / interact 同一道闸，理由见 tryStep。
    if (app.scripts().isRunning() || app.awaitingCommand()) return false;
    // 找人与 interact 同一个口径：不在场的人不应声。
    const core::MapObject* npc = visibleNpcAt(app.state(), *map, facingCell(app.state()), app.data().companions);
    if (npc == nullptr) return false;
    return gPathActionOpener(app, *npc);
}

std::string WorldScene::pathActionHint(Application& app) const {
    const core::TileMap* map = app.currentMap();
    if (map == nullptr || gPathActionOpener == nullptr || app.topScene() != this) return {};
    if (app.scripts().isRunning() || app.awaitingCommand()) return {};
    const core::MapObject* npc = visibleNpcAt(app.state(), *map, facingCell(app.state()), app.data().companions);
    if (npc == nullptr) return {};

    std::string labels;
    core::PathActionKind previous = core::PathActionKind::Inquire;
    // pathActionsFor 与菜单共用按种类排序的清单，同一种行动只提示一次。
    for (const core::PathAction* action : app.pathActionsFor(npc->name)) {
        if (!labels.empty() && action->kind == previous) continue;
        if (!labels.empty()) labels += " / ";
        labels += PathActionScene::rowFor(app.data(), *action).label;
        previous = action->kind;
    }
    return labels.empty() ? std::string{} : app.text("ui.world.path.key") + " " + labels;
}

std::string WorldScene::interactHint(Application& app, core::Point& cell, const core::MapObject*& npcOut) const {
    cell = core::Point{-1, -1};
    npcOut = nullptr;
    const core::TileMap* map = app.currentMap();
    if (map == nullptr || app.topScene() != this) return {};
    if (app.scripts().isRunning() || app.awaitingCommand()) return {};
    const core::GameState& state = app.state();
    const core::Point front = facingCell(state);
    const Companions& companions = app.data().companions;
    const core::WorldObjectState& objects = app.worldObjects();
    const worldobj::FoePoses* poses = foePoses(app);
    const auto hint = [&app](const char* verb) {
        return app.text("ui.world.interact.key") + " " + app.text(verb);
    };
    if (const core::MapObject* npc = visibleNpcAt(state, *map, front, companions)) {
        if (!npc->property("script").empty()) {
            cell = front;
            npcOut = npc;
            return hint("ui.world.interact.talk");
        }
    }
    if (const core::MapObject* trigger = map->objectAt(front, "trigger")) {
        if (trigger->property("mode") == "interact" && triggerReady(state, *trigger, companions)) {
            cell = front;
            return hint("ui.world.interact.examine");
        }
    }
    if (map->objectAt(front, "facility") != nullptr) {
        cell = front;
        return hint("ui.world.interact.use");
    }
    if (const core::MapObject* chest = worldobj::presentAt(state, objects, *map, front, worldobj::kChest)) {
        if (worldobj::chestOpened(objects, *chest)) return {};
        cell = front;
        return hint("ui.world.interact.open");
    }
    if (const core::MapObject* gather = worldobj::presentAt(state, objects, *map, front, worldobj::kGather)) {
        if (worldobj::gatherDepleted(objects, *gather, state.day)) return {};
        cell = front;
        return hint("ui.world.interact.gather");
    }
    if (const core::MapObject* foe = worldobj::presentAt(state, objects, *map, front, worldobj::kFoe, poses)) {
        cell = front;
        // 站在它面前那一格按确认是一场普通的仗，侧后才是偷袭（interact() 同一判据，会走的按它此刻的朝向）。
        return hint(worldobj::foeFrontCell(*foe, poses) == state.position ? "ui.world.interact.fight"
                                                                           : "ui.world.interact.ambush");
    }
    if (worldobj::presentAt(state, objects, *map, front, worldobj::kGate) != nullptr) {
        cell = front;
        return hint("ui.world.interact.gate");
    }
    if (const core::MapObject* here = map->objectAt(state.position, "trigger")) {
        if (here->property("mode") == "interact" && triggerReady(state, *here, companions)) {
            cell = state.position;
            return hint("ui.world.interact.examine");
        }
    }
    return {};
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

    // 明雷巡逻与追击（docs/ch07-playtest-foes.md）。排在上面那道「脚本在跑就不受理」之后：脚本、对话、菜单、
    // 战斗压着的时候它们一律停住（后三样压在栈上时 update 根本轮不到世界层）。排在按键之前：这一帧有一只
    // 追上了就开打，这一帧的按键不再受理。
    if (advanceFoes(app, deltaSeconds)) return true;

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
    // 剧情传送、打输回出生点这些「不是一步走过来的」挪动，要在画之前认出来，伙伴才叠回主角格上。
    syncTrail(app);
    const core::GameState& state = app.state();

    // 目标在别的图上时，通往它的**下一道门**要认得出来（portalLeadsToGoal）。
    //
    // 从前这里写着「只认直接通到目标图的那一道，不做跨图寻路」，理由是多走一张图的
    // 代价小于一条算错的路线。第 4 章复验（N-7）量出来那个代价不是一张图，是七张：
    // 第 3 章终局的谷外到七玄门各堂，中间没有一道门亮。跨图寻路现在是一个纯函数
    //（rules::nextPortalToward，只走此刻开着的门），有真地图的测试钉着它指的是哪道门。

    const Companions& companions = app.data().companions;
    const core::WorldObjectState& objects = app.worldObjects();
    // 会走的明雷认一下图：换了图（还没轮到 update）就是全体在图上那一格，与 update 里 sync 之后同一个样子。
    foes_.sync(map);
    const worldobj::FoePoses* poses = foes_.posesFor(map);
    for (const core::MapObject& object : map->objects) {
        if (object.type == "npc") {
            // 在场判定与走位、对话共用同一个 visibleNpcAt 背后那一条：
            // 画不画、谈不谈得上、占不占格，永远是同一个答案（含一人一身）。
            if (!npcPresent(state, object, companions)) continue;
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
            if (!triggerReady(state, object, companions)) continue;
            frame_.hotspots.push_back(&object);
        } else if (worldobj::isWorldObjectType(object.type)) {
            // 四种新对象：只放此刻在场的（require_flag 没置的、清掉了还没重生的明雷不放），
            // 判据与走位、交互同一组函数（worldobj）。
            if (!worldobj::present(state, objects, object)) continue;
            if (object.type == worldobj::kChest) {
                const bool opened = worldobj::chestOpened(objects, object);
                frame_.chests.push_back(WorldFrame::Chest{&object, opened});
                if (!opened) frame_.hotspots.push_back(&object);
            } else if (object.type == worldobj::kGather) {
                const bool depleted = worldobj::gatherDepleted(objects, object, state.day);
                frame_.gathers.push_back(WorldFrame::Gather{&object, depleted});
                if (!depleted) frame_.hotspots.push_back(&object);
            } else if (object.type == worldobj::kFoe) {
                // 站在哪、面朝哪按此刻的站位（会走的明雷），头顶冒不冒「!」、这一步滑多久看它在追还是在巡逻。
                const worldobj::FoePose pose = worldobj::foePose(object, poses);
                WorldFrame::Foe foe{&object, object.property("look"), pose.facing, object.property("elite") == "true"};
                foe.cell = pose.cell;
                if (const foepatrol::Walker* walker = foes_.find(map, object)) {
                    foe.alert = walker->alert;
                    foe.stepSeconds = static_cast<float>(walker->mode == foepatrol::Mode::Chase
                                                             ? foepatrol::kChaseStepSeconds
                                                             : foepatrol::kPatrolStepSeconds);
                }
                frame_.foes.push_back(std::move(foe));
            } else {
                frame_.gates.push_back(WorldFrame::Gate{&object, worldobj::gateOpen(objects, object)});
            }
        }
    }

    // 跟随（契约 3.7）：上场的伙伴按队伍次序排在主角身后，各站一格足迹。
    const std::vector<std::string> roles = worldobj::fieldedCompanionRoles(state, companions);
    const std::vector<worldobj::FollowerPlace> places =
        worldobj::layoutFollowers(trail_, state.position, state.facing, roles.size());
    for (std::size_t i = 0; i < roles.size(); ++i) {
        frame_.followers.push_back(WorldFrame::Follower{roles[i], places[i].cell, places[i].facing});
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
    frame_.pathActionHint = pathActionHint(app);
    frame_.interactHint = interactHint(app, frame_.interactCell, frame_.interactNpc);
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

const WorldFrame& WorldScene::describeFrame(Application& app) {
    describe(app, guidePortal(app));
    return frame_;
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
