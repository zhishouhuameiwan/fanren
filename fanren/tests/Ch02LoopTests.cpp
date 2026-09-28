// 第 2 章的玩法闭环测试（docs/ch02-design.md 第 8 节验收第 4 条，也就是第 9.2
// 节自己标注「仍缺」的那一条）：
//
//     种下 → 生长 → 催熟 → 收割带年份 → 卖出价差显著
//
// 链条上每一环都早有测试覆盖（播种与收割在 tests/PanelTests.cpp，计价在
// tests/ShopTests.cpp，绿液的账在 tests/Ch02BottleTests.cpp），但在这条测试之前
// 没有任何一条把它们串起来跑过。各环自己都对、拼起来却断掉，正是第 1 章栽过
// 两次的那种错法——掌天瓶经济是全作的经济中枢，第 2 章是它闭合的地方，
// 「闭合」这件事本身必须有人验。
//
// 与邻居的分工：
//   * tests/Ch02SliceTests.cpp   —— 通关测试：六道剧情闸门都过得去。验的是剧情。
//   * 本文件                      —— 闭环测试：经济链本身成立。验的是机制。
//   * tests/PanelTests.cpp       —— 灵田面板每个动作的语义与拒绝路径。
//   * tests/ShopTests.cpp        —— 计价单一出处、按年份分堆、四条拒绝路径。
//
// ---------------------------------------------------------------------------
// 「价差显著」这一条怎么写
// ---------------------------------------------------------------------------
// 第 5 环是整条链的要害：前四环全对、柜台却不按年份给钱，那四年就白熬了。
// 判据写成**相对**的——
//
//     链条末端卖到的钱 / 同一味药采下即卖的钱 ≥ 20
//
// ——而不是只钉 145 那一个数。只钉一个点的检查，换上另一条恰好穿过该点的错
// 曲线就能蒙混过关（tests/ShopTests.cpp 的 matchesHerbPrice 正是按这个思路写
// 的，那边扫的是基价 × 年份的一整片）。
//
// 反过来，光有比值也不够：一条比 herbPrice 更陡的错曲线（Economy.cpp 里曾经
// 真的有过一条，44 年上给 493）比值照样过得去。所以判据在比值之外还要求两笔
// 进账与 rules::herbPrice 分毫不差。两条合起来才咬得住。
//
// 两个数都取自这一趟实际跑出来的钱袋差额，不是常量：链条上任何一环断掉，
// 末端那一笔就跟着塌，比值当场不成立。
//
// ---------------------------------------------------------------------------
// 判据自检
// ---------------------------------------------------------------------------
// 「卖完之后背包里找不到四十四年那株」这类断言，在背包整个是空的时候照样通过；
// 比值判据也有同形的空转法——分母塌成 0 时比值无穷大，一样「通过」。凡这种
// 形状的断言都另配一条用例先证明判据本身咬得动人（本文件末尾的 Ch02LoopVerdict
// 四条，照 tests/Ch02SliceTests.cpp 的 Ch02EndState 写）。这个项目最惨的一次
// 事故正是校验器的正则被 heredoc 吃掉转义、从此永远报通过而无人发现，因为
// 没人做过负向验证。
#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Bottle.h"
#include "core/rules/Calendar.h"
#include "core/rules/Field.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/FieldScene.h"
#include "game/ShopScene.h"
#include "ui/Widgets.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::Item;
using fanren::core::MapObject;
using fanren::game::Application;
using fanren::game::FieldScene;
using fanren::game::ShopScene;
using fanren::game::spiritStones;
using fanren::rules::herbPrice;
using fanren::rules::Realm;

// ---- 本章用到的 id。契约见 docs/interfaces-p3-script.md 第 4 节 ----
constexpr const char* kFieldId = "shenshougu_yaopu";
constexpr const char* kHerbId = "herb_huangjing_cao";
constexpr const char* kMoneyId = "material_lingshi";
constexpr const char* kShopId = "ch02_wairentang_yaoshang";

constexpr const char* kMapYaopu = "ch02_yaopu";
constexpr const char* kMapWairentang = "ch02_wairentang";
constexpr const char* kFieldFacility = "facility_lingtian";
constexpr const char* kShopFacility = "facility_yaoshang";

