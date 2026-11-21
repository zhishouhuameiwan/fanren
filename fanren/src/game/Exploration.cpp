#include "game/Exploration.h"

#include <memory>
#include <vector>

#include "core/rules/Companions.h"
#include "game/Application.h"
#include "game/DialogueScene.h"
#include "game/ItemToasts.h"
#include "game/ShopScene.h"
#include "game/WorldObjects.h"

namespace fanren::game::explore {
namespace {

// 把文案里的 slot 换成 value（与 WorldScene.cpp 里那个 fill 同一个口径）。
std::string fill(std::string text, const std::string& slot, const std::string& value) {
    for (std::size_t at = text.find(slot); at != std::string::npos; at = text.find(slot, at + value.size())) {
        text.replace(at, slot.size(), value);
    }
    return text;
}

std::string fraction(Application& app, const char* key, int have, int total) {
    return fill(fill(app.text(key), "{n}", std::to_string(have)), "{total}", std::to_string(total));
}

}  // namespace

Tally tally(const core::GameState& state, const core::WorldObjectState& objects, const core::TileMap& map) {
    Tally out;
    for (const core::MapObject& object : map.objects) {
        if (object.type != worldobj::kChest && object.type != worldobj::kGather) continue;
        if (!worldobj::present(state, objects, object)) continue;
        if (object.type == worldobj::kChest) {
            ++out.chests;
            if (worldobj::chestOpened(objects, object)) ++out.opened;
        } else {
            ++out.gathers;
            if (objects.gatheredDay.count(object.property("id")) != 0) ++out.gathered;
        }
    }
    return out;
}

Tally currentTally(Application& app) {
    const core::TileMap* map = app.currentMap();
    if (map == nullptr || app.inLilian()) return {};
    return tally(app.state(), app.worldObjects(), *map);
}

std::string progressLine(Application& app, const Tally& t) {
    if (!t.counted()) return {};
    constexpr const char* kSep = " · ";
    std::string line;
    if (t.chests > 0) line = fraction(app, "ui.world.explore.chests", t.opened, t.chests);
    if (t.gathers > 0) {
        if (!line.empty()) line += kSep;
        line += fraction(app, "ui.world.explore.gathers", t.gathered, t.gathers);
    }
    if (t.complete()) line += kSep + app.text("ui.world.explore.complete");
    return line;
}

bool settle(Application& app) {
    const core::TileMap* map = app.currentMap();
    if (map == nullptr || app.inLilian()) return false;
    const core::ExplorationReward* reward = app.explorationReward(map->id);
    if (reward == nullptr) return false;
    core::GameState& state = app.state();
    if (state.flag(reward->doneFlag) != 0) return false;
    if (!tally(state, app.worldObjects(), *map).complete()) return false;

    // 先记账再发：旗标一置，这张图就不会再发第二回（读档后也一样——旗标进存档）。
    state.setFlag(reward->doneFlag);
    std::vector<worldobj::ItemStack> stacks;
    if (reward->spiritStones > 0) stacks.push_back({kSpiritStoneItemId, reward->spiritStones});
    for (const core::BagEntry& entry : reward->items) stacks.push_back({entry.itemId, entry.count});
    worldobj::giveItems(state, app.data(), stacks);
    std::string items;
    for (const worldobj::ItemStack& stack : stacks) {
        const ItemToast card = makeItemToast(app.data(), state, stack.itemId, stack.count);
        if (!items.empty()) items += "、";
        items += card.name + (stack.count > 1 ? "×" + std::to_string(stack.count) : std::string{});
        app.announceItem(stack.itemId, stack.count);
    }

    // 默契：此刻上场的伙伴各加 bond（跟着走的就是上场的那几位）。不受「并肩每天至多 +6」管——那是打仗的账。
    std::string names;
    if (reward->bond > 0) {
        const auto& companions = app.data().companions;
        for (const std::string& role : worldobj::fieldedCompanionRoles(state, companions)) {
            const std::string who = rules::companionOfRole(companions, role);
            if (who.empty()) continue;
            rules::addBond(state.bonds[who], reward->bond, rules::bondCapOf(companions, who, state));
            if (!names.empty()) names += "、";
            names += app.speakerName(role);
        }
    }

    const std::string place = app.text(map->displayNameKey);
    std::string body = items.empty() ? fill(app.text("ui.world.explore.done"), "{place}", place)
                                     : fill(fill(app.text("ui.world.explore.done.items"), "{place}", place), "{items}",
                                            items);
    if (!names.empty()) body += fill(app.text("ui.world.explore.bond"), "{names}", names);
    app.pushDialogueLine(std::string{}, body);
    app.pushScene(std::make_unique<DialogueScene>(std::string{}, body));
    return true;
}

}  // namespace fanren::game::explore
