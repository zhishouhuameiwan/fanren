#include "game/ItemInfoView.h"

#include <algorithm>
#include <filesystem>
#include <set>
#include <utility>

#include "engine/TextLayout.h"
#include "game/Application.h"
#include "game/ChapterCardScene.h"
#include "game/ItemToasts.h"
#include "game/Wording.h"
#include "game/WorldObjects.h"
#include "io/DataLoader.h"
#include "ui/Widgets.h"

namespace fanren::game {

namespace {

namespace fs = std::filesystem;

std::string joined(const std::vector<std::string>& lines, const std::string& sep) {
    std::string out;
    for (const std::string& line : lines) {
        if (line.empty()) continue;
        if (!out.empty()) out += sep;
        out += line;
    }
    return out;
}

std::string label(const core::GameData& data, const char* key) {
    return data.lookupText(std::string("ui.iteminfo.label.") + key);
}

void addSpot(rules::ItemSpots& spots, std::set<std::pair<std::string, std::string>>& seen, const std::string& itemId,
             rules::ItemSpot spot) {
    const std::string tag = spot.mapId + "#" + std::to_string(static_cast<int>(spot.kind));
    if (!seen.insert({itemId, tag}).second) return;
    spots[itemId].push_back(std::move(spot));
}

std::string mapNameOf(const core::GameData& data, const std::string& mapId, const std::string& key) {
    const std::string k = key.empty() ? core::mapDisplayNameKey(mapId) : key;
    if (k.empty()) return mapId;
    const auto it = data.text.find(k);
    return it == data.text.end() ? mapId : it->second;
}

}  // namespace

rules::ItemSpots buildItemSpots(const std::string& assetRoot, const core::GameData& data,
                                const std::map<std::string, core::ExplorationReward>& rewards) {
    rules::ItemSpots spots;
    std::set<std::pair<std::string, std::string>> seen;
    std::map<std::string, int> chapterOf;
    std::error_code ec;
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(fs::path(assetRoot) / "maps", ec)) {
        if (entry.path().extension() == ".tmj") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    for (const fs::path& file : files) {
        auto loaded = io::loadTileMap(file.string());
        if (!loaded) continue;
        const core::TileMap& map = loaded.value;
        chapterOf[map.id] = map.chapter;
        const std::string name = mapNameOf(data, map.id, map.displayNameKey);
        for (const core::MapObject& object : map.objects) {
            const bool chest = object.type == worldobj::kChest;
            const bool gather = object.type == worldobj::kGather;
            if (!chest && !gather) continue;
            bool ok = false;
            const auto items = worldobj::parseItemList(object.property(chest ? "loot" : "item"), ok);
            if (!ok) continue;
            for (const worldobj::ItemStack& stack : items) {
                addSpot(spots, seen, stack.itemId,
                        rules::ItemSpot{chest ? rules::ItemSpot::Kind::Chest : rules::ItemSpot::Kind::Gather, map.id,
                                        name, map.chapter});
            }
        }
    }
    for (const auto& [mapId, reward] : rewards) {
        const auto chapter = chapterOf.find(mapId);
        for (const core::BagEntry& entry : reward.items) {
            addSpot(spots, seen, entry.itemId,
                    rules::ItemSpot{rules::ItemSpot::Kind::Reward, mapId, mapNameOf(data, mapId, {}),
                                    chapter == chapterOf.end() ? 0 : chapter->second});
        }
    }
    return spots;
}

rules::InfoWords infoWords(const core::GameState& state) {
    const PanelStage stage = wordingStage(state);
    rules::InfoWords words;
    words.mp = mpWord(stage);
    words.currency = currencyName(stage);
    return words;
}

int currentChapter(Application& app) {
    int chapter = 0;
    for (const ChapterEntry& entry : app.chapterTable().chapters) {
        chapter = entry.number;
        if (!app.chapterTable().chapterDone(app.state(), entry.number)) break;
    }
    if (const core::TileMap* map = app.currentMap()) chapter = std::max(chapter, map->chapter);
    return std::max(chapter, app.state().chapter);
}

rules::ItemInfo itemInfoOf(Application& app, const std::string& itemId, int herbAge) {
    rules::ItemInfoSources sources;
    sources.chapter = currentChapter(app);
    sources.recipes = &app.recipes();
    sources.shops = &app.shops();
    sources.battles = &app.battles();
    sources.spots = &app.itemSpots();
    return rules::describeItem(app.data(), sources, &app.state(), infoWords(app.state()), itemId, herbAge);
}

rules::MagicInfo magicInfoOf(Application& app, const core::Magic& magic) {
    return rules::describeMagic(app.data(), magic, infoWords(app.state()));
}

std::string itemEffectLine(Application& app, const std::string& itemId, int herbAge) {
    // 效果只看物品自己的字段，不必建来源索引：只拿效果那一类（来源、用途的几张表都不给）。
    const rules::ItemInfo info =
        rules::describeItem(app.data(), rules::ItemInfoSources{}, nullptr, infoWords(app.state()), itemId, herbAge);
    return info.effects.empty() ? std::string{} : info.effects.front();
}

std::string itemEffectSummary(Application& app, const std::string& itemId) {
    return rules::effectSummary(
        rules::describeItem(app.data(), rules::ItemInfoSources{}, nullptr, infoWords(app.state()), itemId, 0));
}

std::string magicEffectSummary(Application& app, const core::Magic& magic) {
    const rules::MagicInfo info = magicInfoOf(app, magic);
    return rules::magicSummary(info);
}

std::vector<DetailRow> itemDetailRows(const core::GameData& data, const rules::ItemInfo& info,
                                      const std::string& flavor, bool withPrice) {
    const std::string sep = "；";
    std::vector<DetailRow> rows;
    if (!info.effects.empty()) rows.push_back({label(data, "effect"), joined(info.effects, sep)});
    if (!info.uses.empty()) rows.push_back({label(data, "use"), joined(info.uses, sep)});
    if (!info.sources.empty()) rows.push_back({label(data, "source"), joined(info.sources, sep)});
    const std::string& value = withPrice ? info.value : info.grade;
    if (!value.empty()) rows.push_back({label(data, "value"), value});
    if (!flavor.empty()) {
        rows.push_back({{}, {}, true});
        rows.push_back({{}, flavor});
    }
    return rows;
}

std::vector<DetailRow> magicDetailRows(const core::GameData& data, const rules::MagicInfo& info,
                                       const std::string& flavor) {
    std::vector<DetailRow> rows;
    rows.push_back({label(data, "category"), info.category});
    rows.push_back({label(data, "cost"), info.cost});
    if (!info.lines.empty()) rows.push_back({label(data, "detail"), joined(info.lines, "；")});
    if (!flavor.empty()) {
        rows.push_back({{}, {}, true});
        rows.push_back({{}, flavor});
    }
    return rows;
}

void drawDetailBlock(Application& app, const engine::Rect& area, const DetailBlock& block, int fontSize) {
    const std::string& title = block.title;
    const engine::Color& color = block.color;
    const std::vector<DetailRow>& rows = block.rows;
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const int left = area.x + theme.padding;
    const int width = area.w - theme.padding * 2;
    const int bottom = area.y + area.h - theme.padding / 2;
    if (width <= 0 || area.h <= 0) return;
    int y = area.y + theme.padding / 2;
    const int titleSize = fontSize + 4;
    if (!title.empty()) {
        e.drawText(title, left, y, titleSize, color, theme.titleStyle);
        y += titleSize + 10;
    }
    const auto measure = [&e, fontSize](const std::string& sample) { return e.measureText(sample, fontSize).x; };
    int labelW = 0;
    for (const DetailRow& row : rows) {
        if (!row.label.empty()) labelW = std::max(labelW, measure(row.label));
    }
    const int gap = labelW > 0 ? 14 : 0;
    const int lineH = fontSize + std::max(4, theme.lineSpacing - 2);
    for (const DetailRow& row : rows) {
        if (y + fontSize > bottom) break;
        if (row.rule) {
            ui::drawRuleH(e, static_cast<float>(left), static_cast<float>(y + 4), static_cast<float>(width), theme);
            y += 12;
            continue;
        }
        const bool flavor = row.label.empty();
        const int textX = flavor ? left : left + labelW + gap;
        const int textW = flavor ? width : width - labelW - gap;
        if (textW <= 0) continue;
        if (!flavor) e.drawText(row.label, left, y, fontSize, theme.gold, theme.bodyStyle);
        for (const engine::LayoutLine& line : engine::layoutText(row.text, textW, measure)) {
            if (y + fontSize > bottom) break;
            if (!line.text.empty()) {
                e.drawText(line.text, textX, y, fontSize, flavor ? theme.paperDim : theme.paper, theme.bodyStyle);
            }
            y += lineH;
        }
        y += flavor ? 0 : 3;
    }
}

DetailBlock itemDetailBlock(Application& app, const std::string& itemId, int herbAge, const std::string& extra,
                            bool withPrice) {
    DetailBlock block;
    const core::Item* item = app.data().findItem(itemId);
    if (item == nullptr) return block;
    std::string flavor = item->descKey.empty() ? std::string{} : app.text(item->descKey);
    if (!extra.empty()) flavor += (flavor.empty() ? "" : "\n") + extra;
    block.title = item->name.empty() ? itemId : item->name;
    if (herbAge > 0) block.title += "（" + std::to_string(herbAge) + " 年）";
    block.color = app.theme().gradeColors[std::clamp(gradeTier(item->grade), 0, 4)];
    block.rows = itemDetailRows(app.data(), itemInfoOf(app, itemId, herbAge), flavor, withPrice);
    return block;
}

DetailBlock magicDetailBlock(Application& app, const core::Magic& magic) {
    DetailBlock block;
    const std::string flavor = magic.descKey.empty() ? std::string{} : app.text(magic.descKey);
    block.title = magic.name;
    block.color = app.theme().goldBright;
    block.rows = magicDetailRows(app.data(), magicInfoOf(app, magic), flavor);
    return block;
}

const DetailBlock& DetailCache::item(Application& app, const std::string& itemId, int herbAge, const std::string& extra,
                                     bool withPrice) {
    const std::string key = "item:" + itemId + "#" + std::to_string(herbAge) + "#" + extra + (withPrice ? "#$" : "");
    if (key != key_) {
        block_ = itemDetailBlock(app, itemId, herbAge, extra, withPrice);
        key_ = key;
    }
    return block_;
}

const DetailBlock& DetailCache::magic(Application& app, const core::Magic& magic) {
    const std::string key = "magic:" + magic.id;
    if (key != key_) {
        block_ = magicDetailBlock(app, magic);
        key_ = key;
    }
    return block_;
}

}  // namespace fanren::game
