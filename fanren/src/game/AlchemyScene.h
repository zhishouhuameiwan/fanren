#pragma once
// 炼制面板：炼丹 / 制符 / 炼器 / 布阵共用这一块（第 4 章只有炼丹真的接了内容）。
//
// 与灵田、商店、修炼三块面板同一条分层纪律：成功率怎么算、失败扣不扣料、
// 熟练度涨多少，**全在 core/rules/Crafting.h 里，这里一个数也不重算**。
// 本场景只做三件事：把配方翻成列表、把玩家的选择翻成对 rules 层的调用、
// 把 rules 层给的那句话翻回玩家看得懂的说法。
//
// 「翻回人话」只有一处：`rules::canCraft` 的拒绝理由里嵌的是**物品 id**
// （「材料不足：herb_qingfeng_cao 需 2，现有 0」），因为规则层不认识物品册。
// humanizeReason 把那几个 id 换成显示名，**除此之外一个字不改**——
// 判据仍然只有 canCraft 这一个，换掉的只是名字。
//
// 面板的硬口径（与灵田、商店一致，本项目在这三处已经写死）：
//   · **配不出来的方子照样列出来**，置灰并写明缺什么。直接不显示，玩家会以为
//     这门手艺根本没有这张方子，掉头去别处找。这一条管的是**到手了的**方子；
//     还没到手的（requireFlag 未置）一概不列，连名字都不露（visibleRecipes）。
//   · **静默失败是明令禁止的。** 成功、失败、连开工都开不了，三种结果各有各的
//     一句话，且失败那句要说清楚亏了什么（rules::craft 的 log 已经写好）。
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Crafting.h"
#include "game/Scene.h"
#include "ui/Widgets.h"

namespace fanren::game {

// 地图上的炉鼎没写品阶时按几品算。
//
// 取 1 而不是 0：0 在规则层的意思是「没有炉鼎」（rules::hasToolOfGrade），
// 于是一处忘了写 grade 的丹房会变成一间点不动任何东西的屋子，而提示语是
// 「身边没有丹炉，无从起火」——玩家站在丹炉前看着这句话，只会当成 bug。
// 1 是「有一口最普通的炉子」，那正是韩神医那间药房该有的样子。
inline constexpr int kDefaultCraftToolGrade = 1;

// 这门手艺的熟练度记在 GameState 的哪一位上。
//
// 四个字段散在 GameState 里，没有一处把「技艺 → 字段」这条映射写下来，
// 于是每个想读熟练度的地方都要自己 switch 一遍。集中在这里，加第五艺时
// /W4 会因为漏掉的分支报警。
[[nodiscard]] int craftProficiency(const core::GameState& state, rules::CraftKind kind);
void addCraftProficiency(core::GameState& state, rules::CraftKind kind, int gain);

// 把 rules 层那句拒绝理由里的物品 id 换成显示名。查不到的 id 原样留着
// （画面上一眼看出是哪件东西没进 data，与本项目别处同一条规矩）。
//
// 只替换**这张方子里真的出现过的** id，不做通用的全局扫描：那样才不可能
// 把一句本来就没提到物品的话（「火候未到」「这张方子残缺不全」）改坏。
[[nodiscard]] std::string humanizeReason(const core::GameData& data, const rules::Recipe& recipe,
                                         std::string reason);

class AlchemyScene : public Scene {
public:
    // toolGrade 由地图上的 facility 给（属性 grade，缺省 kDefaultCraftToolGrade）。
    // 0 即「没有炉鼎」，面板照开，只是每一张方子都点不动并写明缺的是炉子——
    // 与商店「此处并无这家店」同一个办法：默不作声与「功用尚未开启」看着一样，
    // 而后者至少能让人去查地图。
    AlchemyScene(rules::CraftKind kind, int toolGrade);

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    // 本面板的列表页大小：面板高度去掉标题带之后画得满几行。
    // enter*() 设页大小与 render() 摆列表区必须走同一份高度，否则会出现
    // 「区域明明画得下十几行，却因为页大小是默认的 8 而只显示 8 行」——
    // 多出来的那几行玩家得按方向键才找得到，而他多半不知道下面还有。
    // 公开是为了让无头测试能直接断言「今天这张表一行都不会被藏起来」。
    [[nodiscard]] static int listPageRows(const ui::Theme& theme);

    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "Alchemy"; }

