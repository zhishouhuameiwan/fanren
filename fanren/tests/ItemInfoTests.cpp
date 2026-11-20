// 物品与法术说明四行式（第 7 章可玩性改版·第一期「地基包」路 M，docs/ch07-foundation.md 3.3、3.5 第 3、4 条）。
//
// 规则层 rules::describeItem / describeMagic 全由数据生成。这里钉三件事：
//   · 遍历 data/items 全部物品：每一样都生成得出来源行（都没有时是「剧情所得」）、品阶与价值行；
//     带回复、施法、兵刃、解毒、下毒字段的都有效果行；
//   · 遍历 data/magics：每一门都生成得出类别与耗法力；
//   · 几样有代表性的东西逐字对上（期望值照设计写死，不从被测代码推）；剧透闸、凡人篇用词。
// 宝箱与采集点的索引走 game 层的 buildItemSpots（扫真 maps/），与游戏里同一条路。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/ItemInfo.h"
#include "game/ItemInfoView.h"
#include "io/BattleLoader.h"
#include "io/DataLoader.h"
#include "io/ExplorationLoader.h"
#include "io/RecipeLoader.h"
#include "io/ShopLoader.h"

namespace {

namespace fs = std::filesystem;
using fanren::rules::describeItem;
using fanren::rules::describeMagic;
using fanren::rules::ItemInfo;
using fanren::rules::ItemInfoSources;

fs::path repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

struct Shipped {
    fanren::core::GameData data;
    std::map<std::string, fanren::rules::Recipe> recipes;
    std::map<std::string, fanren::rules::Shop> shops;
    std::map<std::string, fanren::core::BattleSetup> battles;
    fanren::rules::ItemSpots spots;

