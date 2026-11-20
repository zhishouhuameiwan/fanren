#include "core/rules/ItemInfo.h"

#include <algorithm>
#include <cstddef>
#include <set>

#include "core/battle/Battle.h"
#include "core/model/AttackCategory.h"
#include "core/rules/Companions.h"
#include "core/rules/Roster.h"

namespace fanren::rules {

namespace {

using core::GameData;

std::string word(const GameData& data, const std::string& key) {
    return data.lookupText("ui.iteminfo." + key);
}

std::string num(int n) {
    return std::to_string(n);
}

// 名字列成「甲、乙、丙」；多于 limit 个时写前 limit 个再接「等 N 处」。
std::string listNames(const GameData& data, const std::vector<std::string>& names, std::size_t limit) {
    std::string out;
    for (std::size_t i = 0; i < names.size() && i < limit; ++i) {
        if (i > 0) out += word(data, "sep");
        out += "「" + names[i] + "」";
    }
    if (names.size() > limit) out += fillTemplate(word(data, "more"), {{"n", num(static_cast<int>(names.size()))}});
    return out;
}

void pushUnique(std::vector<std::string>& names, const std::string& name) {
    if (name.empty()) return;
    if (std::find(names.begin(), names.end(), name) == names.end()) names.push_back(name);
}

// 剧透闸：给了 state 时，章号大于当前章（gate）的不列。章号 0 = 不设闸。
bool visibleChapter(const core::GameState* state, int gate, int chapter) {
    return state == nullptr || chapter <= 0 || chapter <= gate;
}

std::string craftWord(const GameData& data, CraftKind kind) {
    switch (kind) {
        case CraftKind::Alchemy: return word(data, "craft.alchemy");
        case CraftKind::Talisman: return word(data, "craft.talisman");
        case CraftKind::Forge: return word(data, "craft.forge");
        case CraftKind::Formation: return word(data, "craft.formation");
    }
    return {};
}

bool recipeOpen(const Recipe& recipe, const core::GameState* state) {
    return state == nullptr || recipe.requireFlag.empty() || state->flag(recipe.requireFlag) != 0;
}

std::string recipeLabel(const GameData& data, const Recipe& recipe) {
    if (const core::Item* product = data.findItem(recipe.productId); product != nullptr && !product->name.empty()) {
        return product->name;
    }
    return recipe.name.empty() ? recipe.id : recipe.name;
}

// 符箓施展的那门法术：「施展「火弹术」· 火 · 威力 12」。
std::string castLine(const GameData& data, const core::Magic& magic) {
    std::string line = fillTemplate(word(data, "effect.cast"), {{"name", magic.name}});
    const std::string cats = core::categoryNames(core::magicCategories(magic));
    if (magic.effect == core::MagicEffect::Reveal) {
        line += word(data, "dot") + word(data, "magic.reveal");
    } else if (magic.effect == core::MagicEffect::Stagger) {
        line += word(data, "dot") + fillTemplate(word(data, "magic.stagger"), {{"n", num(magic.stagger)}});
    } else {
        if (!cats.empty()) line += word(data, "dot") + cats;
        if (magic.power > 0) line += word(data, "dot") + fillTemplate(word(data, "magic.power"), {{"n", num(magic.power)}});
        if (magic.target == core::MagicTarget::All) line += word(data, "dot") + word(data, "magic.all");
    }
    return line;
}

void addEffects(const GameData& data, const core::Item& item, const InfoWords& words, int herbAge, ItemInfo& info) {
    if (item.restoreHp > 0) {
        info.effects.push_back(fillTemplate(word(data, "effect.hp"), {{"hp", words.hp}, {"n", num(item.restoreHp)}}));
    }
    if (item.restoreMp > 0) {
        info.effects.push_back(fillTemplate(word(data, "effect.mp"), {{"mp", words.mp}, {"n", num(item.restoreMp)}}));
    }
    if (item.curesPoison) info.effects.push_back(word(data, "effect.cure"));
    if (item.poison > 0) {
        info.effects.push_back(fillTemplate(word(data, "effect.poison"),
                                            {{"turns", num(item.poison)}, {"n", num(item.poisonPower)}}));
    }
    if (!item.castMagic.empty()) {
        if (const core::Magic* magic = data.findMagic(item.castMagic)) {
            info.effects.push_back(castLine(data, *magic));
        } else {
            info.effects.push_back(fillTemplate(word(data, "effect.cast"), {{"name", item.castMagic}}));
        }
        info.effects.push_back(fillTemplate(word(data, "effect.cast_free"), {{"mp", words.mp}}));
    }
    if (item.weapon != 0) {
        info.effects.push_back(fillTemplate(word(data, "effect.weapon"), {{"cat", core::categoryNames(item.weapon)}}));
    }
    if (item.kind == core::ItemKind::Herb) {
        if (herbAge > 0 && item.maxAge > 0) {
            info.effects.push_back(
                fillTemplate(word(data, "effect.herb_age"), {{"age", num(herbAge)}, {"max", num(item.maxAge)}}));
        } else if (herbAge > 0) {
            info.effects.push_back(fillTemplate(word(data, "effect.herb_age_only"), {{"age", num(herbAge)}}));
        } else if (item.maxAge > 0) {
            info.effects.push_back(fillTemplate(word(data, "effect.herb_max"), {{"max", num(item.maxAge)}}));
        }
    }
}

void addUses(const GameData& data, const ItemInfoSources& sources, const core::GameState* state,
             const core::Item& item, ItemInfo& info) {
    if (sources.recipes != nullptr) {
        for (const auto& [id, recipe] : *sources.recipes) {
            if (!recipeOpen(recipe, state)) continue;
            int count = 0;
            int minAge = 0;
            for (const Ingredient& input : recipe.inputs) {
                if (input.itemId != item.id) continue;
                count += input.count;
                minAge = std::max(minAge, input.minAge);
            }
            if (count <= 0) continue;
            std::string line = fillTemplate(word(data, "use.recipe"), {{"recipe", "「" + recipeLabel(data, recipe) + "」"},
                                                                       {"n", num(count)},
                                                                       {"craft", craftWord(data, recipe.kind)}});
            if (minAge > 0) line += fillTemplate(word(data, "use.min_age"), {{"age", num(minAge)}});
            info.uses.push_back(std::move(line));
        }
    }
    if (core::battleUsable(item)) info.uses.push_back(word(data, "use.battle"));
    if (state != nullptr) {
        std::vector<std::string> likes;
        std::vector<std::string> dislikes;
        for (const std::string& companionId : roster(data.companions, *state)) {
            const auto it = data.companions.find(companionId);
            if (it == data.companions.end()) continue;
            const core::Companion& companion = it->second;
            const std::string roleId = currentRole(data.companions, companionId, *state);
            const core::RoleTemplate* role = data.findRole(roleId);
            const std::string name = role != nullptr && !role->name.empty() ? role->name : companionId;
            const auto has = [&item](const std::vector<std::string>& list) {
                return std::find(list.begin(), list.end(), item.id) != list.end();
            };
            if (has(companion.giftsLike)) pushUnique(likes, name);
            if (has(companion.giftsDislike)) pushUnique(dislikes, name);
        }
        if (!likes.empty()) info.uses.push_back(fillTemplate(word(data, "use.like"), {{"names", listNames(data, likes, 4)}}));
        if (!dislikes.empty()) {
            info.uses.push_back(fillTemplate(word(data, "use.dislike"), {{"names", listNames(data, dislikes, 4)}}));
        }
    }
}

void addSources(const GameData& data, const ItemInfoSources& sources, const core::GameState* state,
                const core::Item& item, ItemInfo& info) {
    const int gate = sources.chapter > 0 ? sources.chapter : (state != nullptr ? state->chapter : 0);
    if (sources.shops != nullptr) {
        std::vector<std::string> shops;
        for (const auto& [id, shop] : *sources.shops) {
            if (!visibleChapter(state, gate, shopChapter(id))) continue;
            const bool sells = std::any_of(shop.entries.begin(), shop.entries.end(),
                                           [&item](const ShopEntry& e) { return e.itemId == item.id; });
            if (!sells) continue;
            pushUnique(shops, shop.nameKey.empty() ? id : data.lookupText(shop.nameKey));
        }
        if (!shops.empty()) info.sources.push_back(fillTemplate(word(data, "source.shop"), {{"names", listNames(data, shops, 2)}}));
    }
    if (sources.recipes != nullptr) {
        std::vector<std::string> recipes;
        for (const auto& [id, recipe] : *sources.recipes) {
            if (recipe.productId != item.id || !recipeOpen(recipe, state)) continue;
            pushUnique(recipes, recipe.name.empty() ? id : recipe.name);
        }
        if (!recipes.empty()) {
            info.sources.push_back(fillTemplate(word(data, "source.recipe"), {{"names", listNames(data, recipes, 2)}}));
        }
    }
    if (sources.battles != nullptr) {
        std::vector<std::string> foes;
        for (const auto& [id, battle] : *sources.battles) {
            if (!visibleChapter(state, gate, battle.chapter)) continue;
            const bool drops = std::any_of(battle.reward.drops.begin(), battle.reward.drops.end(),
                                           [&item](const core::BagEntry& e) { return e.itemId == item.id && e.count > 0; });
            if (!drops) continue;
            for (const core::BattleUnitSpec& unit : battle.units) {
                if (unit.ally) continue;
                const core::RoleTemplate* role = data.findRole(unit.roleId);
                pushUnique(foes, role != nullptr && !role->name.empty() ? role->name : unit.roleId);
            }
        }
        if (!foes.empty()) info.sources.push_back(fillTemplate(word(data, "source.drop"), {{"names", listNames(data, foes, 3)}}));
    }
    if (sources.spots != nullptr) {
        if (const auto it = sources.spots->find(item.id); it != sources.spots->end()) {
            std::vector<std::string> chests;
            std::vector<std::string> gathers;
            std::vector<std::string> rewards;
            for (const ItemSpot& spot : it->second) {
                if (!visibleChapter(state, gate, spot.chapter)) continue;
                const std::string name = spot.mapName.empty() ? spot.mapId : spot.mapName;
                switch (spot.kind) {
                    case ItemSpot::Kind::Chest: pushUnique(chests, name); break;
                    case ItemSpot::Kind::Gather: pushUnique(gathers, name); break;
                    case ItemSpot::Kind::Reward: pushUnique(rewards, name); break;
                }
            }
            if (!gathers.empty()) {
                info.sources.push_back(fillTemplate(word(data, "source.gather"), {{"names", listNames(data, gathers, 3)}}));
            }
            if (!chests.empty()) {
                info.sources.push_back(fillTemplate(word(data, "source.chest"), {{"names", listNames(data, chests, 3)}}));
            }
            if (!rewards.empty()) {
                info.sources.push_back(fillTemplate(word(data, "source.reward"), {{"names", listNames(data, rewards, 2)}}));
            }
        }
    }
    std::vector<std::string> quests;
    for (const core::Quest& quest : data.quests) {
        if (quest.kind != core::QuestKind::Side || !visibleChapter(state, gate, quest.chapter)) continue;
        const bool pays = std::any_of(quest.rewards.begin(), quest.rewards.end(),
                                      [&item](const core::BagEntry& e) { return e.itemId == item.id && e.count > 0; });
        if (pays) pushUnique(quests, quest.titleKey.empty() ? quest.id : data.lookupText(quest.titleKey));
    }
    if (!quests.empty()) info.sources.push_back(fillTemplate(word(data, "source.quest"), {{"names", listNames(data, quests, 2)}}));
    if (info.sources.empty()) info.sources.push_back(word(data, "source.story"));
}

int tierOf(int grade) {
    if (grade <= 1) return 0;
    return std::min(4, grade / 2);
}

}  // namespace

std::string fillTemplate(std::string text, const std::vector<std::pair<std::string, std::string>>& values) {
    for (const auto& [name, value] : values) {
        const std::string slot = "{" + name + "}";
        for (std::size_t at = text.find(slot); at != std::string::npos; at = text.find(slot, at + value.size())) {
            text.replace(at, slot.size(), value);
        }
    }
    return text;
}

int shopChapter(const std::string& shopId) {
    if (shopId.size() < 5 || shopId.compare(0, 2, "ch") != 0 || shopId[4] != '_') return 0;
    const char a = shopId[2];
    const char b = shopId[3];
    if (a < '0' || a > '9' || b < '0' || b > '9') return 0;
    return (a - '0') * 10 + (b - '0');
}

std::string gradeName(const GameData& data, int grade) {
    return word(data, "tier." + num(tierOf(grade)));
}

ItemInfo describeItem(const GameData& data, const ItemInfoSources& sources, const core::GameState* state,
                      const InfoWords& words, const std::string& itemId, int herbAge) {
    ItemInfo info;
    const core::Item* item = data.findItem(itemId);
    if (item == nullptr) return info;
    addEffects(data, *item, words, herbAge, info);
    addUses(data, sources, state, *item, info);
    addSources(data, sources, state, *item, info);

    std::string value = gradeName(data, item->grade);
    if (item->grade > 0) value += fillTemplate(word(data, "value.grade"), {{"n", num(item->grade)}});
    value += word(data, "dot");
    value += item->tradeable
                 ? fillTemplate(word(data, "value.price"), {{"n", num(item->price)}, {"currency", words.currency}})
                 : word(data, "value.untradeable");
    info.value = std::move(value);
    return info;
}

std::string effectSummary(const ItemInfo& info) {
    std::string out;
    for (const std::string& line : info.effects) {
        if (!out.empty()) out += " · ";
        out += line;
    }
    return out;
}

MagicInfo describeMagic(const GameData& data, const core::Magic& magic, const InfoWords& words) {
    MagicInfo info;
    const std::string cats = core::categoryNames(core::magicCategories(magic));
    if (magic.effect == core::MagicEffect::Reveal) {
        info.category = word(data, "magic.cat.reveal");
    } else if (magic.effect == core::MagicEffect::Stagger) {
        info.category = word(data, "magic.cat.stagger");
    } else if (!core::castableMagic(magic)) {
        info.category = word(data, "magic.cat.support");
    } else if (!cats.empty()) {
        info.category = cats;
    } else {
        info.category = word(data, "magic.cat.plain");
    }
    info.cost = fillTemplate(word(data, "magic.cost"), {{"mp", words.mp}, {"n", num(magic.needMp)}});

    if (magic.effect == core::MagicEffect::Reveal) {
        info.lines.push_back(word(data, "magic.reveal_full"));
    } else if (magic.effect == core::MagicEffect::Stagger) {
        info.lines.push_back(fillTemplate(word(data, "magic.stagger_full"), {{"n", num(magic.stagger)}}));
    }
    if (magic.power > 0) {
        std::string line = fillTemplate(word(data, "magic.power"), {{"n", num(magic.power)}});
        line += word(data, "dot") + word(data, magic.target == core::MagicTarget::All ? "magic.all" : "magic.single");
        info.lines.push_back(std::move(line));
    }
    if (magic.poison > 0) {
        info.lines.push_back(fillTemplate(word(data, "effect.poison"),
                                          {{"turns", num(magic.poison)}, {"n", num(magic.poisonPower)}}));
    }
    if (!core::castableMagic(magic)) {
        info.lines.push_back(word(data, "magic.no_battle"));
    } else if (magic.effect != core::MagicEffect::None) {
        info.lines.push_back(word(data, "magic.no_boost"));
    } else if (magic.boost == core::MagicBoost::Hits) {
        info.lines.push_back(fillTemplate(word(data, "magic.boost_hits"), {{"n", num(1 + core::battle::kBoostMax)}}));
    } else {
        info.lines.push_back(fillTemplate(word(data, "magic.boost_power"), {{"n", num(1 + core::battle::kBoostMax)}}));
    }
    return info;
}

std::string magicSummary(const MagicInfo& info) {
    std::string out = info.category;
    for (const std::string& line : info.lines) {
        if (!out.empty()) out += " · ";
        out += line;
    }
    return out;
}

}  // namespace fanren::rules