    [[nodiscard]] rules::CraftKind kind() const { return kind_; }
    [[nodiscard]] int toolGrade() const { return toolGrade_; }
    [[nodiscard]] const std::string& feedback() const { return feedback_; }

    // ---- 纯逻辑，公开供无头测试直接驱动 ----
    // （与灵田、商店、战斗菜单同一个理由：藏在实现文件里的规则没人测得到。）

    // 本门手艺列出来的方子（到手了的那些），按 id 字典序，与列表同源。定序不是装饰：
    // std::map 的遍历序是稳的，而「第二行是回气丹方」是玩家会记住的东西。
    [[nodiscard]] const std::vector<std::string>& recipeIds() const { return recipeIds_; }

    // 配方列表。末项固定是「离开」，因此条目数恒为方子数 + 1。
    // 配不出来的条目置灰，disabledReason 就是 canCraft 那句话（换过物品名）。
    [[nodiscard]] static std::vector<ui::ListItem> buildRecipeItems(
        const core::GameData& data, const core::GameState& state, rules::CraftKind kind,
        int toolGrade, const std::vector<const rules::Recipe*>& recipes);

    // 本门手艺此刻能看见的方子：只列到手了的（rules::recipeKnown，契约 docs/interfaces-p3-ch07.md 1.5）。
    // 指针指进 Application 持有的那张表，只许在同一帧内即取即用。
    // 刻意只有这一个签名：从前那个不收存档的两参版删掉了，留着就会有人绕过门闸。
    [[nodiscard]] static std::vector<const rules::Recipe*> visibleRecipes(
        const std::map<std::string, rules::Recipe>& all, rules::CraftKind kind,
        const core::GameState& state);

    // 开一炉。返回是否**真的开工了**（成了还是炸了都算开工，见下）。
    //
    // 三种结果分得清清楚楚，这是契约第 3.2 节点名要的：
    //   · 开不了工（缺料 / 缺炉 / 火候不够）→ 返回 false，feedback 写明缺什么，
    //     **一个字节都不动**：不扣料、不涨熟练度（「没开工」和「开工失败」是
    //     两回事，只有后者该长经验，这是 rules::craft 的口径，这里照办）。
    //   · 炸炉 → 返回 true，按 rules::failurePolicyOf 扣料，熟练度照涨。
    //   · 成了 → 返回 true，扣料、给产物、熟练度照涨。
    bool craftAt(Application& app, int recipeIndex);

    // 关面板。玩家选「离开」或按取消键走的就是这一条；无头测试也直接调它。
    void leave(Application& app);

private:
    void enterList(Application& app);
    void renderStatus(Application& app, const engine::Rect& area) const;

    // 这一炉的随机种子。
    //
    // 不取当前时间：同种子同输入必得同结果是 rules::craft 明写的性质
    //（「炼制要能随存档回放」），拿时间当种子等于把它扔掉。
    //
    // 但也不能只取配方 id + 日期：那样同一天连炼两炉必得同一个结果，玩家
    // 炸一炉、补齐材料再来一次，还是原样炸一次——而那看着就是「这游戏坏了」。
    // 种子里因此还有**熟练度**与**本次进面板的第几炉**：熟练度每炉都涨
    //（rules::proficiencyGain 成功 3-8、失败 1-3，恒为正），所以连炼两炉的
    // 种子必然不同，这一条不依赖计数器，计数器只是第二道。
    [[nodiscard]] std::uint32_t seedFor(const core::GameState& state,
                                        const rules::Recipe& recipe) const;

    rules::CraftKind kind_;
    int toolGrade_;
    ui::ListView list_;
    // 与列表同序，供确认时回查。存 id 而不是指针：Application 那张表在面板
    // 开着期间不会变，但存 id 就连「万一变了」也不必担心。
    std::vector<std::string> recipeIds_;
    std::string feedback_;
    int attempts_ = 0;
    bool done_ = false;
};

}  // namespace fanren::game