// 黄精基价。data/items/herbs/huangjing_cao.json 里那一条，实算表的起点。
constexpr int kHuangjingBasePrice = 5;

// 「价差显著」的门槛。docs/ch02-design.md 第 3 节那张实算表给的是 24 倍
// （1 年 6 块 → 44 年 145 块），门槛取 20 留出余量：这道判据要拦的是「链条
// 断了一环」那个量级的塌陷，不是一两块钱的定价微调。
constexpr int kMinPriceRatio = 20;

constexpr double kFrame = 1.0 / 60.0;

// 八畦里用到的三畦。分开三畦是为了让三件事互不干扰：
//   链条畦 —— 走完整条链的那一株；
//   对照畦 —— 同一天种下、同样长满一年，但一滴绿液也不浇，采下即卖；
//   新苗畦 —— 生长之后才种下的一株，专供「未足年的一律回绝」那两条断言。
constexpr int kSlotChain = 0;
constexpr int kSlotControl = 1;
constexpr int kSlotFresh = 2;

// 仓库根：测试可能从 build/ 或工程根启动。判据用本章自己的地图与店铺数据，
// 找错根目录时报的是「找不到第 2 章的东西」，而不是一串莫名其妙的空断言。
std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "maps" / "ch02_yaopu.tmj") &&
            fs::exists(root / "data" / "shops" / "ch02_wairentang_yaoshang.json")) {
            return candidate;
        }
    }
    return ".";
}

// ---------------------------------------------------------------------------
// 价差判据
// ---------------------------------------------------------------------------
// 做成一个具名判据而不是就地摊一串 EXPECT，是为了能拿同一份判据去跑
// 「故意摆错 → 判据确实说不」的自检（见本文件末尾的 Ch02LoopVerdict 四条）。

// 一笔实际成交：卖的那一株几年份、柜台实际给了多少钱（钱袋前后之差）。
struct SaleRecord {
    int age = 0;
    int income = 0;
};

struct ChainLedger {
    SaleRecord plain;     // 对照组：采下即卖
    SaleRecord matured;   // 链条末端：种下 → 生长 → 催熟 → 收割 → 卖出
    int basePrice = 0;    // 这一味药的基价，供曲线核对那一半用
};

struct ChainVerdict {
    bool ok = true;
    std::string why;
};

ChainVerdict priceGapIsSignificant(const ChainLedger& ledger) {
    const auto fail = [](std::string why) { return ChainVerdict{false, std::move(why)}; };

    // ---- 先验判据。这几条不是形式主义：底下那个比值有三种空转法，
    // 分母塌成 0、两株其实是同一个年份、基价为 0 让整条曲线退化，
    // 每一种都会让「≥ 20 倍」变成一句永远成立的空话。
    if (ledger.basePrice <= 0) {
        return fail("基价是 " + std::to_string(ledger.basePrice) +
                    "，计价曲线整个退化了，这时候比什么都没意义");
    }
    if (ledger.plain.age != fanren::rules::kRipeAge) {
        return fail("对照组该是刚足年（" + std::to_string(fanren::rules::kRipeAge) +
                    " 年）就采下的那一株，实为 " + std::to_string(ledger.plain.age) + " 年");
    }
    if (ledger.matured.age <= ledger.plain.age) {
        return fail("链条末端那一株 " + std::to_string(ledger.matured.age) +
                    " 年，并不比对照组的 " + std::to_string(ledger.plain.age) +
                    " 年高——催熟或收割那一环断了");
    }
    if (ledger.plain.income <= 0) {
        return fail("对照组一分钱没卖到（" + std::to_string(ledger.plain.income) +
                    "）。分母塌了，比值就是无穷大，这条判据会被它蒙混过去");
    }
    if (ledger.matured.income <= 0) {
        return fail("链条末端一分钱没卖到（" + std::to_string(ledger.matured.income) + "）");
    }

    // ---- 相对判据：链条末端的收入 / 不催熟直接卖的收入 ≥ kMinPriceRatio。
    // 这是这条测试真正要断言的东西，也是第 2 章那一课的全部内容。
    if (ledger.matured.income < ledger.plain.income * kMinPriceRatio) {
        return fail("价差不显著：采下即卖 " + std::to_string(ledger.plain.income) + " 块，走完全链 " +
                    std::to_string(ledger.matured.income) + " 块，只有 " +
                    std::to_string(static_cast<double>(ledger.matured.income) /
                                   static_cast<double>(ledger.plain.income)) +
                    " 倍，不足 " + std::to_string(kMinPriceRatio) + " 倍");
    }

    // ---- 曲线核对：比值过得去还不够。一条比 herbPrice 更陡的错曲线
    // （Economy.cpp 里曾经真有过一条，44 年上给 493）比值照样合格，
    // 却会让剧情台词与柜台在同一家店给出两套价钱。
    const int plainExpected = herbPrice(ledger.basePrice, ledger.plain.age);
    if (ledger.plain.income != plainExpected) {
        return fail("对照组那一笔与 herbPrice 对不上：" + std::to_string(ledger.plain.age) +
                    " 年该得 " + std::to_string(plainExpected) + "，实得 " +
                    std::to_string(ledger.plain.income));
    }
    const int maturedExpected = herbPrice(ledger.basePrice, ledger.matured.age);
    if (ledger.matured.income != maturedExpected) {
        return fail("链条末端那一笔与 herbPrice 对不上：" + std::to_string(ledger.matured.age) +
                    " 年该得 " + std::to_string(maturedExpected) + "，实得 " +
                    std::to_string(ledger.matured.income));
    }
    return ChainVerdict{};
}

