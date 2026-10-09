#include "core/model/Types.h"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace fanren::core {

const Item* GameData::findItem(const std::string& id) const {
    const auto it = items.find(id);
    return it == items.end() ? nullptr : &it->second;
}

const Magic* GameData::findMagic(const std::string& id) const {
    const auto it = magics.find(id);
    return it == magics.end() ? nullptr : &it->second;
}

const RoleTemplate* GameData::findRole(const std::string& id) const {
    const auto it = roles.find(id);
    return it == roles.end() ? nullptr : &it->second;
}

std::string GameData::lookupText(const std::string& key) const {
    const auto it = text.find(key);
    // 缺文案时回显 key 本身：画面上立刻能看出漏了哪条，比显示空串好定位。
    return it == text.end() ? key : it->second;
}

int GameState::flag(const std::string& name) const {
    const auto it = flags.find(name);
    return it == flags.end() ? 0 : it->second;
}

void GameState::setFlag(const std::string& name, int value) {
    flags[name] = value;
}

int GameState::itemCount(const std::string& itemId) const {
    int total = 0;
    for (const BagEntry& entry : bag) {
        if (entry.itemId == itemId) total += entry.count;
    }
    return total;
}

int GameState::itemCountOfAge(const std::string& itemId, int herbAge) const {
    int total = 0;
    for (const BagEntry& entry : bag) {
        if (entry.itemId == itemId && entry.herbAge == herbAge) total += entry.count;
    }
    return total;
}

void GameState::addItem(const std::string& itemId, int count, int herbAge) {
    if (count <= 0) return;
    // 灵草按年份分堆：年份不同价格与药效都不同，不能合并计数。
    const auto it = std::find_if(bag.begin(), bag.end(), [&](const BagEntry& e) {
        return e.itemId == itemId && e.herbAge == herbAge;
    });
    if (it != bag.end()) {
        it->count += count;
        return;
    }
    bag.push_back(BagEntry{itemId, count, herbAge});
}

bool GameState::removeItem(const std::string& itemId, int count) {
    if (count <= 0) return true;
    if (itemCount(itemId) < count) return false;

    int remaining = count;
    // 先扣年份低的，把高年份留给玩家：低年份灵草价值低，符合玩家预期。
    std::sort(bag.begin(), bag.end(), [](const BagEntry& a, const BagEntry& b) {
        return a.herbAge < b.herbAge;
    });
    for (BagEntry& entry : bag) {
        if (entry.itemId != itemId || remaining == 0) continue;
        const int take = std::min(entry.count, remaining);
        entry.count -= take;
        remaining -= take;
    }
    bag.erase(std::remove_if(bag.begin(), bag.end(),
                             [](const BagEntry& e) { return e.count <= 0; }),
              bag.end());
    return remaining == 0;
}

bool GameState::removeItemOfAge(const std::string& itemId, int count, int herbAge) {
    if (count <= 0) return true;
    // 先问够不够再动手：扣到一半发现不够就只能回滚，而回滚背包比多查一次贵。
    if (itemCountOfAge(itemId, herbAge) < count) return false;

    int remaining = count;
    // 同一 (itemId, herbAge) 正常只有一条（addItem 会合并），但老存档或将来
    // 的合并逻辑出岔子时可能有几条，这里按条扣干净而不是假定只有一条。
    for (BagEntry& entry : bag) {
        if (entry.itemId != itemId || entry.herbAge != herbAge || remaining == 0) continue;
        const int take = std::min(entry.count, remaining);
        entry.count -= take;
        remaining -= take;
    }
    bag.erase(std::remove_if(bag.begin(), bag.end(),
                             [](const BagEntry& e) { return e.count <= 0; }),
              bag.end());
    return remaining == 0;
}

int GameState::itemCountAtLeastAge(const std::string& itemId, int minAge) const {
    int total = 0;
    for (const BagEntry& entry : bag) {
        if (entry.itemId == itemId && entry.herbAge >= minAge) total += entry.count;
    }
    return total;
}

bool GameState::removeItemAtLeastAge(const std::string& itemId, int count, int minAge) {
    if (count <= 0) return true;
    // 全有或全无：先问够不够再动手（与 removeItemOfAge 同一个理由）。交药是一句「够不够」，
    // 不够却先扣掉几株，脚本走的是「你还没凑齐」那一支，药却已经少了。
    if (itemCountAtLeastAge(itemId, minAge) < count) return false;

    // 够格的堆按年份从低到高扣：够格里最嫩的先交，更老的留给玩家（契约 4.1）。
    // 只排够格那几堆的下标，不动背包里别的堆。
    std::vector<std::size_t> eligible;
    for (std::size_t i = 0; i < bag.size(); ++i) {
        if (bag[i].itemId == itemId && bag[i].herbAge >= minAge) eligible.push_back(i);
    }
    std::stable_sort(eligible.begin(), eligible.end(),
                     [this](std::size_t a, std::size_t b) { return bag[a].herbAge < bag[b].herbAge; });
    int remaining = count;
    for (const std::size_t index : eligible) {
        if (remaining == 0) break;
        const int take = std::min(bag[index].count, remaining);
        bag[index].count -= take;
        remaining -= take;
    }
    bag.erase(std::remove_if(bag.begin(), bag.end(),
                             [](const BagEntry& e) { return e.count <= 0; }),
              bag.end());
    return remaining == 0;
}

rules::SpiritField* GameState::findField(const std::string& fieldId) {
    for (rules::SpiritField& field : fields) {
        if (field.id == fieldId) return &field;
    }
    return nullptr;
}

std::string MapObject::property(const std::string& key) const {
    const auto it = properties.find(key);
    return it == properties.end() ? std::string{} : it->second;
}

bool TileMap::inBounds(Point p) const {
    return p.x >= 0 && p.y >= 0 && p.x < width && p.y < height;
}

bool TileMap::walkable(Point p) const {
    if (!inBounds(p)) return false;
    const auto index = static_cast<std::size_t>(p.y) * static_cast<std::size_t>(width) +
                       static_cast<std::size_t>(p.x);
    if (index >= collision.size()) return false;
    return collision[index] == 0;
}

const MapObject* TileMap::objectAt(Point p, const std::string& type) const {
    for (const MapObject& object : objects) {
        if (!type.empty() && object.type != type) continue;
        const bool hit = p.x >= object.position.x && p.x < object.position.x + object.width &&
                         p.y >= object.position.y && p.y < object.position.y + object.height;
        if (hit) return &object;
    }
    return nullptr;
}

std::string mapDisplayNameKey(const std::string& mapId) {
    const std::size_t sep = mapId.find('_');
    // 没有下划线的 id 不符合命名规范（map_spec 第 6 节），推不出 key 来。
    // 回一个空串让调用方原样显示 id：屏幕上看见一个 ch02_wairentang，
    // 比看见一个凭空拼出来的、查不到的 key 更容易查。
    if (sep == std::string::npos || sep + 1 >= mapId.size()) return {};
    return mapId.substr(0, sep) + ".map." + mapId.substr(sep + 1) + ".name";
}

const MapObject* TileMap::defaultSpawn() const {
    const MapObject* first = nullptr;
    for (const MapObject& object : objects) {
        if (object.type != "spawn") continue;
        if (first == nullptr) first = &object;
        if (object.property("default") == "true") return &object;
    }
    // 没有标记 default 的地图属于 map_spec 违规，validate.py 会拦下；
    // 运行期退而取第一个 spawn，避免直接崩在玩家面前。
    return first;
}

}  // namespace fanren::core
