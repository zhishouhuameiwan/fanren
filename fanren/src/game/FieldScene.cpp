#include "game/FieldScene.h"

#include <algorithm>
#include <utility>

#include "core/rules/Bottle.h"
#include "game/Application.h"

namespace fanren::game {
namespace {

constexpr int kPanelX = 160;
constexpr int kPanelY = 60;
constexpr int kPanelW = 960;
constexpr int kPanelH = 600;

constexpr int kStatusPercent = 45;

// 占用槽位的三个动作，下标写死在 update 里。
constexpr int kActionMature = 0;
constexpr int kActionHarvest = 1;
constexpr int kActionBack = 2;

// 灵草的显示名。查不到就回显 id：画面上一眼看出是哪味药没进 data。
[[nodiscard]] std::string herbName(const core::GameData& data, const std::string& itemId) {
    if (const core::Item* item = data.findItem(itemId)) return item->name;
    return itemId;
}

// 这一味药自己的年份上限。
//
// 传全局的 kMaxHerbAge 是不行的：绿液把年份翻倍、售价又是平方曲线，全局上限
// 等于没有上限，十一滴就能把一株一阶药推到一万年、卖价八十余万倍，于是最优
// 解永远是「把每一滴都浇在同一株上」，灵田种什么、什么时候收全失去意义。
//
// data 里查不到这味药（种子 id 写错、或数据没跟上）时退到最低那一档，而不是
// 退到全局上限：一个拼错的 id 不该换来一株能刷到万年的药。
[[nodiscard]] int herbMaxAge(const core::GameData& data, const std::string& itemId) {
    if (const core::Item* item = data.findItem(itemId); item != nullptr && item->maxAge > 0) {
        return item->maxAge;
    }
    return rules::herbMaxAgeForGrade(0);
}

}  // namespace

FieldScene::FieldScene(std::string fieldId) : fieldId_(std::move(fieldId)) {}

rules::SpiritField* FieldScene::field(Application& app) const {
    return app.state().findField(fieldId_);
}

std::vector<ui::ListItem> FieldScene::buildSlotItems(const core::GameData& data,
                                                     const rules::SpiritField& field) {
    std::vector<ui::ListItem> items;
    items.reserve(field.slots.size() + 1);

    for (std::size_t i = 0; i < field.slots.size(); ++i) {
        const rules::FieldSlot& slot = field.slots[i];
        ui::ListItem item;
        item.label = "第 " + std::to_string(i + 1) + " 畦　";
        if (slot.seedId.empty()) {
            // 空槽写「空地」而不是留白：留白只会让玩家以为这一行没画出来。
            item.label += "空地";
            item.detail = "可下种";
        } else {
            item.label += herbName(data, slot.seedId);
            item.detail = std::to_string(slot.age) + " 年 · " + (slot.ripe ? "已熟" : "未熟");
        }
        items.push_back(std::move(item));
    }

    ui::ListItem leave;
    leave.label = "离开";
    items.push_back(std::move(leave));
    return items;
}

std::vector<ui::ListItem> FieldScene::buildSeedItems(const core::GameData& data,
                                                     const core::GameState& state,
                                                     std::vector<std::string>& seedIds) {
    seedIds.clear();
    std::vector<ui::ListItem> items;

    // 同一味药按年份在背包里分堆，但下种之后年份一律归零，分堆列出来只会
    // 让玩家在几行一模一样的选项之间猜。按 itemId 合并计数。
    for (const core::BagEntry& entry : state.bag) {
        const core::Item* item = data.findItem(entry.itemId);
        if (item == nullptr || item->kind != core::ItemKind::Herb) continue;

        // itemCount 已经把各年份的堆加在一起了，重复的 itemId 直接跳过。
        if (std::find(seedIds.begin(), seedIds.end(), entry.itemId) != seedIds.end()) continue;

        seedIds.push_back(entry.itemId);
        ui::ListItem row;
        row.label = item->name;
        row.detail = "共 " + std::to_string(state.itemCount(entry.itemId)) + " 株";
        items.push_back(std::move(row));
    }

    ui::ListItem back;
    back.label = "返回";
    items.push_back(std::move(back));
    return items;
}

bool FieldScene::plantAt(Application& app, int slotIndex, const std::string& seedId) {
    rules::SpiritField* target = field(app);
    if (target == nullptr) {
        feedback_ = "此处并无灵田。";
        return false;
    }
    // 越界与「已占用」是两回事，理由要分开说：rules::plant 两种情形都只回
    // false，笼统地报「已有灵草」会把地图或脚本的下标错误伪装成玩家操作失误。
    if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= target->slots.size()) {
        feedback_ = "并无此畦。";
        return false;
    }
    core::GameState& state = app.state();
    if (state.itemCount(seedId) <= 0) {
        feedback_ = "囊中并无此种。";
        return false;
    }
    // 先问规则层能不能种，成了再扣种子：反过来会在重种时白吞一株。
    // 年份上限跟着种子一起进槽位，自然生长才知道该在哪停住。
    if (!rules::plant(*target, slotIndex, seedId, state.day, herbMaxAge(app.data(), seedId))) {
        feedback_ = "此畦已有灵草，不可覆种。";
        return false;
    }
    // 上面刚查过存货，这一步不会失败；[[nodiscard]] 仍要显式吃掉，
    // 免得日后有人把前面那道检查删了还一路静默。
    static_cast<void>(state.removeItem(seedId, 1));
    feedback_ = "将 " + herbName(app.data(), seedId) + " 埋入第 " + std::to_string(slotIndex + 1) +
                " 畦，只待时日。";
    return true;
}

