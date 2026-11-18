#include "game/WorldObjects.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <utility>

#include "core/rules/Companions.h"

namespace fanren::game::worldobj {
namespace {

// 非负整数属性；没写、写坏了都是 fallback。门禁拦写坏的，这里只是不崩。
int intProperty(const core::MapObject& object, const std::string& key, int fallback) {
    const std::string text = object.property(key);
    if (text.empty() || text.size() > 9) return fallback;
    int value = 0;
    for (const char c : text) {
        if (std::isdigit(static_cast<unsigned char>(c)) == 0) return fallback;
        value = value * 10 + (c - '0');
    }
    return value;
}

bool covers(const core::MapObject& object, core::Point cell) {
    // 与 core::TileMap::objectAt、WorldScene::visibleNpcAt 逐字相同的矩形命中。
    return cell.x >= object.position.x && cell.x < object.position.x + object.width &&
           cell.y >= object.position.y && cell.y < object.position.y + object.height;
}

std::string trim(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) --end;
    return text.substr(begin, end - begin);
}

int herbAgeOf(const core::GameData& data, const std::string& itemId) {
    const core::Item* item = data.findItem(itemId);
    return item == nullptr ? 0 : item->herbAge;
}

}  // namespace

bool isWorldObjectType(const std::string& type) {
    return type == kChest || type == kGather || type == kFoe || type == kGate;
}

