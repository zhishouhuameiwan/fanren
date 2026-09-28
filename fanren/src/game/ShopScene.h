#pragma once
// 商店面板：买货、卖货。
//
// 与灵田、修炼两块面板同一条分层纪律：价钱怎么算、库存怎么补，全在
// core/rules/Economy.h 里；这里只负责把货架与背包翻成列表、把玩家的选择翻成
// 对 rules 层的调用，再把结果翻回人话。面板不许再写第二套计价——界面各算一套
// 正是这一轮要消掉的东西（见 core/rules/Economy.cpp 顶部）。
//
// 灵草按年份分堆列出：同一味药，一年份与四十四年份是两行、两个价钱。这是第 2
// 章要教给玩家的那一课的落点（docs/ch02-design.md 第 3 节），把它们合并成一行
// 就等于把那一课抹掉。
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Economy.h"
#include "game/Scene.h"
#include "game/Wording.h"
#include "ui/Widgets.h"

namespace fanren::game {

// 玩家的钱。灵石在本作里就是一件普通物品（data/items/materials/lingshi.json），
// 存在 GameState::bag 里——**没有**第二本账：买卖一律 addItem / removeItem 这一处，
// 另设一个 spiritStones 字段就意味着两边迟早对不上，而对不上的那一天没人查得出
// 是哪一笔交易记漏了。
inline constexpr const char* kSpiritStoneItemId = "material_lingshi";

// 玩家身上有多少灵石。只有这一个口径，面板与测试都问它。
[[nodiscard]] int spiritStones(const core::GameState& state);

class ShopScene : public Scene {
public:
    explicit ShopScene(std::string shopId);

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    // 本面板的列表页大小：面板高度去掉标题带之后画得满几行。
    // enter*() 设页大小与 render() 摆列表区必须走同一份高度，否则会出现
    // 「区域明明画得下十几行，却因为页大小是默认的 8 而只显示 8 行」——
    // 多出来的那几行玩家得按方向键才找得到，而他多半不知道下面还有。
    // 公开是为了让无头测试能直接断言「今天这张表一行都不会被藏起来」。
    [[nodiscard]] static int listPageRows(const ui::Theme& theme);

    // 面板是覆盖层：底下的世界照画，玩家才知道自己是在哪家店里。
    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "Shop"; }

    [[nodiscard]] const std::string& shopId() const { return shopId_; }
    [[nodiscard]] const std::string& feedback() const { return feedback_; }

    // ---- 纯逻辑，公开供无头测试直接驱动 ----
    // （与灵田面板同一个理由：藏在实现文件里的规则没人测得到。）

    // 背包里可卖的一堆货。**同 itemId 且同年份**才算一堆——这正是分堆的定义处。
    struct SellStack {
        std::string itemId;
        int herbAge = 0;
        int count = 0;
        int unitPrice = 0;   // rules::sellPrice 算出的单价，面板与结算共用这一个数
    };

    // 货架。末项固定是「返回」，因此条目数恒为货架条目数 + 1。
    // 买不起与已售罄的条目照样列出，只是变灰并写明原因：直接不显示，玩家会
    // 以为这家店根本没有这味药。
    [[nodiscard]] static std::vector<ui::ListItem> buildBuyItems(const core::GameData& data,
                                                                 const core::GameState& state,
                                                                 const rules::Shop& shop);

    // 背包里能卖的东西，按 (itemId, 年份) 分堆并定序（itemId 升序，同 id 内年份
    // 升序）。定序不是装饰：GameState::removeItem 会把整个背包按年份重排，
    // 列表顺序若跟着背包走，买一次东西就可能让卖出列表的行序跳动。
    [[nodiscard]] static std::vector<SellStack> buildSellStacks(const core::GameData& data,
                                                                const core::GameState& state,
                                                                const rules::Shop& shop);
    // 末项固定是「返回」，因此条目数恒为堆数 + 1。
    [[nodiscard]] static std::vector<ui::ListItem> buildSellItems(
        PanelStage stage, const core::GameData& data,
                                                                  const std::vector<SellStack>& stacks);

    // 两个动作。全部返回「是否真的成交了」，失败时 feedback() 写明原因——
    // 买不起、卖空货、库存为零都必须有话说，静默无反应一律会被当成 bug。
    bool buyAt(Application& app, int entryIndex);
    bool sellAt(Application& app, int stackIndex);

    // 关店。玩家选「离开」或按取消键走的就是这一条；无头测试也直接调它，
    // 否则「关店之后脚本有没有被叫醒」这件事没人验得到——而它一旦没接上，
    // 表现就是剧情从此卡在药商门口。
    void leave(Application& app);

private:
    enum class Mode { Main, Buy, Sell };

    void enterMain(Application& app);
    void enterBuy(Application& app);
    void enterSell(Application& app);
    bool updateMain(Application& app);
    bool updateBuy(Application& app);
    bool updateSell(Application& app);
    void renderStatus(Application& app, const engine::Rect& area) const;

    std::string shopId_;
    Mode mode_ = Mode::Main;
    bool shopFound_ = false;
    std::vector<SellStack> sellStacks_;   // 与卖出列表同序，供确认时回查
    ui::ListView main_;
    ui::ListView buy_;
    ui::ListView sell_;
    std::string feedback_;
    bool done_ = false;
};

}  // namespace fanren::game