bool FieldScene::matureAt(Application& app, int slotIndex) {
    rules::SpiritField* target = field(app);
    if (target == nullptr) {
        feedback_ = "此处并无灵田。";
        return false;
    }
    if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= target->slots.size()) {
        feedback_ = "并无此畦。";
        return false;
    }
    rules::FieldSlot& slot = target->slots[static_cast<std::size_t>(slotIndex)];
    if (slot.seedId.empty()) {
        feedback_ = "此畦空空如也，无物可催。";
        return false;
    }

    // 先把要用的值取出来，再去动 GameState：slot 是指进 state.fields 的引用，
    // 跨过一次 GameState 的改动之后它是否还有效，取决于那边的实现细节。
    // 眼下 matureHerb 只碰 bottle，但这种「今天恰好安全」的依赖不该留给将来。
    const std::string seedId = slot.seedId;
    const int before = slot.age;

    // 上限按这一株的种取，不是全局的 kMaxHerbAge：浇到这一味药自己的顶就
    // 该换一株浇，决策于是从「全浇一株」回到「浇哪一株」。
    const rules::MatureResult result =
        rules::matureHerb(app.state().bottle, before, herbMaxAge(app.data(), seedId));
    if (!result.ok) {
        // 原样转述规则层给的理由。笼统地说「不能催熟」，玩家分不清是缺瓶子、
        // 缺见识还是缺绿液，而这三件事要靠完全不同的方式去解决。
        feedback_ = result.reason;
        return false;
    }

    rules::FieldSlot& fresh = field(app)->slots[static_cast<std::size_t>(slotIndex)];
    fresh.age = result.newAge;
    if (!fresh.ripe && fresh.age >= rules::kRipeAge) fresh.ripe = true;
    feedback_ = "一滴绿液落下，" + herbName(app.data(), seedId) + " 自 " +
                std::to_string(before) + " 年拔至 " + std::to_string(fresh.age) + " 年。";
    return true;
}

bool FieldScene::harvestAt(Application& app, int slotIndex) {
    rules::SpiritField* target = field(app);
    if (target == nullptr) {
        feedback_ = "此处并无灵田。";
        return false;
    }
    if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= target->slots.size()) {
        feedback_ = "并无此畦。";
        return false;
    }
    rules::FieldSlot& slot = target->slots[static_cast<std::size_t>(slotIndex)];
    if (slot.seedId.empty()) {
        feedback_ = "此畦空空如也。";
        return false;
    }
    if (!slot.ripe) {
        feedback_ = "尚未足年，此时采下便废了。";
        return false;
    }

    const std::string seedId = slot.seedId;
    const int age = slot.age;

    // 先腾空槽位再进背包，指向 state.fields 的引用就不必跨过一次 GameState
    // 的改动而存活。顺序反过来今天也能跑，但那是碰巧，不是保证。
    slot = rules::FieldSlot{};
    slot.plantedDay = app.state().day;

    // 年份必须跟着进背包：不带年份，催熟攒下的价值在收割这一步就没了，
    // 掌天瓶那条经济循环也就断在这里。
    app.state().addItem(seedId, 1, age);

    feedback_ = "采得 " + std::to_string(age) + " 年 " + herbName(app.data(), seedId) + " 一株。";
    return true;
}

