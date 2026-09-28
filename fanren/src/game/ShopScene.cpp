#include "game/ShopScene.h"

#include <algorithm>
#include <cstddef>
#include <utility>

#include "game/Application.h"
#include "game/Wording.h"

namespace fanren::game {
namespace {

constexpr int kPanelX = 140;
constexpr int kPanelY = 60;
constexpr int kPanelW = 1000;
constexpr int kPanelH = 600;

constexpr int kStatusPercent = 45;

// 主菜单三项，下标写死在 update 里，用具名常量而不是字面量。
constexpr int kActionBuy = 0;
constexpr int kActionSell = 1;
constexpr int kActionLeave = 2;

// 物品显示名。查不到就回显 id：画面上一眼看出是哪件货没进 data。
[[nodiscard]] std::string itemName(const core::GameData& data, const std::string& itemId) {
    if (const core::Item* item = data.findItem(itemId)) return item->name;
    return itemId;
}

// 灵草才把年份写进行首。非灵草的 herbAge 恒为 0，写出来只是噪音。
[[nodiscard]] std::string stackLabel(const core::GameData& data, const std::string& itemId,
                                     int herbAge) {
    std::string label = itemName(data, itemId);
    if (herbAge > 0) label += "（" + std::to_string(herbAge) + " 年）";
    return label;
}

}  // namespace

int spiritStones(const core::GameState& state) {
    return state.itemCount(kSpiritStoneItemId);
}

ShopScene::ShopScene(std::string shopId) : shopId_(std::move(shopId)) {}

std::vector<ui::ListItem> ShopScene::buildBuyItems(const core::GameData& data,
                                                   const core::GameState& state,
                                                   const rules::Shop& shop) {
    std::vector<ui::ListItem> items;
    items.reserve(shop.entries.size() + 1);

    const int purse = spiritStones(state);
    for (const rules::ShopEntry& entry : shop.entries) {
        ui::ListItem row;
        const core::Item* item = data.findItem(entry.itemId);
        if (item == nullptr) {
            // 货架上列着一件 data 里没有的东西。照样显示并写明原因：
            // 悄悄跳过等于让一条写坏的数据永远没人发现。
            row.label = entry.itemId;
            row.enabled = false;
            row.disabledReason = "物品册上查无此物";
            items.push_back(std::move(row));
            continue;
        }

        // 店里卖的灵草都是没年份的种苗，年份一律按 0 问价。
        const int price = rules::buyPrice(shop, *item, 0);
        row.label = item->name;
        row.detail = std::to_string(price) + " " + currencyName(state) + "　" +
                     (entry.stock < 0 ? std::string("不限") : "存 " + std::to_string(entry.stock));
        if (entry.stock == 0) {
            row.enabled = false;
            row.disabledReason = "已售罄";
        } else if (purse < price) {
            // 只把字变灰玩家会当成 bug，差多少必须写出来——这是他下一步的目标。
            row.enabled = false;
            row.disabledReason = std::string(currencyName(state)) + "不足，尚差 " +
                                 std::to_string(price - purse);
        }
        items.push_back(std::move(row));
    }

    ui::ListItem back;
    back.label = "返回";
    items.push_back(std::move(back));
    return items;
}

std::vector<ShopScene::SellStack> ShopScene::buildSellStacks(const core::GameData& data,
                                                             const core::GameState& state,
                                                             const rules::Shop& shop) {
    std::vector<SellStack> stacks;

    for (const core::BagEntry& entry : state.bag) {
        if (entry.count <= 0) continue;
        // 钱不能卖给店家换钱。
        if (entry.itemId == kSpiritStoneItemId) continue;
        const core::Item* item = data.findItem(entry.itemId);
        if (item == nullptr || !item->tradeable) continue;

        // 同一味药的不同年份是两堆，价钱各算各的——这正是本面板要让玩家
        // 看见的那件事，合并成一行就没得看了。
        const auto same = std::find_if(stacks.begin(), stacks.end(), [&](const SellStack& s) {
            return s.itemId == entry.itemId && s.herbAge == entry.herbAge;
        });
        if (same != stacks.end()) {
            same->count += entry.count;
            continue;
        }

        SellStack stack;
        stack.itemId = entry.itemId;
        stack.herbAge = entry.herbAge;
        stack.count = entry.count;
        stack.unitPrice = rules::sellPrice(shop, *item, entry.herbAge);
        stacks.push_back(std::move(stack));
    }

    // 定序：itemId 升序，同一味药内年份由低到高。背包本身的顺序是不稳的
    // （removeItem 会把整个背包按年份重排），跟着它走会让列表行序无端跳动，
    // 而玩家记住的是「第二行是那株四十四年的」。
    std::sort(stacks.begin(), stacks.end(), [](const SellStack& a, const SellStack& b) {
        if (a.itemId != b.itemId) return a.itemId < b.itemId;
        return a.herbAge < b.herbAge;
    });
    return stacks;
}

std::vector<ui::ListItem> ShopScene::buildSellItems(PanelStage stage,
                                                    const core::GameData& data,
                                                    const std::vector<SellStack>& stacks) {
    std::vector<ui::ListItem> items;
    items.reserve(stacks.size() + 1);

    for (const SellStack& stack : stacks) {
        ui::ListItem row;
        row.label = stackLabel(data, stack.itemId, stack.herbAge);
        row.detail = std::to_string(stack.unitPrice) + " " + currencyName(stage) +
                     "　共 " + std::to_string(stack.count);
        items.push_back(std::move(row));
    }

    ui::ListItem back;
    back.label = "返回";
    items.push_back(std::move(back));
    return items;
}

bool ShopScene::buyAt(Application& app, int entryIndex) {
    rules::Shop* shop = app.shop(shopId_);
    if (shop == nullptr) {
        feedback_ = "此处并无这家店。";
        return false;
    }
    if (entryIndex < 0 || static_cast<std::size_t>(entryIndex) >= shop->entries.size()) {
        feedback_ = "货架上并无此物。";
        return false;
    }

    rules::ShopEntry& entry = shop->entries[static_cast<std::size_t>(entryIndex)];
    const core::Item* item = app.data().findItem(entry.itemId);
    if (item == nullptr) {
        feedback_ = "店家自己也说不清这是什么（" + entry.itemId + " 不在物品册上）。";
        return false;
    }
    // stock < 0 是无限供应；恰好为 0 才是卖光了。
    if (entry.stock == 0) {
        feedback_ = item->name + "已售罄，店家说改日再来。";
        return false;
    }

    const int price = rules::buyPrice(*shop, *item, 0);
    const int purse = spiritStones(app.state());
    if (purse < price) {
        feedback_ = std::string("囊中") + currencyName(app.state()) +
                    "不足，尚差 " + std::to_string(price - purse) + " 块。";
        return false;
    }

    // 先付钱再拿货。反过来会在扣钱失败时白送一件货，而扣钱失败这条路
    // 上面刚查过、今天走不到——正因为走不到，写反了也测不出来。
    if (price > 0 && !app.state().removeItem(kSpiritStoneItemId, price)) {
        feedback_ = std::string("囊中") + currencyName(app.state()) +
                    "不足，付不出这笔钱。";
        return false;
    }
    app.state().addItem(entry.itemId, 1, 0);
    if (entry.stock > 0) --entry.stock;

    feedback_ = "买下" + item->name + "一份，付" +
                currencyAmount(app.state(), price) + "。";
    return true;
}

bool ShopScene::sellAt(Application& app, int stackIndex) {
    rules::Shop* shop = app.shop(shopId_);
    if (shop == nullptr) {
        feedback_ = "此处并无这家店。";
        return false;
    }

    // 就地重算一遍分堆，不信任上一帧建的那份列表：中间可能已经卖掉过、
    // 或者剧情脚本刚拿走了什么。下标的含义由 buildSellStacks 的定序保证。
    const std::vector<SellStack> stacks = buildSellStacks(app.data(), app.state(), *shop);
    if (stackIndex < 0 || static_cast<std::size_t>(stackIndex) >= stacks.size()) {
        feedback_ = "囊中并无此物。";
        return false;
    }

    const SellStack& stack = stacks[static_cast<std::size_t>(stackIndex)];
    // 一文不值的东西不收。给 0 灵石还把货收走，在玩家眼里就是「点一下东西没了」，
    // 比不让卖难解释得多。眼下的定价与倍率下算不出 0 来，但倍率是数据里的旋钮，
    // 调低一档就到了——这道闸在那之前先摆好。
    if (stack.unitPrice <= 0) {
        feedback_ = "此物店家不收，白搭上一趟。";
        return false;
    }
    // 必须按年份精扣这一堆。走 GameState::removeItem 会先扣年份最低的那堆——
    // 玩家点的是四十四年那株，掉的却是一年份那株，账面上还看不出来。
    if (!app.state().removeItemOfAge(stack.itemId, 1, stack.herbAge)) {
        feedback_ = "囊中并无此物。";
        return false;
    }
    app.state().addItem(kSpiritStoneItemId, stack.unitPrice, 0);

    feedback_ = "卖出" + stackLabel(app.data(), stack.itemId, stack.herbAge) + "一份，得" +
                std::string(currencyName(app.state())) + " " +
                std::to_string(stack.unitPrice) + " 块。";
    return true;
}

void ShopScene::enterMain(Application& app) {
    mode_ = Mode::Main;

    const rules::Shop* shop = app.shop(shopId_);
    std::vector<ui::ListItem> items;

    ui::ListItem buy;
    buy.label = "买入";
    if (shop == nullptr || shop->entries.empty()) {
        buy.enabled = false;
        buy.disabledReason = "货架上空空如也";
    } else {
        buy.detail = "货架上 " + std::to_string(shop->entries.size()) + " 样";
    }
    items.push_back(std::move(buy));

    ui::ListItem sell;
    sell.label = "卖出";
    const std::size_t sellable =
        shop == nullptr ? 0 : buildSellStacks(app.data(), app.state(), *shop).size();
    if (sellable == 0) {
        sell.enabled = false;
        sell.disabledReason = "囊中无可卖之物";
    } else {
        sell.detail = "囊中 " + std::to_string(sellable) + " 样";
    }
    items.push_back(std::move(sell));

    ui::ListItem leave;
    leave.label = "离开";
    items.push_back(std::move(leave));

    main_.reset();
    main_.setPageSize(listPageRows(app.theme()));
    main_.setItems(std::move(items));
}

void ShopScene::enterBuy(Application& app) {
    mode_ = Mode::Buy;
    buy_.reset();
    buy_.setPageSize(listPageRows(app.theme()));
    if (const rules::Shop* shop = app.shop(shopId_)) {
        buy_.setItems(buildBuyItems(app.data(), app.state(), *shop));
        return;
    }
    ui::ListItem back;
    back.label = "返回";
    buy_.setItems({std::move(back)});
}

void ShopScene::enterSell(Application& app) {
    mode_ = Mode::Sell;
    sell_.reset();
    sell_.setPageSize(listPageRows(app.theme()));
    sellStacks_.clear();
    if (const rules::Shop* shop = app.shop(shopId_)) {
        sellStacks_ = buildSellStacks(app.data(), app.state(), *shop);
    }
    sell_.setItems(buildSellItems(wordingStage(app.state()), app.data(), sellStacks_));
}

void ShopScene::onEnter(Application& app) {
    rules::Shop* shop = app.shop(shopId_);
    shopFound_ = shop != nullptr;
    if (shop == nullptr) {
        // 店不在 data 里（脚本或地图写错了 id）。面板照开，只是什么也买不到：
        // 默不作声地把这条路吞掉，玩家看到的就是「按了没反应」。
        feedback_ = "此处并无这家店。";
    } else {
        // 补货按当前日结算。玩家离开三个月再回来，货架该补的一次补齐，
        // 这一步是 rules::restock 的唯一调用点。
        rules::restock(*shop, app.state().day);
        feedback_ = "货在架上，价随年份。要买要卖，自己看着办。";
    }
    enterMain(app);
}

void ShopScene::leave(Application& app) {
    if (done_) return;
    done_ = true;
    script::CommandResult result;
    // 店 id 写错时如实报失败，让脚本能分支。玩家从地图设施走进来的那条路上
    // 没有挂起的命令，completeCommand 会自己忽略掉这次回填。
    result.ok = shopFound_;
    app.completeCommand(result);
}

bool ShopScene::updateBuy(Application& app) {
    if (buy_.update(app.engine())) {
        const rules::Shop* shop = app.shop(shopId_);
        const int index = buy_.selection();
        if (shop == nullptr || index < 0 ||
            static_cast<std::size_t>(index) >= shop->entries.size()) {
            enterMain(app);   // 末项「返回」，以及没有店时的任何确认
            return true;
        }
        buyAt(app, index);
        // 钱变了、库存变了，买不起与售罄的标注得跟着重算。
        enterBuy(app);
        return true;
    }
    if (buy_.cancelled()) enterMain(app);
    return true;
}

bool ShopScene::updateSell(Application& app) {
    if (sell_.update(app.engine())) {
        const int index = sell_.selection();
        if (index < 0 || static_cast<std::size_t>(index) >= sellStacks_.size()) {
            enterMain(app);   // 末项「返回」
            return true;
        }
        sellAt(app, index);
        // 卖完一堆可能整堆没了，列表必须重建，否则下一次确认打到的是别人。
        enterSell(app);
        if (sellStacks_.empty()) enterMain(app);
        return true;
    }
    if (sell_.cancelled()) enterMain(app);
    return true;
}

bool ShopScene::updateMain(Application& app) {
    if (main_.update(app.engine())) {
        switch (main_.selection()) {
            case kActionBuy:
                enterBuy(app);
                return true;
            case kActionSell:
                enterSell(app);
                return true;
            case kActionLeave:
            default:
                leave(app);
                return false;
        }
    }
    if (main_.cancelled()) {
        leave(app);
        return false;
    }
    return true;
}

bool ShopScene::update(Application& app, double) {
    // 已经关过店（含无头测试直接调 leave 的那条路）就别再更新了，
    // 等主循环把自己弹掉。
    if (done_) return false;

    switch (mode_) {
        case Mode::Buy:  return updateBuy(app);
        case Mode::Sell: return updateSell(app);
        case Mode::Main:
        default:         return updateMain(app);
    }
}

void ShopScene::renderStatus(Application& app, const engine::Rect& area) const {
    engine::Engine& engine = app.engine();
    const ui::Theme& theme = app.theme();
    if (area.w <= 0 || area.h <= 0) return;

    const int row = theme.bodyFontSize + theme.lineSpacing;
    int y = area.y;
    const auto line = [&](const std::string& textLine) {
        engine.drawText(textLine, area.x, y, theme.bodyFontSize, theme.paper, theme.bodyStyle);
        y += row;
    };

    line("日期　第 " + std::to_string(app.state().day) + " 日");
    // 兜里还剩多少必须摆在面上：看不见余额，玩家无从判断这一笔买不买得起。
    line(std::string(currencyName(app.state())) + "　" +
         std::to_string(spiritStones(app.state())) + " 块");

    const engine::Rect note{area.x, y, area.w, std::max(0, area.y + area.h - y)};
    ui::drawTextBlock(engine, feedback_, note, theme.bodyFontSize, theme);
}

void ShopScene::render(Application& app) {
    engine::Engine& engine = app.engine();
    const ui::Theme& theme = app.theme();

    const rules::Shop* shop = app.shop(shopId_);
    // 店名走文案表。没登记 nameKey（或这家店根本不在 data 里）就把 id 顶上去：
    // 与本项目别处同一条规矩——查不到就回显 id，画面上一眼看出是哪家店没配好，
    // 显示一个笼统的「店铺」反而把数据错藏起来了。
    const std::string title =
        (shop != nullptr && !shop->nameKey.empty()) ? app.text(shop->nameKey) : shopId_;

    const engine::Rect panel{kPanelX, kPanelY, kPanelW, kPanelH};
    ui::drawScrim(engine, theme);
    ui::drawPanel(engine, panel, title, theme);

    const engine::Rect content = ui::panelContentArea(panel, title, theme);
    const int statusW = content.w * kStatusPercent / 100;
    // 状态栏让出左右内边距：字贴着金线外框写，读起来像是框没画完。
    renderStatus(app, engine::Rect{content.x + theme.padding, content.y + theme.lineSpacing,
                                   statusW - theme.padding * 2, content.h - theme.lineSpacing});

    const engine::Rect listArea{content.x + statusW, content.y, content.w - statusW, content.h};
    ui::drawRuleV(engine, static_cast<float>(listArea.x), static_cast<float>(content.y),
                  static_cast<float>(content.h), theme);
    switch (mode_) {
        case Mode::Buy:
            buy_.render(engine, listArea, theme);
            break;
        case Mode::Sell:
            sell_.render(engine, listArea, theme);
            break;
        case Mode::Main:
        default:
            main_.render(engine, listArea, theme);
            break;
    }
}

int ShopScene::listPageRows(const ui::Theme& theme) {
    // 标题带按「有标题」算：本面板的标题从来不为空，render() 也是这么摆的。
    return ui::listRowsThatFit(kPanelH - ui::panelTitleBandHeight(true, theme), theme);
}

}  // namespace fanren::game