// ---------------------------------------------------------------------------
// 夹具
// ---------------------------------------------------------------------------
class Ch02Loop : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        state().day = 1;
        // 本章的境界。凝液速度按它折算（炼气与凡人同为 7 天一滴），
        // 容量三滴正是「攒满一瓶浇一株」那一课的前提。
        state().realm = Realm::QiRefining1;
    }

    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }

    // 按名字取当前地图上的设施。返回副本而不是指针：换一次图 TileMap 就整个
    // 重建，留着的指针会悄悄变成悬空的。
    MapObject facilityNamed(const std::string& name) const {
        const fanren::core::TileMap* map = app_.currentMap();
        if (map == nullptr) return MapObject{};
        for (const MapObject& object : map->objects) {
            if (object.type == "facility" && object.name == name) return object;
        }
        return MapObject{};
    }

    // 打开一处设施面板并把它取回来。走的是 Application::openFacility 那条真路
    // ——灵田是不是八槽、店铺 id 对不对，都写在地图上，绕过它就等于自己编一份。
    fanren::game::Scene* openFacility(const std::string& name) {
        const MapObject facility = facilityNamed(name);
        if (facility.name.empty()) {
            ADD_FAILURE() << state().mapId << " 上没有 " << name;
            return nullptr;
        }
        app_.openFacility(facility);
        app_.tick(kFrame);   // 场景是帧末才真正入栈的
        return app_.topScene();
    }

    void closePanel() {
        app_.popScene();
        app_.tick(kFrame);
    }

    // 把瓶子调成第 2 章段五那个状态：瓶在手、也已经知道绿液能催熟、瓶里是空的。
    // 绿液由日历自己凝出来（rules::refill，七天一滴），不在这里塞——「绿液真的
    // 少」那几条断言比的是前后差额，起点要是手填的就没有意义。
    void readyBottle() {
        fanren::rules::Bottle& bottle = state().bottle;
        bottle.owned = true;
        bottle.matureKnown = true;
        bottle.drops = 0;
        bottle.capacity = fanren::rules::kBottleCapacityPerTier;
        bottle.lastChargeDay = state().day;
    }

    const fanren::rules::FieldSlot& slot(int index) {
        return app_.state().fields.front().slots[static_cast<std::size_t>(index)];
    }

    Application app_;
};

