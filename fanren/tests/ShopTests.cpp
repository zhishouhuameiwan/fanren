// 商店的验收测试：计价单一出处、买卖闭环、灵草按年份分堆、四条拒绝路径。
//
// 无头模式下绘制全是空操作，因此这里一律不测像素，只测三件事：
//   1. 同一株药在商店与在剧情脚本里是不是同一个价钱（第 2 章的核心对照）；
//   2. 买卖有没有真的改到那唯一一本账（GameState 里的灵石与背包）；
//   3. 做不成的时候有没有把话说清楚——静默无反应一律会被玩家当成 bug。
//
// 每一处新加的判据都配一条「故意写坏 → 确实被抓住」的用例（本文件里以
// [负向] 标注）。本项目栽过的那次是校验器的正则被 heredoc 吃掉转义、
// 从此永远报通过而无人发现，正因为没人做过负向验证。
#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Bottle.h"
#include "core/rules/Economy.h"
#include "game/Application.h"
#include "game/ShopScene.h"
#include "io/DataLoader.h"
#include "io/ShopLoader.h"

namespace {

using fanren::core::GameState;
using fanren::core::Item;
using fanren::core::ItemKind;
using fanren::game::Application;
using fanren::game::ShopScene;
using fanren::game::spiritStones;
using fanren::rules::buyPrice;
using fanren::rules::herbPrice;
using fanren::rules::sellPrice;
using fanren::rules::Shop;

// 第 2 章外刃堂门前的收药处，data/shops/ch02_wairentang_yaoshang.json。
constexpr const char* kShopId = "ch02_wairentang_yaoshang";
constexpr const char* kHuangjing = "herb_huangjing_cao";
constexpr const char* kZhitongSan = "pill_zhitong_san";   // 在册但 stock 为 0，且不补货
constexpr const char* kJinchuangYao = "pill_jinchuang_yao";
constexpr const char* kLingshi = "material_lingshi";
// data 里唯一一件 tradeable:false 的东西（本命法宝，卖掉就没了）。
constexpr const char* kUntradeable = "weapon_qingzhu_fengyun_jian";

// docs/ch02-design.md 第 3 节那张实算表。黄精基价 5，三个年份三个价钱。
// 编剧的台词与 scripts/ch02 的三次结算都按这三个数写死了，改这里要连那张表
// 和那几条脚本一起改。
constexpr int kHuangjingBasePrice = 5;
constexpr int kPriceAtOneYear = 6;
constexpr int kPriceAtElevenYears = 22;
constexpr int kPriceAtFortyFourYears = 145;

std::string assetRoot() {
    namespace fs = std::filesystem;
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "shops" / "ch02_wairentang_yaoshang.json")) {
            return candidate;
        }
    }
    return ".";
}

Item makeHerb(int price) {
    Item item;
    item.id = "herb_probe";
    item.name = "测试灵草";
    item.kind = ItemKind::Herb;
    item.price = price;
    return item;
}

Shop makeShop(double buyRate, double sellRate) {
    Shop shop;
    shop.id = "shop_probe";
    shop.buyRate = buyRate;
    shop.sellRate = sellRate;
    return shop;
}

// ---------------------------------------------------------------------------
// 判据本身。抽成函数是为了能把「故意写坏」的实现也喂进去，确认它真抓得住。
// ---------------------------------------------------------------------------

using HerbPricer = int (*)(int basePrice, int age);

// 灵草的年份估值必须与 rules::herbPrice 分毫不差。
//
// 扫一片而不是只钉 44 年那一个点：只钉一个点的检查，换上另一条同样错的曲线
// 只要恰好穿过那个点就能蒙混过关。
[[nodiscard]] bool matchesHerbPrice(HerbPricer pricer, std::string& firstMismatch) {
    for (const int base : {1, 5, 8, 20, 100}) {
        for (const int age : {0, 1, 2, 5, 11, 22, 44, 100, 300}) {
            const int expected = herbPrice(base, age);
            const int actual = pricer(base, age);
            if (actual != expected) {
                firstMismatch = "基价 " + std::to_string(base) + " · " + std::to_string(age) +
                                " 年：应为 " + std::to_string(expected) + "，实得 " +
                                std::to_string(actual);
                return false;
            }
        }
    }
    firstMismatch.clear();
    return true;
}

// 商店侧的实际估值。买卖倍率取 1:1，价差被剥掉，剩下的就是那条年份曲线本身。
[[nodiscard]] int shopHerbValuation(int basePrice, int age) {
    return buyPrice(makeShop(1.0, 1.0), makeHerb(basePrice), age);
}

// [负向用例的样本] Economy.cpp 里曾经的那条平行曲线，原样搬来。
// 它与 herbPrice 在 age = 0 处重合、在低年份很接近，正是「只钉一个点的检查
// 抓不住它」的那种错法：44 年份上它给 493，herbPrice 给 145。
[[nodiscard]] int legacyAgedHerbPrice(int basePrice, int age) {
    if (basePrice <= 0) return 0;
    if (age <= 0) return basePrice;
    const double multiplier = 1.0 + 0.02 * age + 0.05 * age * age;
    return static_cast<int>(std::llround(basePrice * multiplier));
}

// 卖出列表的判据：同一味药的不同年份必须各占一行、各有各的价钱。
[[nodiscard]] bool listsEachAgeSeparately(const std::vector<ShopScene::SellStack>& stacks,
                                          const std::string& itemId, int lowAge, int highAge,
                                          int lowPrice, int highPrice, std::string& why) {
    int lowSeen = 0;
    int highSeen = 0;
    for (const ShopScene::SellStack& stack : stacks) {
        if (stack.itemId != itemId) continue;
        if (stack.herbAge == lowAge) {
            ++lowSeen;
            if (stack.unitPrice != lowPrice) {
                why = std::to_string(lowAge) + " 年那堆的价钱是 " +
                      std::to_string(stack.unitPrice) + "，应为 " + std::to_string(lowPrice);
                return false;
            }
        } else if (stack.herbAge == highAge) {
            ++highSeen;
            if (stack.unitPrice != highPrice) {
                why = std::to_string(highAge) + " 年那堆的价钱是 " +
                      std::to_string(stack.unitPrice) + "，应为 " + std::to_string(highPrice);
                return false;
            }
        }
    }
    if (lowSeen != 1 || highSeen != 1) {
        why = "两个年份应当各占一行，实得 " + std::to_string(lowAge) + " 年 " +
              std::to_string(lowSeen) + " 行、" + std::to_string(highAge) + " 年 " +
              std::to_string(highSeen) + " 行";
        return false;
    }
    why.clear();
    return true;
}

