#pragma once
// 灵田面板：播种、催熟、收割。
//
// 与修炼面板同一条分层纪律：年份怎么长、绿液够不够、催熟跳多少年，全在
// core/rules/Field.h 与 core/rules/Bottle.h 里；这里只负责挑槽位、挑种子，
// 以及把失败原因原样说给玩家听。
//
// 「原样」是要紧的：matureHerb 的五条失败路径（没瓶子 / 不会用 / 没绿液 /
// 尚未足年 / 已达上限）对应原著「绿瓶四年」里依次撞上的墙，合并成一句
// 「不能催熟」会把那段解锁过程整个抹平。
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Bottle.h"
#include "core/rules/Field.h"
#include "game/Scene.h"
#include "ui/Widgets.h"

namespace fanren::game {

class FieldScene : public Scene {
public:
    explicit FieldScene(std::string fieldId);

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
    [[nodiscard]] std::string name() const override { return "Field"; }

    [[nodiscard]] const std::string& fieldId() const { return fieldId_; }
    [[nodiscard]] const std::string& feedback() const { return feedback_; }

    // ---- 纯逻辑，公开供无头测试直接驱动 ----

    // 槽位列表。末项固定是「离开」，因此条目数恒为槽位数 + 1。
    [[nodiscard]] static std::vector<ui::ListItem> buildSlotItems(const core::GameData& data,
                                                                  const rules::SpiritField& field);
    // 可下种的背包条目（kind == Herb）。返回的下标与 seedIds 一一对应。
    [[nodiscard]] static std::vector<ui::ListItem> buildSeedItems(const core::GameData& data,
                                                                  const core::GameState& state,
                                                                  std::vector<std::string>& seedIds);

    // 已占用槽位上的三个动作：催熟 / 采收 / 返回。条目数恒为 3——三个下标
    // 写死在 update 里，「这一项现在不能用」一律靠置灰表达，不靠删行。
    //
    // 催熟能不能点，判据直接问 rules::matureHerb，不在这里另写一套：另写的
    // 那套迟早与规则层分家，而分家的样子正是「面板说能点，点下去被回绝」。
    [[nodiscard]] static std::vector<ui::ListItem> buildSlotActionItems(
        const rules::Bottle& bottle, const rules::FieldSlot& slot, int maxAge);

    // 三个动作。全部返回「是否真的做成了」，失败时 feedback() 写明原因。
    bool plantAt(Application& app, int slotIndex, const std::string& seedId);
    bool matureAt(Application& app, int slotIndex);
    bool harvestAt(Application& app, int slotIndex);

private:
    // 三级菜单：选槽位 → 空槽选种子 / 占用槽选动作。
    enum class Mode { Slots, Seeds, SlotAction };

    [[nodiscard]] rules::SpiritField* field(Application& app) const;
    // 把槽位里缓存的年份上限按 data 重新对一遍（老存档补齐、调平衡后跟进）。
    void syncSlotCaps(Application& app);
    bool updateSlots(Application& app);
    bool updateSeeds(Application& app);
    bool updateSlotAction(Application& app);
    void enterSlots(Application& app);
    void enterSeeds(Application& app);
    void enterSlotAction(Application& app);
    void renderStatus(Application& app, const engine::Rect& area) const;

    std::string fieldId_;
    Mode mode_ = Mode::Slots;
    int slot_ = 0;                       // 当前操作的槽位下标
    std::vector<std::string> seedIds_;   // 与种子列表同序，供确认时回查
    ui::ListView slots_;
    ui::ListView seeds_;
    ui::ListView actions_;
    std::string feedback_;
};

}  // namespace fanren::game