// ---------------------------------------------------------------------------
// 闭环：五环一趟串到底
// ---------------------------------------------------------------------------
TEST_F(Ch02Loop, PlantGrowMatureHarvestSellRunsEndToEndWithASignificantPriceGap) {
    GameState& s = state();

    // ===================== 起手：真地图上的那块八槽灵田 =====================
    auto loaded = app_.loadMap(kMapYaopu, std::string{});
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_EQ(s.findField(kFieldId), nullptr) << "开局存档里本来没有这块田";

    auto* panel = dynamic_cast<FieldScene*>(openFacility(kFieldFacility));
    ASSERT_NE(panel, nullptr) << "灵田面板没开起来";
    EXPECT_EQ(panel->fieldId(), kFieldId) << "地图上的 ref_id 与契约对不上";
    const fanren::rules::SpiritField* field = s.findField(kFieldId);
    ASSERT_NE(field, nullptr) << "走到田边该按地图上的 slots 把田建出来";
    EXPECT_EQ(field->slots.size(), 8u) << "契约说八槽";

    const Item* herb = app_.data().findItem(kHerbId);
    ASSERT_NE(herb, nullptr);
    ASSERT_EQ(herb->price, kHuangjingBasePrice) << "基价一改，下面整张实算表都得跟着改";
    ASSERT_EQ(herb->maxAge, 44) << "一阶药的年份上限，三滴绿液正好浇到顶";

    readyBottle();
    // 种苗就是药本身（设计第 3 节）：没有单独的种子物品，播种消耗背包里的一株。
    // 章内这三株是药圃管事发的，这里直接放进背包——本条测试验的是经济链，
    // 发苗那一场戏由 tests/Ch02SliceTests.cpp 负责。
    s.addItem(kHerbId, 3, 0);
    ASSERT_EQ(s.itemCount(kMoneyId), 0) << "钱袋要从零起算，两笔进账才读得出差额";

    // ========================= 环 1 · 种下 =========================
    const int seedlingsBefore = s.itemCount(kHerbId);
    ASSERT_EQ(seedlingsBefore, 3);

    ASSERT_TRUE(panel->plantAt(app_, kSlotChain, kHerbId)) << panel->feedback();
    EXPECT_EQ(s.itemCount(kHerbId), seedlingsBefore - 1) << "播种必须消耗背包里的一株";
    ASSERT_TRUE(panel->plantAt(app_, kSlotControl, kHerbId)) << panel->feedback();
    EXPECT_EQ(s.itemCount(kHerbId), seedlingsBefore - 2);

    EXPECT_EQ(slot(kSlotChain).seedId, kHerbId);
    EXPECT_EQ(slot(kSlotChain).plantedDay, s.day);
    EXPECT_EQ(slot(kSlotChain).age, 0) << "刚下种的是零年份";
    EXPECT_FALSE(slot(kSlotChain).ripe);
    EXPECT_EQ(slot(kSlotChain).maxAge, herb->maxAge)
        << "年份上限要跟着种子一起进槽位，自然生长才知道该在哪停住";
    EXPECT_TRUE(slot(3).seedId.empty()) << "只该占用点中的那两畦";

    // 还没长起来就采，该被回绝——这一条是下一环「生长真的有用」的先验判据：
    // 少了它，「长满一年之后能采」只证明了能采，没证明生长是必需的。
    EXPECT_FALSE(panel->harvestAt(app_, kSlotChain)) << "零年份的苗采下来就是废的";
    EXPECT_FALSE(panel->feedback().empty()) << "回绝要说明原因，静默无反应一律被当成 bug";
    EXPECT_EQ(slot(kSlotChain).seedId, kHerbId) << "没采成就不该腾空";
    EXPECT_EQ(s.itemCount(kHerbId), seedlingsBefore - 2) << "没采成就不该往背包里塞东西";

    // ========================= 环 2 · 生长 =========================
    const int dayBeforeGrowing = s.day;
    app_.advanceDays(fanren::rules::kDaysPerYear);
    EXPECT_EQ(s.day - dayBeforeGrowing, fanren::rules::kDaysPerYear) << "日历必须真的走";

    EXPECT_EQ(slot(kSlotChain).age, fanren::rules::kRipeAge) << "一年一年份，跳时要连灵田一起结算";
    EXPECT_TRUE(slot(kSlotChain).ripe) << "满一年方可采（rules::kRipeAge）";
    EXPECT_EQ(slot(kSlotControl).age, fanren::rules::kRipeAge) << "对照畦同一天种下，长势该一样";
    EXPECT_TRUE(slot(kSlotControl).ripe);
    EXPECT_TRUE(slot(3).seedId.empty()) << "空畦不该凭空长出东西";

    // 顺带把瓶子凝满：七天一滴，一年下来早到顶了。这三滴是下一环的本钱。
    EXPECT_EQ(s.bottle.drops, s.bottle.capacity) << "一年下来瓶子该满了";
    ASSERT_EQ(s.bottle.drops, 3) << "炼气期满瓶三滴，下面 1 → 11 → 22 → 44 全靠它";

    // 生长之后才种下的一株：绿液在手也浇不进去，足年之前也采不得。
    // 这两条拦的是「种下就浇」那条劣化路线（Bottle.h 的 NotRipe 有长注释），
    // 它一旦松开，槽位数与生长时间就都不再是瓶颈，本章的收益表整个失准。
    ASSERT_TRUE(panel->plantAt(app_, kSlotFresh, kHerbId)) << panel->feedback();
    EXPECT_EQ(s.itemCount(kHerbId), 0) << "三株种苗全下地了";
    ASSERT_EQ(slot(kSlotFresh).age, 0);

    const int dropsBeforeRefusal = s.bottle.drops;
    EXPECT_FALSE(panel->matureAt(app_, kSlotFresh)) << "不足一年的苗，绿液浇不进去";
    EXPECT_EQ(s.bottle.drops, dropsBeforeRefusal) << "催不成就一滴也不该扣";
    // 理由用机器码核，不比中文串：文案会改，failure 不会（Bottle.h 的 MatureFailure）。
    fanren::rules::Bottle probe = s.bottle;
    const fanren::rules::MatureResult dry =
        fanren::rules::matureHerb(probe, slot(kSlotFresh).age, herb->maxAge);
    EXPECT_EQ(dry.failure, fanren::rules::MatureFailure::NotRipe)
        << "回绝的理由该是「尚未足年」，实为：" << dry.reason;
    EXPECT_FALSE(panel->harvestAt(app_, kSlotFresh)) << "不足一年的苗，采下来就是废的";
    EXPECT_EQ(slot(kSlotFresh).age, 0) << "两次回绝都不该改动这一畦";

    // ========================= 环 3 · 催熟 =========================
    // 1 → 11 → 22 → 44。每一步都断三件事：绿液真的少一滴、年份按 matureHerb
    // 跃升、旁边那畦分毫不动。
    const std::vector<int> expectedLadder{11, 22, 44};
    std::vector<int> actualLadder;
    for (const int expectedAge : expectedLadder) {
        const int dropsBefore = s.bottle.drops;
        const int ageBefore = slot(kSlotChain).age;
        ASSERT_TRUE(panel->matureAt(app_, kSlotChain)) << panel->feedback();
        EXPECT_EQ(s.bottle.drops, dropsBefore - 1)
            << "催熟必须真的从瓶子里扣掉一滴（校对报告 BLOCKER-2 正是这一条）";
        EXPECT_GT(slot(kSlotChain).age, ageBefore) << "浇下去年份得涨";
        EXPECT_TRUE(slot(kSlotChain).ripe) << "拔上去的年份同样算足年";
        actualLadder.push_back(slot(kSlotChain).age);
        EXPECT_EQ(slot(kSlotChain).age, expectedAge)
            << "一滴绿液把年份翻一倍且至少推进 " << fanren::rules::kMatureMinYears << " 年";
    }
    EXPECT_EQ(actualLadder, expectedLadder) << "实算表钉的就是 1 → 11 → 22 → 44 这条梯子";
    EXPECT_EQ(s.bottle.drops, 0) << "三滴全浇出去了，瓶子该是空的";
    EXPECT_EQ(slot(kSlotChain).age, herb->maxAge)
        << "正好落在这一味药自己的上限上，不是测试写死的数";
    EXPECT_EQ(slot(kSlotControl).age, fanren::rules::kRipeAge)
        << "对照畦一滴也没浇，年份不该跟着动——动了说明催熟浇错了畦";

    // 到顶就得停：再凝一滴出来也浇不进去。
    const int dayBeforeTopUp = s.day;
    app_.advanceDays(fanren::rules::kBaseChargeDays);
    ASSERT_EQ(s.day - dayBeforeTopUp, fanren::rules::kBaseChargeDays);
    ASSERT_EQ(s.bottle.drops, 1) << "七天正好再凝一滴，下面那条回绝才有意义";
    EXPECT_FALSE(panel->matureAt(app_, kSlotChain)) << "已至上限，第四滴该被回绝";
    EXPECT_EQ(s.bottle.drops, 1) << "回绝了就不该扣绿液";
    EXPECT_EQ(slot(kSlotChain).age, herb->maxAge) << "回绝了年份也不该动";

    fanren::rules::Bottle atMaxProbe = s.bottle;
    const fanren::rules::MatureResult atMax =
        fanren::rules::matureHerb(atMaxProbe, slot(kSlotChain).age, herb->maxAge);
    EXPECT_EQ(atMax.failure, fanren::rules::MatureFailure::AtMaxAge)
        << "回绝的理由该是「已至上限」，实为：" << atMax.reason;

    // 面板上也必须是灰的。面板说能点、点下去被回绝，玩家只会当成 bug
    // （设计第 3 节「催熟解锁必须有明确事件」点名过这个观感问题）。
    const auto actions =
        FieldScene::buildSlotActionItems(s.bottle, slot(kSlotChain), herb->maxAge);
    ASSERT_EQ(actions.size(), 3u) << "催熟 / 采收 / 返回，三条写死";
    EXPECT_FALSE(actions[0].enabled) << "年份到顶了，催熟那一项该置灰";
    EXPECT_FALSE(actions[0].disabledReason.empty()) << "置灰要写明为什么";
    EXPECT_TRUE(actions[1].enabled) << "足年了，采收那一项该亮着";

    // ========================= 环 4 · 收割 =========================
    // 对照组那一株先收：同一天种下、同样长过，只是一滴绿液也没浇。
    // 它比那三滴绿液更要紧——没有它，「价差显著」就没有分母。
    const int plainAge = slot(kSlotControl).age;
    ASSERT_EQ(plainAge, fanren::rules::kRipeAge) << "对照组必须是刚足年就采下的那一株";
    ASSERT_TRUE(panel->harvestAt(app_, kSlotControl)) << panel->feedback();
    EXPECT_EQ(s.itemCountOfAge(kHerbId, plainAge), 1);
    EXPECT_TRUE(slot(kSlotControl).seedId.empty()) << "收完该腾空";

    // 自然生长同样受这一味药自己的上限封顶：链条那畦搁上五年也还是 44 年，
    // 否则「种下去搁几百年再收」会原样刷出同一份天价，只是慢一些。
    // 新苗那畦照旧一年一年份长，它是这一条的先验判据——跳时若压根没生效，
    // 「到顶之后不再长」会在一块死掉的日历上空转着通过。
    const int chainAgeBeforeIdling = slot(kSlotChain).age;
    const int freshAgeBeforeIdling = slot(kSlotFresh).age;
    app_.advanceDays(fanren::rules::kDaysPerYear * 5);
    EXPECT_EQ(slot(kSlotFresh).age, freshAgeBeforeIdling + 5) << "没到顶的那畦该照旧长";
    EXPECT_EQ(slot(kSlotChain).age, chainAgeBeforeIdling) << "到顶之后自然生长也该停住";

    // 年份必须跟着进背包。不带年份，催熟攒下的价值在这一步就丢光，
    // 整条经济循环断在这里（校对报告 BLOCKER-1 就是这个形状）。
    const int maturedAge = slot(kSlotChain).age;
    ASSERT_TRUE(panel->harvestAt(app_, kSlotChain)) << panel->feedback();
    EXPECT_TRUE(slot(kSlotChain).seedId.empty()) << "收完该腾空";
    EXPECT_EQ(slot(kSlotChain).age, 0) << "腾空的畦不该留着上一株的年份";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, maturedAge), 1) << "收割必须把年份一并带进背包";

    // 先验判据：下面「卖完就找不到了」那组断言，在背包本来就是空的时候照样
    // 成立。所以先把背包里该有什么钉死。
    ASSERT_EQ(s.itemCount(kHerbId), 2) << "背包里该正好是采下的这两株（三株种苗都下地了）";
    ASSERT_EQ(s.itemCountOfAge(kHerbId, 0), 0) << "零年份的种苗一株不剩，全在地里";
    closePanel();

    // ========================= 环 5 · 卖出 =========================
    // 走真店铺：地图上的柜台 → ShopScene → 按年份分堆 → 成交。
    // 对照组那一株先卖（它是分母），末端那一株后卖。
    auto loadedShop = app_.loadMap(kMapWairentang, std::string{});
    ASSERT_TRUE(loadedShop.ok) << loadedShop.error;
    auto* counter = dynamic_cast<ShopScene*>(openFacility(kShopFacility));
    ASSERT_NE(counter, nullptr) << "收药处的面板没开起来";
    ASSERT_EQ(counter->shopId(), kShopId) << "地图上的 ref_id 与 data/shops 对不上";

    const fanren::rules::Shop* shop = app_.shop(kShopId);
    ASSERT_NE(shop, nullptr);

    // 两株药、两个年份，柜台上必须是两行两个价钱——合成一行就等于把第 2 章
    // 要教的那一课抹掉。
    auto stacks = ShopScene::buildSellStacks(app_.data(), s, *shop);
    ASSERT_EQ(stacks.size(), 2u) << "同一味药的两个年份该各占一行";
    ASSERT_EQ(stacks[0].herbAge, plainAge) << "定序：同一味药内年份由低到高";
    ASSERT_EQ(stacks[1].herbAge, maturedAge);
    EXPECT_LT(stacks[0].unitPrice, stacks[1].unitPrice) << "年份高的那行该更值钱";

    ChainLedger ledger;
    ledger.basePrice = herb->price;

    const int purseBeforePlain = spiritStones(s);
    ASSERT_TRUE(counter->sellAt(app_, 0)) << counter->feedback();
    ledger.plain.age = plainAge;
    ledger.plain.income = spiritStones(s) - purseBeforePlain;
    EXPECT_EQ(s.itemCountOfAge(kHerbId, plainAge), 0) << "卖掉的那一堆该没了";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, maturedAge), 1)
        << "扣错了堆：玩家点的是对照组那株，掉的却是催熟过的那株";

    stacks = ShopScene::buildSellStacks(app_.data(), s, *shop);
    ASSERT_EQ(stacks.size(), 1u);
    ASSERT_EQ(stacks[0].herbAge, maturedAge);

    const int purseBeforeMatured = spiritStones(s);
    ASSERT_TRUE(counter->sellAt(app_, 0)) << counter->feedback();
    ledger.matured.age = maturedAge;
    ledger.matured.income = spiritStones(s) - purseBeforeMatured;
    EXPECT_EQ(s.itemCountOfAge(kHerbId, maturedAge), 0);
    EXPECT_EQ(s.itemCount(kHerbId), 0) << "两株都卖出去了";
    EXPECT_EQ(spiritStones(s), ledger.plain.income + ledger.matured.income)
        << "钱袋里的数该正好是两笔进账之和，别处不许再有一本账";
    closePanel();

    // ===================== 链条末端：价差显著 =====================
    const ChainVerdict verdict = priceGapIsSignificant(ledger);
    EXPECT_TRUE(verdict.ok) << verdict.why;

    // 把那张实算表也对一遍。它与上面那条相对判据是两件事：判据管「链条通不
    // 通」，这几个数管「教学用的那三个数字没被改掉」。
    EXPECT_EQ(ledger.plain.income, 6) << "docs/ch02-design.md 第 3 节：一年份 6 块";
    EXPECT_EQ(ledger.matured.income, 145) << "同上：四十四年份 145 块";
    EXPECT_DOUBLE_EQ(shop->sellRate, 1.0) << "收购倍率被实算表钉住，改它要连那张表一起改";

    const double ratio =
        static_cast<double>(ledger.matured.income) / static_cast<double>(ledger.plain.income);
    EXPECT_GE(ratio, static_cast<double>(kMinPriceRatio));
    std::cout << "\n[ch02 闭环实测] 种下 → 生长 " << fanren::rules::kDaysPerYear << " 日 → 催熟 "
              << expectedLadder.size() << " 滴（" << fanren::rules::kRipeAge << " → "
              << maturedAge << " 年）→ 收割 → 卖出 " << ledger.matured.income << " 块；"
              << "对照组采下即卖 " << ledger.plain.income << " 块；价差 " << ratio << " 倍"
              << std::endl;
}