// [负向用例的样本] 按 itemId 合并的分堆——灵田面板的种子列表就是这么建的
// （那里合并是对的：下种之后年份一律归零）。照搬到商店就会把「一年份卖 6、
// 四十四年份卖 145」这一课整个抹掉。
[[nodiscard]] std::vector<ShopScene::SellStack> mergedByItemIdStacks(
    const fanren::core::GameData& data, const GameState& state, const Shop& shop) {
    std::vector<ShopScene::SellStack> stacks;
    for (const fanren::core::BagEntry& entry : state.bag) {
        if (entry.itemId == kLingshi || entry.count <= 0) continue;
        const Item* item = data.findItem(entry.itemId);
        if (item == nullptr || !item->tradeable) continue;
        bool merged = false;
        for (ShopScene::SellStack& stack : stacks) {
            if (stack.itemId == entry.itemId) {
                stack.count += entry.count;
                merged = true;
                break;
            }
        }
        if (merged) continue;
        ShopScene::SellStack stack;
        stack.itemId = entry.itemId;
        stack.herbAge = entry.herbAge;
        stack.count = entry.count;
        stack.unitPrice = sellPrice(shop, *item, entry.herbAge);
        stacks.push_back(stack);
    }
    return stacks;
}

// 某一行的 detail 里有没有出现这个数。玩家看到的是这串字，不是结构体。
[[nodiscard]] bool anyDetailContains(const std::vector<fanren::ui::ListItem>& rows,
                                     const std::string& needle) {
    for (const fanren::ui::ListItem& row : rows) {
        if (row.detail.find(needle) != std::string::npos) return true;
    }
    return false;
}

// ===========================================================================
// 1. 计价单一出处
// ===========================================================================

TEST(ShopPricing, ShopValuationOfHerbsIsExactlyRulesHerbPrice) {
    std::string mismatch;
    EXPECT_TRUE(matchesHerbPrice(&shopHerbValuation, mismatch)) << mismatch;
}

TEST(ShopPricing, TheChapterTwoTableComesOutOfTheShopUnchanged) {
    // 这三个数是第 2 章的教学对照组，scripts/ch02 的三次结算与
    // docs/ch02-design.md 第 3 节那张表都按它们写死。
    EXPECT_EQ(herbPrice(kHuangjingBasePrice, 1), kPriceAtOneYear);
    EXPECT_EQ(herbPrice(kHuangjingBasePrice, 11), kPriceAtElevenYears);
    EXPECT_EQ(herbPrice(kHuangjingBasePrice, 44), kPriceAtFortyFourYears);

    EXPECT_EQ(shopHerbValuation(kHuangjingBasePrice, 1), kPriceAtOneYear);
    EXPECT_EQ(shopHerbValuation(kHuangjingBasePrice, 11), kPriceAtElevenYears);
    EXPECT_EQ(shopHerbValuation(kHuangjingBasePrice, 44), kPriceAtFortyFourYears);
}

TEST(ShopPricing, FortyFourYearHuangjingFromRealDataIsOneFourFive) {
    // 不用手捏的 Item，直接用 data/items/herbs/huangjing_cao.json 里那一条：
    // 基价一旦被改，这条会连同上面那张表一起红。
    auto data = fanren::io::loadGameData(assetRoot() + std::string("/data"));
    ASSERT_TRUE(data.ok) << data.error;
    const Item* huangjing = data.value.findItem(kHuangjing);
    ASSERT_NE(huangjing, nullptr);
    EXPECT_EQ(huangjing->price, kHuangjingBasePrice);

    const Shop oneToOne = makeShop(1.0, 1.0);
    EXPECT_EQ(buyPrice(oneToOne, *huangjing, 44), kPriceAtFortyFourYears);
    EXPECT_EQ(sellPrice(oneToOne, *huangjing, 44), kPriceAtFortyFourYears);
    EXPECT_EQ(buyPrice(oneToOne, *huangjing, 44), herbPrice(huangjing->price, 44));
}

// [负向] 把 Economy 换回那条独立公式，上面那道判据必须当场变红。
//
// 这条用例证明的是「检查本身有效」：它把旧实现原样喂给同一个判据，
// 判据返回 false，且它在 44 年那一点上给出的正是当初对不上的 493。
TEST(ShopPricing, NEGATIVE_TheGuardCatchesAReintroducedParallelCurve) {
    std::string mismatch;
    EXPECT_FALSE(matchesHerbPrice(&legacyAgedHerbPrice, mismatch))
        << "判据放过了一条与 herbPrice 不同的曲线——它抓不住回退，等于没有";
    EXPECT_FALSE(mismatch.empty()) << "判据失败时必须说清是哪一点对不上";

    // 当初那两个对不上的数，原样钉在这里当证物。
    EXPECT_EQ(legacyAgedHerbPrice(kHuangjingBasePrice, 44), 493);
    EXPECT_EQ(herbPrice(kHuangjingBasePrice, 44), kPriceAtFortyFourYears);
    EXPECT_NE(legacyAgedHerbPrice(kHuangjingBasePrice, 44),
              herbPrice(kHuangjingBasePrice, 44));
}