    [[nodiscard]] ItemInfoSources sources() const {
        ItemInfoSources s;
        s.recipes = &recipes;
        s.shops = &shops;
        s.battles = &battles;
        s.spots = &spots;
        return s;
    }
};

const Shipped& shipped() {
    static const Shipped s = [] {
        Shipped out;
        const fs::path root = repoRoot();
        auto data = fanren::io::loadGameData((root / "data").string());
        EXPECT_TRUE(data.ok) << data.error;
        out.data = data.value;
        auto recipes = fanren::io::loadRecipes((root / "data" / "recipes").string(), out.data);
        EXPECT_TRUE(recipes.ok) << recipes.error;
        out.recipes = recipes.value;
        auto shops = fanren::io::loadShops((root / "data" / "shops").string());
        EXPECT_TRUE(shops.ok) << shops.error;
        out.shops = shops.value;
        auto battles = fanren::io::loadBattles((root / "data" / "battles").string());
        EXPECT_TRUE(battles.ok) << battles.error;
        out.battles = battles.value;
        auto rewards = fanren::io::loadExplorationRewards((root / "data" / "exploration").string());
        EXPECT_TRUE(rewards.ok) << rewards.error;
        out.spots = fanren::game::buildItemSpots(root.string(), out.data, rewards.value);
        return out;
    }();
    return s;
}

bool anyContains(const std::vector<std::string>& lines, const std::string& needle) {
    return std::any_of(lines.begin(), lines.end(), [&](const std::string& l) { return l.find(needle) != std::string::npos; });
}

std::string dump(const std::vector<std::string>& lines) {
    std::string out;
    for (const std::string& l : lines) out += "[" + l + "]";
    return out;
}

// ===========================================================================
// 验收判据 3：遍历全部物品
// ===========================================================================

TEST(ItemInfo, EveryItemHasASourceAndAValueLine) {
    const Shipped& s = shipped();
    ASSERT_GT(s.data.items.size(), 200u) << "data/items 读全了";
    for (const auto& [id, item] : s.data.items) {
        const ItemInfo info = describeItem(s.data, s.sources(), nullptr, {}, id);
        EXPECT_FALSE(info.sources.empty()) << id << " 没有来源行";
        for (const std::string& line : info.sources) EXPECT_FALSE(line.empty()) << id;
        EXPECT_FALSE(info.value.empty()) << id << " 没有品阶与价值行";
        EXPECT_EQ(info.value.find("ui.iteminfo"), std::string::npos) << id << " 缺文案：" << info.value;
        for (const auto* group : {&info.effects, &info.uses, &info.sources}) {
            for (const std::string& line : *group) {
                EXPECT_EQ(line.find("ui.iteminfo"), std::string::npos) << id << " 缺文案：" << line;
                EXPECT_EQ(line.find('{'), std::string::npos) << id << " 占位没填上：" << line;
            }
        }
    }
}

TEST(ItemInfo, EveryItemWithAnEffectFieldHasAnEffectLine) {
    const Shipped& s = shipped();
    int checked = 0;
    for (const auto& [id, item] : s.data.items) {
        const bool hasEffect = item.restoreHp > 0 || item.restoreMp > 0 || !item.castMagic.empty() ||
                               item.weapon != 0 || item.curesPoison || item.poison > 0;
        if (!hasEffect) continue;
        ++checked;
        const ItemInfo info = describeItem(s.data, s.sources(), nullptr, {}, id);
        EXPECT_FALSE(info.effects.empty()) << id << " 有效果字段却没有效果行";
        if (item.restoreHp > 0) EXPECT_TRUE(anyContains(info.effects, std::to_string(item.restoreHp))) << id;
        if (item.restoreMp > 0) EXPECT_TRUE(anyContains(info.effects, std::to_string(item.restoreMp))) << id;
        if (!item.castMagic.empty()) {
            const fanren::core::Magic* magic = s.data.findMagic(item.castMagic);
            ASSERT_NE(magic, nullptr) << id;
            EXPECT_TRUE(anyContains(info.effects, "「" + magic->name + "」")) << id << dump(info.effects);
            EXPECT_TRUE(anyContains(info.effects, "不能蓄劲")) << id;
        }
    }
    EXPECT_GE(checked, 30) << "回复、施法、兵刃的东西不止这几样";
}

TEST(ItemInfo, NothingStrayEndsUpAsTheOnlySourceForShopGoods) {
    // 店里卖的东西来源行里一定写得出那家店（不会退成「剧情所得」）。
    const Shipped& s = shipped();
    for (const auto& [shopId, shop] : s.shops) {
        for (const fanren::rules::ShopEntry& entry : shop.entries) {
            const ItemInfo info = describeItem(s.data, s.sources(), nullptr, {}, entry.itemId);
            EXPECT_TRUE(anyContains(info.sources, "有售")) << shopId << " / " << entry.itemId << dump(info.sources);
        }
    }
}

// ===========================================================================
// 验收判据 4：遍历全部法术
// ===========================================================================

TEST(ItemInfo, EveryMagicHasACategoryAndACostLine) {
    const Shipped& s = shipped();
    ASSERT_GT(s.data.magics.size(), 10u);
    for (const auto& [id, magic] : s.data.magics) {
        const fanren::rules::MagicInfo info = describeMagic(s.data, magic, {});
        EXPECT_FALSE(info.category.empty()) << id;
        EXPECT_EQ(info.category.find("ui.iteminfo"), std::string::npos) << id;
        EXPECT_EQ(info.cost, "耗法力 " + std::to_string(magic.needMp)) << id;
        EXPECT_FALSE(info.lines.empty()) << id << " 至少写得出蓄劲那一条";
    }
}

// ===========================================================================
// 逐字对上的几样
// ===========================================================================

TEST(ItemInfo, APillSaysWhatItRestoresWhereItIsSoldAndWhatItIsWorth) {
    const Shipped& s = shipped();
    fanren::core::GameState state;
    state.chapter = 7;
    const ItemInfo info = describeItem(s.data, s.sources(), &state, {}, "pill_huiqi_dan");
    ASSERT_EQ(info.effects.size(), 1u) << dump(info.effects);
    EXPECT_EQ(info.effects[0], "回复法力 90");
    EXPECT_TRUE(anyContains(info.uses, "战斗里用得上"));
    EXPECT_TRUE(anyContains(info.sources, "「" + s.data.lookupText(s.shops.at("ch07_fangshi_yaotan").nameKey) + "」有售")) << dump(info.sources);
    EXPECT_TRUE(anyContains(info.sources, "照「回气丹方」炼得")) << dump(info.sources);
    EXPECT_TRUE(anyContains(info.sources, "的宝箱")) << "第 7 章禁地的宝箱里有回气丹：" << dump(info.sources);
    EXPECT_EQ(info.value, "良品（2 阶） · 基准价 500 灵石");
}

TEST(ItemInfo, AHerbSaysItsAgeAndWhichRecipesItGoesInto) {
    const Shipped& s = shipped();
    const ItemInfo info = describeItem(s.data, s.sources(), nullptr, {}, "herb_huangjing_cao", 44);
    EXPECT_TRUE(anyContains(info.effects, "回复气血 6"));
    EXPECT_TRUE(anyContains(info.effects, "这一株 44 年，至多长到 44 年")) << dump(info.effects);
    EXPECT_TRUE(anyContains(info.uses, "入「回气丹」要 1 份（炼丹），须 20 年以上")) << dump(info.uses);

    const ItemInfo noAge = describeItem(s.data, s.sources(), nullptr, {}, "herb_huangjing_cao");
    EXPECT_TRUE(anyContains(noAge.effects, "至多长到 44 年")) << "不知道是哪一堆时只写上限";
}

TEST(ItemInfo, ATalismanSaysWhichSpellItCastsAndThatItCostsNothing) {
    const Shipped& s = shipped();
    const ItemInfo info = describeItem(s.data, s.sources(), nullptr, {}, "talisman_huoqiu_fu");
    const fanren::core::Magic* magic = s.data.findMagic("magic_huoqiu_shu");
    ASSERT_NE(magic, nullptr);
    ASSERT_GE(info.effects.size(), 2u) << dump(info.effects);
    EXPECT_EQ(info.effects[0], "施展「" + magic->name + "」 · 火 · 威力 " + std::to_string(magic->power));
    EXPECT_EQ(info.effects[1], "不耗法力，不能蓄劲");
    EXPECT_EQ(fanren::rules::effectSummary(info), info.effects[0] + " · " + info.effects[1]);
}

TEST(ItemInfo, AWeaponSaysWhichAttackItAdds) {
    const Shipped& s = shipped();
    const ItemInfo info = describeItem(s.data, s.sources(), nullptr, {}, "weapon_yudai_duanjian");
    ASSERT_FALSE(info.effects.empty());
    EXPECT_EQ(info.effects[0], "普攻多一样「剑」");
}

TEST(ItemInfo, UntradeableThingsSaySo) {
    const Shipped& s = shipped();
    int seen = 0;
    for (const auto& [id, item] : s.data.items) {
        if (item.tradeable) continue;
        ++seen;
        const ItemInfo info = describeItem(s.data, s.sources(), nullptr, {}, id);
        EXPECT_NE(info.value.find("不可买卖"), std::string::npos) << id << " " << info.value;
        EXPECT_EQ(info.value.find("基准价"), std::string::npos) << id;
    }
    EXPECT_GT(seen, 0);
}

TEST(ItemInfo, StoryOnlyThingsFallBackToStory) {
    // 一件不在任何店、配方、掉落、宝箱、支线里的剧情物件。
    const Shipped& s = shipped();
    const ItemInfo info = describeItem(s.data, s.sources(), nullptr, {}, "story_huangtong_ping");
    ASSERT_EQ(info.sources.size(), 1u) << dump(info.sources);
    EXPECT_EQ(info.sources[0], "剧情所得");
}

TEST(ItemInfo, LaterChaptersAreNotSpoiledByTheSourceLine) {
    // 第 7 章的玩家读回气丹的来源：第 8 章以后的店不列。不给 state（单测遍历）时全列。
    const Shipped& s = shipped();
    fanren::core::GameState state;
    state.chapter = 7;
    const ItemInfo gated = describeItem(s.data, s.sources(), &state, {}, "pill_huiqi_dan");
    const ItemInfo open = describeItem(s.data, s.sources(), nullptr, {}, "pill_huiqi_dan");
    const std::string ch13 = s.data.lookupText(s.shops.at("ch13_cunjin_ge").nameKey);
    EXPECT_FALSE(anyContains(gated.sources, ch13)) << dump(gated.sources);
    EXPECT_NE(dump(open.sources), dump(gated.sources)) << "不设闸时列得更多";
}

TEST(ItemInfo, TheGateChapterComesFromTheSourcesWhenTheSaveDoesNotTrackIt) {
    // 存档里的 GameState::chapter 不跟着剧情走（第 7 章的检查点存档里还是 1）：game 层把按章节表推出的当前章
    // 填进 sources.chapter，闸以它为准。
    const Shipped& s = shipped();
    fanren::core::GameState state;   // chapter 缺省 1
    ItemInfoSources sources = s.sources();
    const std::string shop = s.data.lookupText(s.shops.at("ch07_fangshi_yaotan").nameKey);
    EXPECT_FALSE(anyContains(describeItem(s.data, sources, &state, {}, "pill_qingling_san").sources, shop))
        << "只看存档的章号：第 1 章看不见第 7 章的店";
    sources.chapter = 7;
    EXPECT_TRUE(anyContains(describeItem(s.data, sources, &state, {}, "pill_qingling_san").sources, shop))
        << dump(describeItem(s.data, sources, &state, {}, "pill_qingling_san").sources);
}

TEST(ItemInfo, MortalChaptersUseTheMortalWords) {
    const Shipped& s = shipped();
    fanren::rules::InfoWords mortal;
    mortal.mp = "气力";
    mortal.currency = "碎银";
    const ItemInfo info = describeItem(s.data, s.sources(), nullptr, mortal, "pill_huiqi_dan");
    EXPECT_EQ(info.effects[0], "回复气力 90");
    EXPECT_NE(info.value.find("碎银"), std::string::npos);
    EXPECT_EQ(info.value.find("灵石"), std::string::npos);
}

TEST(ItemInfo, CompanionsGiftTastesOnlyForTheMetOnes) {
    const Shipped& s = shipped();
    // 找一位有喜好的伙伴与她喜欢的一样东西。
    const fanren::core::Companion* who = nullptr;
    for (const auto& [id, companion] : s.data.companions) {
        if (!companion.giftsLike.empty() && !companion.metFlag.empty()) {
            who = &companion;
            break;
        }
    }
    ASSERT_NE(who, nullptr) << "伙伴数据里至少一位写了喜好";
    const std::string liked = who->giftsLike.front();
    fanren::core::GameState state;
    state.chapter = 13;
    EXPECT_FALSE(anyContains(describeItem(s.data, s.sources(), &state, {}, liked).uses, "喜欢")) << "没结识不写";
    state.setFlag(who->metFlag);
    EXPECT_TRUE(anyContains(describeItem(s.data, s.sources(), &state, {}, liked).uses, "喜欢"))
        << dump(describeItem(s.data, s.sources(), &state, {}, liked).uses);
}

TEST(ItemInfo, MagicLinesFollowTheBattleRules) {
    const Shipped& s = shipped();
    for (const auto& [id, magic] : s.data.magics) {
        const auto info = describeMagic(s.data, magic, {});
        const std::string all = fanren::rules::magicSummary(info);
        if (magic.effect == fanren::core::MagicEffect::Stagger) {
            EXPECT_NE(all.find("减 " + std::to_string(magic.stagger) + " 点"), std::string::npos) << id << all;
            EXPECT_NE(all.find("不吃蓄劲"), std::string::npos) << id;
        } else if (magic.effect == fanren::core::MagicEffect::Reveal) {
            EXPECT_NE(all.find("揭开"), std::string::npos) << id;
        } else if (fanren::core::castableMagic(magic)) {
            // 蓄劲上限 kBoostMax = 3：威力至多 ×4，连发至多 4 发（docs/octopath-battle.md 2.3）。
            const std::string want = magic.boost == fanren::core::MagicBoost::Hits ? "至多 4 发" : "至多 ×4";
            EXPECT_NE(all.find(want), std::string::npos) << id << all;
        } else {
            EXPECT_NE(all.find("战斗里施展不了"), std::string::npos) << id << all;
        }
        if (magic.power > 0) EXPECT_NE(all.find("威力 " + std::to_string(magic.power)), std::string::npos) << id;
    }
}

TEST(ItemInfo, FillTemplateAndShopChapter) {
    using fanren::rules::fillTemplate;
    EXPECT_EQ(fillTemplate("回复{hp} {n}", {{"hp", "气血"}, {"n", "30"}}), "回复气血 30");
    EXPECT_EQ(fillTemplate("{n}{n}", {{"n", "a"}}), "aa");
    EXPECT_EQ(fillTemplate("{x} 留着", {{"n", "a"}}), "{x} 留着") << "没给的占位原样留着";
    EXPECT_EQ(fanren::rules::shopChapter("ch07_fangshi_yaotan"), 7);
    EXPECT_EQ(fanren::rules::shopChapter("ch13_cunjin_ge"), 13);
    EXPECT_EQ(fanren::rules::shopChapter("village_general_store"), 0);
    EXPECT_EQ(fanren::rules::shopChapter("chx7_a"), 0);
}

TEST(ItemInfo, DetailRowsKeepTheOrderAndTheFlavourTextUntouched) {
    const Shipped& s = shipped();
    const ItemInfo info = describeItem(s.data, s.sources(), nullptr, {}, "pill_huiqi_dan");
    const std::string flavour = s.data.lookupText(s.data.findItem("pill_huiqi_dan")->descKey);
    const auto rows = fanren::game::itemDetailRows(s.data, info, flavour);
    ASSERT_GE(rows.size(), 6u);
    EXPECT_EQ(rows[0].label, "效果");
    EXPECT_EQ(rows[1].label, "用途");
    EXPECT_EQ(rows[2].label, "来源");
    EXPECT_EQ(rows[3].label, "品阶");
    EXPECT_TRUE(rows[4].rule);
    EXPECT_EQ(rows[5].text, flavour) << "原来的说明一个字不改";
}

}  // namespace