// ---------------------------------------------------------------------------
// 判据自检：证明上面那条价差判据真的有牙
// ---------------------------------------------------------------------------
// 照 tests/Ch02SliceTests.cpp 的 Ch02EndState 三条写。先证明判据咬得动人，
// 再拿它去验真实的运行结果；否则一条永远返回 true 的判据与没写没有区别。

TEST(Ch02LoopVerdict, ThePriceGapVerdictAcceptsTheRealChainNumbers) {
    // 先证明它不是一律说不：实算表那三个数喂进去必须过。
    ChainLedger real;
    real.basePrice = kHuangjingBasePrice;
    real.plain = SaleRecord{1, 6};
    real.matured = SaleRecord{44, 145};
    const ChainVerdict verdict = priceGapIsSignificant(real);
    EXPECT_TRUE(verdict.ok) << verdict.why;
}

TEST(Ch02LoopVerdict, ThePriceGapVerdictCatchesAHarvestThatDroppedTheYears) {
    // 收割不带年份（校对报告 BLOCKER-1 的形状）：末端那一株进背包时年份掉成
    // 了 1，柜台照 1 年计价，于是两笔一模一样。
    ChainLedger lost;
    lost.basePrice = kHuangjingBasePrice;
    lost.plain = SaleRecord{1, 6};
    lost.matured = SaleRecord{1, 6};
    const ChainVerdict verdict = priceGapIsSignificant(lost);
    EXPECT_FALSE(verdict.ok) << "年份在收割那一步丢光了，判据却说没事——它没有牙";
    EXPECT_NE(verdict.why.find("年"), std::string::npos) << verdict.why;
}