TEST(ShopPricing, TheAgeCurveEntersExactlyOnceUnderRealShopRates) {
    // 真实店铺的买卖倍率各乘一次，年份曲线只进来一次——不是被平方，
    // 也不是在别处又算了一遍。
    auto shops = fanren::io::loadShops(assetRoot() + std::string("/data/shops"));
    ASSERT_TRUE(shops.ok) << shops.error;
    const auto it = shops.value.find(kShopId);
    ASSERT_NE(it, shops.value.end());
    const Shop& shop = it->second;

    const Item herb = makeHerb(kHuangjingBasePrice);
    const int valuation = herbPrice(kHuangjingBasePrice, 44);
    EXPECT_EQ(valuation, kPriceAtFortyFourYears);
    EXPECT_EQ(buyPrice(shop, herb, 44), static_cast<int>(std::llround(valuation * shop.buyRate)));
    EXPECT_EQ(sellPrice(shop, herb, 44), static_cast<int>(std::llround(valuation * shop.sellRate)));
    // 买价恒高于卖价：这道价差是货币回收口，没有它灵石会无限膨胀。
    EXPECT_GT(buyPrice(shop, herb, 44), sellPrice(shop, herb, 44));
}

TEST(ShopPricing, ListedPricesInTheChapterTwoShopAgreeWithTheRules) {
    // 挂牌价是数据作者写下的数（json 里的 price），实际收付走 rules::buyPrice。
    // 两者对不上，玩家就会看到「标 6 收 8」。ch02 这家店的 note 写明挂牌价
    // 按 buyRate 对基价取整，这里把那句话钉成可执行的检查。
    auto data = fanren::io::loadGameData(assetRoot() + std::string("/data"));
    ASSERT_TRUE(data.ok) << data.error;
    auto shops = fanren::io::loadShops(assetRoot() + std::string("/data/shops"));
    ASSERT_TRUE(shops.ok) << shops.error;
    const auto it = shops.value.find(kShopId);
    ASSERT_NE(it, shops.value.end());

    for (const fanren::rules::ShopEntry& entry : it->second.entries) {
        const Item* item = data.value.findItem(entry.itemId);
        ASSERT_NE(item, nullptr) << entry.itemId << " 不在物品册上";
        EXPECT_EQ(entry.price, buyPrice(it->second, *item, 0))
            << entry.itemId << " 的挂牌价与按规则算出的价钱对不上";
    }
}

TEST(ShopPricing, AnAstronomicallyValuableHerbNeverWrapsAroundIntoAFreebie) {
    // 万年份灵草是大纲 4.1 明写会有的东西（天雷竹）。herbPrice 到顶返回 INT_MAX，
    // 再乘 buyRate 就冲出 int——没有上夹的话绕回来会变成负数，而负数又会被
    // 「价格不为负」那道下夹抹成 0：一件天价宝物标价 0，白送。
    const Item herb = makeHerb(10000);
    const int valuation = herbPrice(10000, 10000);
    EXPECT_GT(valuation, 0);

    const Shop rich = makeShop(1.2, 0.6);
    const int buy = buyPrice(rich, herb, 10000);
    const int sell = sellPrice(rich, herb, 10000);

    EXPECT_GT(buy, 0) << "天价宝物的买价绕成了 0，等于白送";
    EXPECT_GT(sell, 0);
    EXPECT_GE(buy, sell) << "夹住上限之后买价仍不得低于卖价";
    // [配对] 同一道夹子不能把寻常价钱也夹坏。
    EXPECT_EQ(buyPrice(rich, makeHerb(5), 44), 174);
}

// ===========================================================================
// 2. 商店数据加载
// ===========================================================================

TEST(ShopLoader, LoadsTheDeliveredShops) {
    auto shops = fanren::io::loadShops(assetRoot() + std::string("/data/shops"));
    ASSERT_TRUE(shops.ok) << shops.error;
    const auto it = shops.value.find(kShopId);
    ASSERT_NE(it, shops.value.end()) << "第 2 章的收药处没被加载";
    EXPECT_EQ(it->second.nameKey, "ch02.shop.name.wairentang_yaoshang");
    EXPECT_EQ(it->second.entries.size(), 5u);
    EXPECT_GT(it->second.buyRate, it->second.sellRate);
}

// 写一个临时店铺文件，返回路径。内容一律 ASCII，免得测试自己去趟编码的坑。
// name 不带扩展名：文件名得带上 pid，否则两个测试进程会抢同一个 json——一个还
// 开着读，另一个已经 remove，报的是「文件被另一进程占用」。见 tests/TempDir.h。
std::filesystem::path writeTempShop(const std::string& name, const std::string& body) {
    const std::filesystem::path file = fanren::test::uniqueTempPath(name, ".json");
    std::ofstream stream(file, std::ios::binary | std::ios::trunc);
    stream << body;
    return file;
}

TEST(ShopLoader, AcceptsAWellFormedShop) {
    const auto file = writeTempShop("fanren_shop_good", R"({
        "id": "shop_probe_good",
        "buyRate": 1.2,
        "sellRate": 0.6,
        "entries": [{"itemId": "pill_jinchuang_yao", "price": 24, "stock": 3, "restockDays": 5}]
    })");
    auto loaded = fanren::io::loadShop(file.string());
    EXPECT_TRUE(loaded.ok) << loaded.error;
    std::filesystem::remove(file);
}

// [负向] 卖价不低于买价 = 买进再卖回就能刷钱。加载必须当场拒收。
TEST(ShopLoader, NEGATIVE_RejectsAShopWhereSellingBackTurnsAProfit) {
    const auto file = writeTempShop("fanren_shop_mint", R"({
        "id": "shop_probe_mint",
        "buyRate": 0.5,
        "sellRate": 0.9,
        "entries": [{"itemId": "pill_jinchuang_yao", "price": 10, "stock": -1}]
    })");
    auto loaded = fanren::io::loadShop(file.string());
    EXPECT_FALSE(loaded.ok) << "买进再卖回就能赚钱的店铺被放过了";
    EXPECT_NE(loaded.error.find("shop_probe_mint"), std::string::npos)
        << "报错要说清是哪家店：" << loaded.error;
    std::filesystem::remove(file);
}