void FieldScene::enterSlots(Application& app) {
    mode_ = Mode::Slots;
    slots_.reset();
    slots_.setPageSize(listPageRows(app.theme()));
    if (const rules::SpiritField* target = field(app)) {
        slots_.setItems(buildSlotItems(app.data(), *target));
        return;
    }
    // 灵田不在存档里（地图写错了 ref_id，或存档是旧版本）。列表仍要留一条
    // 「离开」：一个只能靠取消键退出的面板，玩家第一反应是游戏卡死了。
    ui::ListItem leave;
    leave.label = "离开";
    slots_.setItems({std::move(leave)});
}

void FieldScene::enterSeeds(Application& app) {
    mode_ = Mode::Seeds;
    seeds_.reset();
    seeds_.setPageSize(listPageRows(app.theme()));
    seeds_.setItems(buildSeedItems(app.data(), app.state(), seedIds_));
}

std::vector<ui::ListItem> FieldScene::buildSlotActionItems(const rules::Bottle& bottle,
                                                           const rules::FieldSlot& slot,
                                                           int maxAge) {
    std::vector<ui::ListItem> items;
    items.reserve(3);

    ui::ListItem mature;
    mature.label = "催熟";

    // 空转一次 matureHerb 取判据与理由。probe 是瓶子的副本，成功那一路扣掉的
    // 那一滴只落在副本上，真瓶子分毫不动——这样「能不能点」与「点下去会不会
    // 成」永远是同一个判断，不会有第二套。
    rules::Bottle probe = bottle;
    const rules::MatureResult dry = rules::matureHerb(probe, slot.age, maxAge);
    if (dry.ok) {
        mature.detail = "耗绿液一滴";
    } else {
        // 「绿瓶四年」的那四年：瓶子在手、绿液也在瓶里，他只是不知道这一滴
        // 能做什么。解锁前照旧挂着「耗绿液一滴」这句引导、照旧可点，玩家只会
        // 反复去点再反复被回绝——设计文档第 3 节点名过这个观感问题，而拾瓶与
        // 发现催熟相隔四年（硬约束 #1），这四年里它必然发生。
        //
        // 置灰而不是删行：三个动作的下标写死在 update 里，删一条要改三处。
        // 理由原样转述规则层给的那一句，站在少年这一侧说——他不知道的是
        // 「这滴绿的有什么用」，不是「功能未解锁」。
        mature.enabled = false;
        mature.disabledReason = dry.reason;
    }
    items.push_back(std::move(mature));

    ui::ListItem harvest;
    harvest.label = "采收";
    if (!slot.ripe) {
        harvest.enabled = false;
        harvest.disabledReason = "尚未足年";
    }
    items.push_back(std::move(harvest));

    ui::ListItem back;
    back.label = "返回";
    items.push_back(std::move(back));
    return items;
}

void FieldScene::enterSlotAction(Application& app) {
    mode_ = Mode::SlotAction;

    rules::FieldSlot slot;
    if (const rules::SpiritField* target = field(app)) {
        if (slot_ >= 0 && static_cast<std::size_t>(slot_) < target->slots.size()) {
            slot = target->slots[static_cast<std::size_t>(slot_)];
        }
    }
    // 上限走与 matureAt 同一个 helper，免得列表算一套、真催熟时算另一套。
    const int maxAge = herbMaxAge(app.data(), slot.seedId);

    std::vector<ui::ListItem> items = buildSlotActionItems(app.state().bottle, slot, maxAge);
    actions_.reset();
    actions_.setPageSize(listPageRows(app.theme()));
    actions_.setItems(std::move(items));
}

void FieldScene::syncSlotCaps(Application& app) {
    rules::SpiritField* target = field(app);
    if (target == nullptr) return;

    // 槽位里那份上限是 data 的副本，这里按 seedId 重新对一遍。两种情形需要它：
    // 一是这次修复之前存下的老档（槽位里根本没有上限，自然生长会一路长到全局
    // 上限）；二是日后调平衡改了 data 里的 maxAge，在田里的那几株也该跟着改。
    //
    // 只改上限，不动 age：把玩家账面上已经拿到的年份改小，看起来就是掉档。
    // 超出新上限的那几株保持原样，只是从此不再长、也催不动。
    for (rules::FieldSlot& slot : target->slots) {
        if (slot.seedId.empty()) continue;
        slot.maxAge = herbMaxAge(app.data(), slot.seedId);
    }
}