std::vector<ItemStack> parseItemList(const std::string& text, bool& ok) {
    ok = true;
    std::vector<ItemStack> items;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t comma = text.find(',', start);
        const std::string piece = trim(text.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
        ItemStack stack;
        const std::size_t colon = piece.find(':');
        stack.itemId = trim(piece.substr(0, colon));
        if (colon != std::string::npos) {
            const std::string count = trim(piece.substr(colon + 1));
            if (count.empty() || count.size() > 6) ok = false;
            int value = 0;
            for (const char c : count) {
                if (std::isdigit(static_cast<unsigned char>(c)) == 0) ok = false;
                else value = value * 10 + (c - '0');
            }
            stack.count = value;
        }
        if (stack.itemId.empty() || stack.count <= 0) ok = false;
        items.push_back(std::move(stack));
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    if (!ok) items.clear();
    return items;
}

bool bagCanTake(const core::GameState& state, const core::GameData& data, const std::vector<ItemStack>& items) {
    // 同一样东西清单里写了两回，要合在一起算：两回各自看都放得下，合起来可能就放不下。
    std::map<std::pair<std::string, int>, long long> adding;
    for (const ItemStack& stack : items) adding[{stack.itemId, herbAgeOf(data, stack.itemId)}] += stack.count;
    for (const auto& [key, count] : adding) {
        const long long have = state.itemCountOfAge(key.first, key.second);
        if (have + count > kBagStackCap) return false;
    }
    return true;
}

void giveItems(core::GameState& state, const core::GameData& data, const std::vector<ItemStack>& items) {
    for (const ItemStack& stack : items) state.addItem(stack.itemId, stack.count, herbAgeOf(data, stack.itemId));
}

bool requirementMet(const core::GameState& state, const core::MapObject& object) {
    const std::string need = object.property("require_flag");
    return need.empty() || state.flag(need) != 0;
}

bool chestOpened(const core::WorldObjectState& objects, const core::MapObject& chest) {
    return objects.openedChests.count(chest.property("id")) != 0;
}

bool gatherDepleted(const core::WorldObjectState& objects, const core::MapObject& gather, int today) {
    const auto it = objects.gatheredDay.find(gather.property("id"));
    if (it == objects.gatheredDay.end()) return false;
    // respawn_days 是必填；写坏了按 1 天（明天就能再采），不至于让一个采集点永远废掉。
    const int days = std::max(1, intProperty(gather, "respawn_days", 1));
    return today - it->second < days;
}

bool foePresent(const core::GameState& state, const core::WorldObjectState& objects, const core::MapObject& foe) {
    if (!requirementMet(state, foe)) return false;
    const auto it = objects.clearedFoes.find(foe.property("id"));
    if (it == objects.clearedFoes.end()) return true;
    // 缺省不重生（契约 2.4）：没写 respawn_days、或写 0，清掉了就一直不在。
    const int days = intProperty(foe, "respawn_days", 0);
    return days > 0 && state.day - it->second >= days;
}

bool gateOpen(const core::WorldObjectState& objects, const core::MapObject& gate) {
    return objects.openedGates.count(gate.property("id")) != 0;
}

bool present(const core::GameState& state, const core::WorldObjectState& objects, const core::MapObject& object) {
    if (object.type == kChest || object.type == kGather) return requirementMet(state, object);
    if (object.type == kFoe) return foePresent(state, objects, object);
    if (object.type == kGate) return true;   // 机关一直在：关着的样子、开着的样子
    return false;
}

bool blocks(const core::GameState& state, const core::WorldObjectState& objects, const core::MapObject& object) {
    if (!present(state, objects, object)) return false;
    return object.type != kGate || !gateOpen(objects, object);
}

const core::MapObject* presentAt(const core::GameState& state, const core::WorldObjectState& objects,
                                 const core::TileMap& map, core::Point cell, const std::string& type) {
    for (const core::MapObject& object : map.objects) {
        if (object.type != type || !covers(object, cell)) continue;
        if (!present(state, objects, object)) continue;
        if (object.type == kGate && gateOpen(objects, object)) continue;
        return &object;
    }
    return nullptr;
}

const core::MapObject* blockerAt(const core::GameState& state, const core::WorldObjectState& objects,
                                 const core::TileMap& map, core::Point cell) {
    for (const core::MapObject& object : map.objects) {
        if (!isWorldObjectType(object.type) || !covers(object, cell)) continue;
        if (blocks(state, objects, object)) return &object;
    }
    return nullptr;
}

core::Point foeFrontCell(const core::MapObject& foe) {
    core::Point cell = foe.position;
    const std::string facing = foe.property("facing");
    if (facing == "up") cell.y -= 1;
    else if (facing == "right") cell.x += 1;
    else if (facing == "left") cell.x -= 1;
    else cell.y += 1;
    return cell;
}

const core::MapObject* foeWatching(const core::GameState& state, const core::WorldObjectState& objects,
                                   const core::TileMap& map, core::Point cell) {
    for (const core::MapObject& object : map.objects) {
        if (object.type != kFoe || !foePresent(state, objects, object)) continue;
        if (foeFrontCell(object) == cell) return &object;
    }
    return nullptr;
}

bool companionInParty(const core::GameState& state, const Companions& companions, const std::string& companionId) {
    if (companionId.empty()) return false;
    for (const core::PartyMember& member : state.party) {
        if (rules::companionOfRole(companions, member.roleId) == companionId) return true;
    }
    return false;
}

std::vector<std::string> fieldedCompanionRoles(const core::GameState& state, const Companions& companions) {
    std::vector<std::string> roles;
    for (const core::PartyMember& member : state.party) {
        if (roles.size() >= kFieldedCompanions) break;
        if (!member.active || rules::companionOfRole(companions, member.roleId).empty()) continue;
        roles.push_back(member.roleId);
    }
    return roles;
}

std::string partyFieldSkillRole(const core::GameState& state, const Companions& companions,
                                const std::string& skill) {
    if (skill.empty()) return {};
    for (const core::PartyMember& member : state.party) {
        const std::string who = rules::companionOfRole(companions, member.roleId);
        const auto it = companions.find(who);
        if (it != companions.end() && it->second.fieldSkill == skill) return member.roleId;
    }
    return {};
}

std::string rosterFieldSkillHolder(const core::GameState& state, const Companions& companions,
                                   const std::string& skill) {
    if (skill.empty()) return {};
    for (const auto& [id, companion] : companions) {
        if (companion.fieldSkill != skill) continue;
        if (!companion.metFlag.empty() && state.flag(companion.metFlag) != 0) return id;
    }
    return {};
}

namespace {

// 从 from 走向 to 的朝向（同 GameState::facing）；两格相同时为 -1。
int facingToward(core::Point from, core::Point to) {
    if (to.y < from.y) return 0;
    if (to.x > from.x) return 1;
    if (to.y > from.y) return 2;
    if (to.x < from.x) return 3;
    return -1;
}

}  // namespace

std::vector<FollowerPlace> layoutFollowers(const std::vector<core::Point>& steps, core::Point hero, int heroFacing,
                                           std::size_t count) {
    std::vector<FollowerPlace> places;
    places.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        FollowerPlace place;
        place.cell = steps.empty() ? hero : steps[std::min(i, steps.size() - 1)];
        // 面朝前面那个人（第 0 位面朝主角）：走路时就是行进的方向。叠在同一格上时照主角的朝向。
        const core::Point ahead = (i == 0 || places.empty()) ? hero : places[i - 1].cell;
        const int toward = facingToward(place.cell, ahead);
        place.facing = toward >= 0 ? toward : heroFacing;
        places.push_back(place);
    }
    return places;
}

}  // namespace fanren::game::worldobj