// [负向] 同一件货列两次：货架上两行一模一样，买哪一行全看运气。
TEST(ShopLoader, NEGATIVE_RejectsDuplicateEntriesInOneShop) {
    const auto file = writeTempShop("fanren_shop_dup", R"({
        "id": "shop_probe_dup",
        "buyRate": 1.0,
        "sellRate": 0.5,
        "entries": [
            {"itemId": "pill_jinchuang_yao", "price": 20, "stock": 1},
            {"itemId": "pill_jinchuang_yao", "price": 20, "stock": 2}
        ]
    })");
    auto loaded = fanren::io::loadShop(file.string());
    EXPECT_FALSE(loaded.ok) << "同一件货列两次被放过了";
    std::filesystem::remove(file);
}

// [负向] 补货量写成负数：rules 侧把任何负库存都当成无限供应，于是一件限量
// 珍品补过几个周期之后会悄悄变成随便拿。加载必须当场拒收。
TEST(ShopLoader, NEGATIVE_RejectsNegativeRestockAmountThatWouldUncapStock) {
    const auto file = writeTempShop("fanren_shop_negstock", R"({
        "id": "shop_probe_negstock",
        "buyRate": 1.0,
        "sellRate": 0.5,
        "entries": [
            {"itemId": "pill_jinchuang_yao", "price": 20, "stock": 2,
             "restockDays": 5, "restockAmount": -5, "maxStock": 10}
        ]
    })");
    auto loaded = fanren::io::loadShop(file.string());
    EXPECT_FALSE(loaded.ok) << "负的补货量被放过了";
    EXPECT_NE(loaded.error.find("pill_jinchuang_yao"), std::string::npos)
        << "报错要说清是哪一条：" << loaded.error;
    std::filesystem::remove(file);
}

// [负向] -1 之外的负库存同样是手误。-1 本身必须照收，否则这道闸就把
// 「无限供应」这个正当写法一起拦掉了。
TEST(ShopLoader, NEGATIVE_RejectsStockSentinelsOtherThanMinusOne) {
    const auto bad = writeTempShop("fanren_shop_badsentinel", R"({
        "id": "shop_probe_badsentinel",
        "buyRate": 1.0,
        "sellRate": 0.5,
        "entries": [{"itemId": "pill_jinchuang_yao", "price": 20, "stock": -7}]
    })");
    EXPECT_FALSE(fanren::io::loadShop(bad.string()).ok) << "stock = -7 被放过了";
    std::filesystem::remove(bad);

    // [配对] -1 是正当的「无限供应」，必须收。
    const auto good = writeTempShop("fanren_shop_infinite", R"({
        "id": "shop_probe_infinite",
        "buyRate": 1.0,
        "sellRate": 0.5,
        "entries": [{"itemId": "pill_jinchuang_yao", "price": 20, "stock": -1}]
    })");
    auto loaded = fanren::io::loadShop(good.string());
    EXPECT_TRUE(loaded.ok) << loaded.error;
    std::filesystem::remove(good);
}

// [负向] 缺 id 的店没法被脚本引用，加载必须失败而不是收下一家匿名店。
TEST(ShopLoader, NEGATIVE_RejectsAShopWithoutAnId) {
    const auto file = writeTempShop("fanren_shop_noid", R"({
        "buyRate": 1.0,
        "sellRate": 0.5,
        "entries": []
    })");
    auto loaded = fanren::io::loadShop(file.string());
    EXPECT_FALSE(loaded.ok) << "没有 id 的店铺被放过了";
    std::filesystem::remove(file);
}

// ===========================================================================
// 3. 面板：灵草按年份分堆
// ===========================================================================

class ShopPanelTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        app_.state().day = 1;
    }
    void TearDown() override { app_.shutdown(); }

    // 真实的第 2 章收药处，直接从 Application 里取（买卖会改它的库存）。
    fanren::rules::Shop& shop() {
        fanren::rules::Shop* found = app_.shop(kShopId);
        EXPECT_NE(found, nullptr);
        return *found;
    }

    Application app_;
};

TEST_F(ShopPanelTest, SplitsTheSameHerbByAgeIntoTwoRowsWithTheirOwnPrices) {
    // 一年份一株、四十四年份一株。倍率取 1:1，让面板上的数就是那张实算表里
    // 的数——这正是玩家要亲眼看见的那一课：一年份卖 6，四十四年份卖 145。
    app_.state().addItem(kHuangjing, 1, 1);
    app_.state().addItem(kHuangjing, 1, 44);

    const Shop oneToOne = makeShop(1.0, 1.0);
    const auto stacks = ShopScene::buildSellStacks(app_.data(), app_.state(), oneToOne);

    std::string why;
    EXPECT_TRUE(listsEachAgeSeparately(stacks, kHuangjing, 1, 44, kPriceAtOneYear,
                                       kPriceAtFortyFourYears, why))
        << why;
    ASSERT_EQ(stacks.size(), 2u);
    // 定序：同一味药内年份由低到高，玩家记得住「第二行是那株四十四年的」。
    EXPECT_EQ(stacks[0].herbAge, 1);
    EXPECT_EQ(stacks[1].herbAge, 44);

    const auto rows =
        ShopScene::buildSellItems(fanren::game::wordingStage(app_.state()), app_.data(), stacks);
    ASSERT_EQ(rows.size(), 3u) << "两堆 + 一条「返回」";
    EXPECT_EQ(rows.back().label, "返回");
    EXPECT_NE(rows[0].label.find("1 年"), std::string::npos) << rows[0].label;
    EXPECT_NE(rows[1].label.find("44 年"), std::string::npos) << rows[1].label;
    EXPECT_TRUE(anyDetailContains(rows, std::to_string(kPriceAtOneYear)));
    EXPECT_TRUE(anyDetailContains(rows, std::to_string(kPriceAtFortyFourYears)));
}