void FieldScene::onEnter(Application& app) {
    syncSlotCaps(app);
    if (field(app) == nullptr) {
        feedback_ = "此处并无灵田。";
    } else {
        feedback_ = "择一畦查看：空地可下种，已种的可催熟或采收。";
    }
    enterSlots(app);
}

// 三个分模式的处理各自成函数：一个 update 里塞三套菜单，加第四套时必然
// 有人把某一支的 enterSlots 忘掉，而那种漏法表现为「面板卡在子菜单里」。
bool FieldScene::updateSeeds(Application& app) {
    if (seeds_.update(app.engine())) {
        const int index = seeds_.selection();
        if (index >= 0 && static_cast<std::size_t>(index) < seedIds_.size()) {
            plantAt(app, slot_, seedIds_[static_cast<std::size_t>(index)]);
        }
        // 末项是「返回」，落在 seedIds_ 之外，于是什么也不种就退回槽位表。
        enterSlots(app);
        return true;
    }
    if (seeds_.cancelled()) enterSlots(app);
    return true;
}

bool FieldScene::updateSlotAction(Application& app) {
    if (actions_.update(app.engine())) {
        switch (actions_.selection()) {
            case kActionMature:
                matureAt(app, slot_);
                enterSlotAction(app);   // 催熟后可能刚好足年，采收要跟着解禁
                return true;
            case kActionHarvest:
                harvestAt(app, slot_);
                enterSlots(app);
                return true;
            case kActionBack:
            default:
                enterSlots(app);
                return true;
        }
    }
    if (actions_.cancelled()) enterSlots(app);
    return true;
}

bool FieldScene::updateSlots(Application& app) {
    if (slots_.update(app.engine())) {
        const rules::SpiritField* target = field(app);
        const int index = slots_.selection();
        if (target == nullptr || index < 0 ||
            static_cast<std::size_t>(index) >= target->slots.size()) {
            return false;   // 末项「离开」，以及没有灵田时的任何确认
        }
        slot_ = index;
        if (target->slots[static_cast<std::size_t>(index)].seedId.empty()) {
            enterSeeds(app);
        } else {
            enterSlotAction(app);
        }
        return true;
    }
    if (slots_.cancelled()) return false;
    return true;
}

bool FieldScene::update(Application& app, double) {
    switch (mode_) {
        case Mode::Seeds:      return updateSeeds(app);
        case Mode::SlotAction: return updateSlotAction(app);
        case Mode::Slots:
        default:               return updateSlots(app);
    }
}

void FieldScene::renderStatus(Application& app, const engine::Rect& area) const {
    engine::Engine& engine = app.engine();
    const ui::Theme& theme = app.theme();
    const core::GameState& state = app.state();
    if (area.w <= 0 || area.h <= 0) return;

    const int row = theme.bodyFontSize + theme.lineSpacing;
    int y = area.y;
    const auto line = [&](const std::string& textLine) {
        engine.drawText(textLine, area.x, y, theme.bodyFontSize, theme.paper, theme.bodyStyle);
        y += row;
    };

    line("日期　第 " + std::to_string(state.day) + " 日");
    // 绿液存量必须摆在面上：看不见还剩几滴，玩家无从判断这一次催熟值不值。
    if (!state.bottle.owned) {
        line("绿液　尚无小瓶");
    } else {
        const std::string hint = state.bottle.matureKnown ? "" : "（尚不知其用）";
        line("绿液　" + std::to_string(state.bottle.drops) + " / " +
             std::to_string(state.bottle.capacity) + hint);
    }

    const engine::Rect note{area.x, y, area.w, std::max(0, area.y + area.h - y)};
    ui::drawTextBlock(engine, feedback_, note, theme.bodyFontSize, theme);
}

void FieldScene::render(Application& app) {
    engine::Engine& engine = app.engine();
    const ui::Theme& theme = app.theme();

    const engine::Rect panel{kPanelX, kPanelY, kPanelW, kPanelH};
    const std::string title = "灵田";
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
        case Mode::Seeds:
            seeds_.render(engine, listArea, theme);
            break;
        case Mode::SlotAction:
            actions_.render(engine, listArea, theme);
            break;
        case Mode::Slots:
        default:
            slots_.render(engine, listArea, theme);
            break;
    }
}

int FieldScene::listPageRows(const ui::Theme& theme) {
    // 标题带按「有标题」算：本面板的标题从来不为空，render() 也是这么摆的。
    return ui::listRowsThatFit(kPanelH - ui::panelTitleBandHeight(true, theme), theme);
}

}  // namespace fanren::game