TEST(Ch02LoopVerdict, ThePriceGapVerdictCatchesACounterThatIgnoresTheYears) {
    // 年份带到了，柜台却不按年份给钱（计价那一环断了）：44 年那株照旧只卖 6 块。
    ChainLedger flat;
    flat.basePrice = kHuangjingBasePrice;
    flat.plain = SaleRecord{1, 6};
    flat.matured = SaleRecord{44, 6};
    const ChainVerdict verdict = priceGapIsSignificant(flat);
    EXPECT_FALSE(verdict.ok) << "四十四年的药卖出一年份的价钱，判据却说没事";
    EXPECT_NE(verdict.why.find("价差不显著"), std::string::npos) << verdict.why;
}

TEST(Ch02LoopVerdict, ThePriceGapVerdictRefusesACollapsedBaselineInsteadOfWavingItThrough) {
    // 这是这条判据的空转法，与「背包空了也算找不到」同形：分母塌成 0 时
    // 「末端 / 对照 ≥ 20」永远成立——比值不存在，检查却通过。
    ChainLedger noBaseline;
    noBaseline.basePrice = kHuangjingBasePrice;
    noBaseline.plain = SaleRecord{1, 0};
    noBaseline.matured = SaleRecord{44, 145};
    const ChainVerdict verdict = priceGapIsSignificant(noBaseline);
    EXPECT_FALSE(verdict.ok) << "对照组一分钱没卖到也算价差显著？那这条判据等于没写";
    EXPECT_NE(verdict.why.find("分母"), std::string::npos) << verdict.why;

    // 基价为 0 时整条曲线退化，所有年份都卖 0 块，同样是空转。
    ChainLedger noBase;
    noBase.plain = SaleRecord{1, 0};
    noBase.matured = SaleRecord{44, 0};
    EXPECT_FALSE(priceGapIsSignificant(noBase).ok) << "整条曲线都是 0 也算通过？";
}

TEST(Ch02LoopVerdict, ThePriceGapVerdictCatchesASteeperParallelCurveThatPassesTheRatio) {
    // 光有比值不够。Economy.cpp 里曾经真的有过一条平行曲线
    // （1 + 0.02·age + 0.05·age²），它在 44 年上给 493、在 1 年上给 5：
    // 比值 98 倍，远超门槛，却与 herbPrice 分家，会让剧情台词说 145、柜台给
    // 493。判据必须连这个也咬住——这正是只钉一个数的检查抓不住的那种错法。
    ChainLedger parallelCurve;
    parallelCurve.basePrice = kHuangjingBasePrice;
    parallelCurve.plain = SaleRecord{1, 5};
    parallelCurve.matured = SaleRecord{44, 493};
    ASSERT_GE(parallelCurve.matured.income, parallelCurve.plain.income * kMinPriceRatio)
        << "样本自己得先过得了比值那一关，否则这条用例证明不了什么";

    const ChainVerdict verdict = priceGapIsSignificant(parallelCurve);
    EXPECT_FALSE(verdict.ok) << "一条更陡的错曲线靠比值蒙混过关了";
    EXPECT_NE(verdict.why.find("herbPrice"), std::string::npos) << verdict.why;
}

}  // namespace