// [负向] 把分堆换成「按 itemId 合并」（灵田面板种子列表的那种建法），
// 同一道判据必须当场变红。
TEST_F(ShopPanelTest, NEGATIVE_TheGuardCatchesAListMergedByItemId) {
    app_.state().addItem(kHuangjing, 1, 1);
    app_.state().addItem(kHuangjing, 1, 44);

    const Shop oneToOne = makeShop(1.0, 1.0);
    const auto merged = mergedByItemIdStacks(app_.data(), app_.state(), oneToOne);

    ASSERT_EQ(merged.size(), 1u) << "合并版本本该只剩一行，样本自己先得是错的";
    std::string why;
    EXPECT_FALSE(listsEachAgeSeparately(merged, kHuangjing, 1, 44, kPriceAtOneYear,
                                        kPriceAtFortyFourYears, why))
        << "判据放过了把两个年份合成一行的列表——它抓不住合并，等于没有";
    EXPECT_FALSE(why.empty());
}

TEST_F(ShopPanelTest, SellListPricesGoThroughTheShopSellRate) {
    // 真实店铺的卖出倍率是 0.6：面板上的数必须是 rules::sellPrice 给的那个，
    // 不是估值本身。估值（145）与到手价（87）之间差的就是这道货币回收口。
    app_.state().addItem(kHuangjing, 1, 1);
    app_.state().addItem(kHuangjing, 1, 44);

    const auto stacks = ShopScene::buildSellStacks(app_.data(), app_.state(), shop());
    ASSERT_EQ(stacks.size(), 2u);

    const Item* huangjing = app_.data().findItem(kHuangjing);
    ASSERT_NE(huangjing, nullptr);
    EXPECT_EQ(stacks[0].unitPrice, sellPrice(shop(), *huangjing, 1));
    EXPECT_EQ(stacks[1].unitPrice, sellPrice(shop(), *huangjing, 44));
    // 分堆的意义在这里：高年份那一堆必须显著更值钱。
    EXPECT_GT(stacks[1].unitPrice, stacks[0].unitPrice * 10);
}

TEST_F(ShopPanelTest, MoneyAndUntradeablesNeverShowUpOnTheSellList) {
    // 两件都要真的放进包里再查，否则这条用例只证得了其中一件——
    // 名字里写着「与不可交易物」，却只塞了灵石，那条 tradeable 分支就没人测过。
    app_.state().addItem(kLingshi, 100, 0);
    app_.state().addItem(kUntradeable, 1, 0);
    const Item* bound = app_.data().findItem(kUntradeable);
    ASSERT_NE(bound, nullptr);
    ASSERT_FALSE(bound->tradeable) << "样本本身必须真的是不可交易的";

    const auto stacks = ShopScene::buildSellStacks(app_.data(), app_.state(), shop());
    for (const ShopScene::SellStack& stack : stacks) {
        EXPECT_NE(stack.itemId, kLingshi) << "灵石不该出现在卖出列表里";
        EXPECT_NE(stack.itemId, kUntradeable) << "本命法宝不该能卖给药商";
    }

    // [配对] 换一件寻常药材放进去，它必须出现——否则上面两条可能只是因为
    // 列表恒为空而通过。
    app_.state().addItem(kHuangjing, 1, 1);
    const auto after = ShopScene::buildSellStacks(app_.data(), app_.state(), shop());
    ASSERT_EQ(after.size(), 1u);
    EXPECT_EQ(after[0].itemId, kHuangjing);
}

TEST_F(ShopPanelTest, SellListOrderSurvivesABagReshuffle) {
    // GameState::removeItem 会把整个背包按年份重排。卖出列表若跟着背包的
    // 顺序走，买一次东西就能让行序跳动，而玩家的手指记的是行号。
    app_.state().addItem(kHuangjing, 1, 44);
    app_.state().addItem(kHuangjing, 1, 1);
    app_.state().addItem(kLingshi, 500, 0);

    const auto before = ShopScene::buildSellStacks(app_.data(), app_.state(), shop());
    ASSERT_EQ(before.size(), 2u);

    // 买一件丹药：付钱那一步（removeItem）会把整个背包按年份重排。
    // 买丹药而不是买灵草，是为了让新增的那一堆排在两株黄精之后，
    // 前两行该待在原处就得待在原处。
    int pillIndex = -1;
    for (std::size_t i = 0; i < shop().entries.size(); ++i) {
        if (shop().entries[i].itemId == kJinchuangYao) pillIndex = static_cast<int>(i);
    }
    ASSERT_GE(pillIndex, 0);

    ShopScene scene(kShopId);
    scene.onEnter(app_);
    ASSERT_TRUE(scene.buyAt(app_, pillIndex)) << scene.feedback();
    const auto after = ShopScene::buildSellStacks(app_.data(), app_.state(), shop());

    ASSERT_GE(after.size(), before.size());
    for (std::size_t i = 0; i < before.size(); ++i) {
        EXPECT_EQ(before[i].itemId, after[i].itemId) << "第 " << i << " 行换人了";
        EXPECT_EQ(before[i].herbAge, after[i].herbAge) << "第 " << i << " 行换人了";
    }
}

// ===========================================================================
// 4. 买卖闭环
// ===========================================================================

TEST_F(ShopPanelTest, SellingTheFortyFourYearPileAddsStonesAndRemovesThatPileOnly) {
    app_.state().addItem(kHuangjing, 1, 1);
    app_.state().addItem(kHuangjing, 1, 44);

    ShopScene scene(kShopId);
    scene.onEnter(app_);

    const auto stacks = ShopScene::buildSellStacks(app_.data(), app_.state(), shop());
    ASSERT_EQ(stacks.size(), 2u);
    const int index = stacks[1].herbAge == 44 ? 1 : 0;
    const int expected = stacks[static_cast<std::size_t>(index)].unitPrice;
    const int before = spiritStones(app_.state());

    ASSERT_TRUE(scene.sellAt(app_, index)) << scene.feedback();

    EXPECT_EQ(spiritStones(app_.state()) - before, expected) << "到手的灵石数额不对";
    EXPECT_EQ(app_.state().itemCountOfAge(kHuangjing, 44), 0) << "卖掉的那一堆没少";
    // 少的必须是玩家点的那一堆。GameState::removeItem 会先扣年份最低的，
    // 用错了这里就会看到一年份那株不翼而飞、四十四年那株还躺在包里。
    EXPECT_EQ(app_.state().itemCountOfAge(kHuangjing, 1), 1) << "扣错了堆";
}

// [负向] 证明上面那条断言真的分得出对错：按年份盲扣（GameState::removeItem）
// 掉的是一年份那株，四十四年那株原封不动。
TEST_F(ShopPanelTest, NEGATIVE_AgeBlindRemovalEatsTheWrongPile) {
    app_.state().addItem(kHuangjing, 1, 1);
    app_.state().addItem(kHuangjing, 1, 44);

    ASSERT_TRUE(app_.state().removeItem(kHuangjing, 1));   // 商店若用它就是这个结果

    EXPECT_EQ(app_.state().itemCountOfAge(kHuangjing, 1), 0)
        << "盲扣本该先吃掉年份最低的那堆，样本自己先得是错的";
    EXPECT_EQ(app_.state().itemCountOfAge(kHuangjing, 44), 1);
}

TEST_F(ShopPanelTest, BuyingMovesStonesStockAndBagTogether) {
    app_.state().addItem(kLingshi, 100, 0);

    ShopScene scene(kShopId);
    scene.onEnter(app_);

    const fanren::rules::ShopEntry& entry = shop().entries.front();
    const std::string itemId = entry.itemId;
    const Item* item = app_.data().findItem(itemId);
    ASSERT_NE(item, nullptr);
    const int price = buyPrice(shop(), *item, 0);
    const int stockBefore = entry.stock;
    const int purseBefore = spiritStones(app_.state());
    const int bagBefore = app_.state().itemCount(itemId);

    ASSERT_TRUE(scene.buyAt(app_, 0)) << scene.feedback();

    EXPECT_EQ(purseBefore - spiritStones(app_.state()), price) << "付出的灵石数额不对";
    EXPECT_EQ(app_.state().itemCount(itemId) - bagBefore, 1);
    EXPECT_EQ(shop().entries.front().stock, stockBefore - 1) << "货架上没少一件";
}

TEST_F(ShopPanelTest, BuyAndSellBackNeverTurnsAProfit) {
    // 价差是货币回收口。买进再卖回必须亏，否则挂机刷钱。
    app_.state().addItem(kLingshi, 500, 0);
    ShopScene scene(kShopId);
    scene.onEnter(app_);

    const int before = spiritStones(app_.state());
    ASSERT_TRUE(scene.buyAt(app_, 0)) << scene.feedback();
    const auto stacks = ShopScene::buildSellStacks(app_.data(), app_.state(), shop());
    ASSERT_FALSE(stacks.empty());
    ASSERT_TRUE(scene.sellAt(app_, 0)) << scene.feedback();

    EXPECT_LT(spiritStones(app_.state()), before) << "买进再卖回没有亏，这就是印钞机";
}

// ===========================================================================
// 5. 四条拒绝路径：做不成的时候必须把话说清楚
// ===========================================================================

TEST_F(ShopPanelTest, RejectsWhenThePurseIsTooThin) {
    ShopScene scene(kShopId);
    scene.onEnter(app_);
    ASSERT_EQ(spiritStones(app_.state()), 0);

    const int stockBefore = shop().entries.front().stock;
    EXPECT_FALSE(scene.buyAt(app_, 0));
    // 问的是「当前阶段的钱不够」，而不是某一句固定的中文：钱的叫法跟着剧情阶段
    // 走（凡人碎银、修仙灵石），把某一个词钉进测试，换阶段就会假红一次。
    const std::string money = fanren::game::currencyName(app_.state());
    EXPECT_NE(scene.feedback().find(money + "不足"), std::string::npos)
        << "该说清楚是钱不够，实为：" << scene.feedback();
    EXPECT_EQ(shop().entries.front().stock, stockBefore) << "没买成却把库存扣了";
    EXPECT_EQ(app_.state().itemCount(shop().entries.front().itemId), 0);

    // 列表上也要写明原因，不能只是一行变灰的字。
    const auto rows = ShopScene::buildBuyItems(app_.data(), app_.state(), shop());
    ASSERT_FALSE(rows.empty());
    EXPECT_FALSE(rows[0].enabled);
    EXPECT_NE(rows[0].disabledReason.find("尚差"), std::string::npos) << rows[0].disabledReason;

    // [配对] 给够钱，同一次调用必须成功——否则上面那个 false 可能是恒假的。
    app_.state().addItem(kLingshi, 500, 0);
    EXPECT_TRUE(scene.buyAt(app_, 0)) << scene.feedback();
}

TEST_F(ShopPanelTest, RejectsWhenTheShelfIsEmpty) {
    // 止痛散在册上，但 stock 为 0 且不补货：药商认得，不出售。
    app_.state().addItem(kLingshi, 500, 0);
    ShopScene scene(kShopId);
    scene.onEnter(app_);

    int soldOut = -1;
    for (std::size_t i = 0; i < shop().entries.size(); ++i) {
        if (shop().entries[i].itemId == kZhitongSan) soldOut = static_cast<int>(i);
    }
    ASSERT_GE(soldOut, 0) << "止痛散应当在这家店的册子上";
    ASSERT_EQ(shop().entries[static_cast<std::size_t>(soldOut)].stock, 0);

    EXPECT_FALSE(scene.buyAt(app_, soldOut));
    EXPECT_NE(scene.feedback().find("售罄"), std::string::npos) << scene.feedback();
    EXPECT_EQ(app_.state().itemCount(kZhitongSan), 0);

    const auto rows = ShopScene::buildBuyItems(app_.data(), app_.state(), shop());
    ASSERT_GT(rows.size(), static_cast<std::size_t>(soldOut));
    EXPECT_FALSE(rows[static_cast<std::size_t>(soldOut)].enabled);
    EXPECT_NE(rows[static_cast<std::size_t>(soldOut)].disabledReason.find("售罄"),
              std::string::npos);

    // [配对] 同一个玩家、同一家店，有货的那件必须买得成。
    EXPECT_TRUE(scene.buyAt(app_, 0)) << scene.feedback();
}

TEST_F(ShopPanelTest, RejectsSellingSomethingTheBagDoesNotHold) {
    ShopScene scene(kShopId);
    scene.onEnter(app_);
    ASSERT_TRUE(ShopScene::buildSellStacks(app_.data(), app_.state(), shop()).empty());

    const int before = spiritStones(app_.state());
    EXPECT_FALSE(scene.sellAt(app_, 0)) << "背包空着却卖成了";
    EXPECT_FALSE(scene.feedback().empty());
    EXPECT_EQ(spiritStones(app_.state()), before) << "没卖成却给了钱";

    // [配对] 包里有东西时同一次调用必须成功。
    app_.state().addItem(kHuangjing, 1, 44);
    EXPECT_TRUE(scene.sellAt(app_, 0)) << scene.feedback();
    EXPECT_GT(spiritStones(app_.state()), before);
}

TEST_F(ShopPanelTest, RejectsSellingSomethingWorthNothingInsteadOfEatingIt) {
    // 倍率是数据里的旋钮。调到这一档，一株药的到手价就归零了——收走货、给 0 块，
    // 玩家看到的是「点了一下东西没了」。
    app_.state().addItem(kHuangjing, 1, 1);
    shop().sellRate = 0.0001;   // 仍低于 buyRate，货币回收口的不变量不破

    ShopScene scene(kShopId);
    scene.onEnter(app_);
    const auto stacks = ShopScene::buildSellStacks(app_.data(), app_.state(), shop());
    ASSERT_EQ(stacks.size(), 1u);
    ASSERT_EQ(stacks[0].unitPrice, 0) << "样本自己先得真的算出 0 来";

    EXPECT_FALSE(scene.sellAt(app_, 0));
    EXPECT_FALSE(scene.feedback().empty());
    EXPECT_EQ(app_.state().itemCountOfAge(kHuangjing, 1), 1) << "没卖成却把货收走了";

    // [配对] 倍率调回来，同一株药必须卖得掉——否则上面那个 false 可能是恒假的。
    shop().sellRate = 0.6;
    EXPECT_TRUE(scene.sellAt(app_, 0)) << scene.feedback();
    EXPECT_EQ(app_.state().itemCountOfAge(kHuangjing, 1), 0);
}

TEST_F(ShopPanelTest, ShowsAShelfEntryWhoseItemIsMissingFromTheItemBook) {
    // 货架上列着一件 data 里没有的东西（json 里 itemId 写错）。悄悄跳过那一行，
    // 这条写坏的数据就永远没人发现；照样列出并写明原因，一眼就能看见。
    app_.state().addItem(kLingshi, 500, 0);
    shop().entries.front().itemId = "no_such_item_id";

    ShopScene scene(kShopId);
    scene.onEnter(app_);

    const auto rows = ShopScene::buildBuyItems(app_.data(), app_.state(), shop());
    ASSERT_FALSE(rows.empty());
    EXPECT_EQ(rows[0].label, "no_such_item_id") << "查不到就回显 id";
    EXPECT_FALSE(rows[0].enabled);
    EXPECT_FALSE(rows[0].disabledReason.empty());

    EXPECT_FALSE(scene.buyAt(app_, 0));
    EXPECT_NE(scene.feedback().find("no_such_item_id"), std::string::npos) << scene.feedback();
    EXPECT_EQ(spiritStones(app_.state()), 500) << "没买成却把钱扣了";

    // [配对] 换成真的那味药，同一次调用必须成功。
    shop().entries.front().itemId = kHuangjing;
    EXPECT_TRUE(scene.buyAt(app_, 0)) << scene.feedback();
}

TEST_F(ShopPanelTest, RejectsAnUnknownShopIdInsteadOfGoingSilent) {
    ShopScene scene("shop_that_does_not_exist");
    scene.onEnter(app_);
    EXPECT_FALSE(scene.feedback().empty()) << "店 id 写错时面板必须说点什么";
    EXPECT_FALSE(scene.buyAt(app_, 0));
    EXPECT_FALSE(scene.sellAt(app_, 0));
}

// ===========================================================================
// 6. 补货：进店时按当前日结算
// ===========================================================================

TEST_F(ShopPanelTest, RestocksOnEntryAfterAFullPeriod) {
    app_.state().addItem(kLingshi, 500, 0);
    ShopScene first(kShopId);
    first.onEnter(app_);
    ASSERT_TRUE(first.buyAt(app_, 0)) << first.feedback();

    const fanren::rules::ShopEntry snapshot = shop().entries.front();
    ASSERT_GT(snapshot.restockDays, 0) << "第一条应当是会补货的货";
    const int afterPurchase = snapshot.stock;

    // 还差一天不足一个周期：一件也不该补。
    app_.state().day = snapshot.lastRestockDay + snapshot.restockDays - 1;
    ShopScene tooSoon(kShopId);
    tooSoon.onEnter(app_);
    EXPECT_EQ(shop().entries.front().stock, afterPurchase) << "不足一个周期却补了货";

    // 满一个周期：补上，且不超过上限。
    app_.state().day = snapshot.lastRestockDay + snapshot.restockDays;
    ShopScene later(kShopId);
    later.onEnter(app_);
    EXPECT_GT(shop().entries.front().stock, afterPurchase) << "过了一个周期却没补货";
    EXPECT_LE(shop().entries.front().stock, snapshot.maxStock) << "补过了上限";
}

// ===========================================================================
// 7. 两条入口：脚本的 shop() 与地图设施
// ===========================================================================

class ShopEntryPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        auto loaded = app_.loadMap("ch02_wairentang", std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
    }
    void TearDown() override { app_.shutdown(); }

    Application app_;
};

TEST_F(ShopEntryPointTest, WalkingUpToTheCounterOpensTheShop) {
    const fanren::core::TileMap* map = app_.currentMap();
    ASSERT_NE(map, nullptr);

    const fanren::core::MapObject* counter = nullptr;
    for (const fanren::core::MapObject& object : map->objects) {
        if (object.type == "facility" && object.property("kind") == "shop") counter = &object;
    }
    ASSERT_NE(counter, nullptr) << "外刃堂地图上应当有一处收药的柜台";
    ASSERT_EQ(counter->property("ref_id"), kShopId);

    // WorldScene 面朝设施按确认键走的就是这一条（见 WorldScene::interact）。
    app_.openFacility(*counter);
    app_.tick(1.0 / 60.0);   // 场景在帧末才真正入栈

    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Shop") << "走到柜台前没开店";
    EXPECT_EQ(static_cast<ShopScene*>(app_.topScene())->shopId(), kShopId);
    // 这条路上没有挂起的脚本命令，关店不该把别人的协程叫醒，也不该卡住。
    EXPECT_FALSE(app_.awaitingCommand());

    static_cast<ShopScene*>(app_.topScene())->leave(app_);
    app_.tick(1.0 / 60.0);
    EXPECT_EQ(app_.topScene(), nullptr) << "关了店场景却没被弹出";
}

TEST_F(ShopEntryPointTest, TheScriptShopCommandOpensTheSameSceneAndResumesOnClose) {
    // 脚本里的 shop("店id")。这条命令此前是空操作，脚本一路跑完什么也没发生。
    auto started = app_.startEvent("ch02/yaoshang.lua");
    ASSERT_TRUE(started.ok) << started.error;

    bool sawShop = false;
    for (int frame = 0; frame < 600 && app_.scripts().isRunning(); ++frame) {
        app_.tick(1.0 / 60.0);
        if (app_.topScene() != nullptr && app_.topScene()->name() == "Shop") {
            sawShop = true;
            break;
        }
        // 对话框要按键才会结束，无头下替玩家按掉。
        if (app_.awaitingCommand()) {
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = 0;
            app_.completeCommand(result);
        }
    }
    ASSERT_TRUE(sawShop) << "脚本走到 shop() 却没有开店——这正是原先那个空操作";
    EXPECT_TRUE(app_.awaitingCommand()) << "开着店时脚本应当挂起等它关上";

    ShopScene* scene = static_cast<ShopScene*>(app_.topScene());
    EXPECT_EQ(scene->shopId(), kShopId);

    // 关店：走面板自己那条路（玩家选「离开」），脚本必须被叫醒并跑完，
    // 否则这条命令就从「空操作」变成了新的卡死点。
    scene->leave(app_);
    EXPECT_FALSE(app_.awaitingCommand()) << "关了店却没把脚本叫醒";

    for (int frame = 0; frame < 600 && app_.scripts().isRunning(); ++frame) {
        app_.tick(1.0 / 60.0);
        if (app_.awaitingCommand()) {
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = 0;
            app_.completeCommand(result);
        }
    }
    EXPECT_FALSE(app_.scripts().isRunning()) << "关店之后脚本没能接着往下跑";
}

// ===========================================================================
// 5. 钱的叫法跟着剧情阶段走
// ===========================================================================

// 灵石是原著 ch131 才出现的修仙界货币，而第 2 章对应 ch10-27。
// 柜台上写着灵石、台词里说的是碎银，玩家在同一屏上就能看见两套说法。
TEST_F(ShopPanelTest, TheCounterUsesTheWordForMoneyHeActuallyKnows) {
    app_.state().addItem(kHuangjing, 1, 44);

    // 凡人阶段（旗标未置）——第 2 章的现场。
    ASSERT_EQ(app_.state().flag(fanren::game::kXiuxianKnownFlag), 0) << "夹具应当从凡人阶段起算";
    auto stacks = ShopScene::buildSellStacks(app_.data(), app_.state(), shop());
    auto rows = ShopScene::buildSellItems(fanren::game::wordingStage(app_.state()), app_.data(), stacks);
    ASSERT_FALSE(rows.empty());
    EXPECT_NE(rows[0].detail.find("碎银"), std::string::npos)
        << "凡人篇的柜台该说碎银，实为：" << rows[0].detail;
    EXPECT_EQ(rows[0].detail.find("灵石"), std::string::npos)
        << "凡人篇不该出现灵石：" << rows[0].detail;

    // 第 3 章揭开真身之后，同一块面板换成修仙界的说法。
    app_.state().setFlag(fanren::game::kXiuxianKnownFlag, 1);
    stacks = ShopScene::buildSellStacks(app_.data(), app_.state(), shop());
    rows = ShopScene::buildSellItems(fanren::game::wordingStage(app_.state()), app_.data(), stacks);
    ASSERT_FALSE(rows.empty());
    EXPECT_NE(rows[0].detail.find("灵石"), std::string::npos)
        << "旗标置上之后该换回灵石，实为：" << rows[0].detail;
}

// [负向] 先验判据本身有没有牙。
//
// 上一条的写法是「找不到某个词就算过」，而这种写法有个着名的失效方式：
// 只要被查的字串因为任何原因变成空的（接线断了、字段没填、调用拿错重载），
// 断言照样通过。本项目最惨的一次事故正是这个形状：校验器的正则被转义吃掉，
// 从此什么都匹配不到、永远报通过，而没人发现。
TEST_F(ShopPanelTest, NEGATIVE_TheCurrencyCheckWouldNoticeAnEmptyDetailLine) {
    app_.state().addItem(kHuangjing, 1, 44);
    const auto stacks = ShopScene::buildSellStacks(app_.data(), app_.state(), shop());
    const auto rows =
        ShopScene::buildSellItems(fanren::game::wordingStage(app_.state()), app_.data(), stacks);
    ASSERT_FALSE(rows.empty());

    // 上一条的两个断言都架在这一行上，它必须真的有内容。
    EXPECT_FALSE(rows[0].detail.empty()) << "明细行是空的，上一条测试等于没在查";
    EXPECT_NE(rows[0].detail.find("145"), std::string::npos)
        << "明细行里连价钱都没有，说明拿到的不是真的那一行：" << rows[0].detail;

    // 且两个阶段的叫法确实不同——否则切换本身就是个摆设。
    EXPECT_STRNE(fanren::game::currencyName(fanren::game::PanelStage::Mortal),
                 fanren::game::currencyName(fanren::game::PanelStage::Immortal));
}

}  // namespace
