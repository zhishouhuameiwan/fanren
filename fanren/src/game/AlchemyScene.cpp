#include "game/AlchemyScene.h"

#include <algorithm>
#include <cstddef>
#include <utility>

#include "game/Application.h"

namespace fanren::game {
namespace {

constexpr int kPanelX = 140;
constexpr int kPanelY = 60;
constexpr int kPanelW = 1000;
constexpr int kPanelH = 600;

constexpr int kStatusPercent = 45;

// 物品显示名。查不到就回显 id：画面上一眼看出是哪件东西没进 data
//（与商店、灵田两块面板同一条规矩）。
[[nodiscard]] std::string itemName(const core::GameData& data, const std::string& itemId) {
    if (const core::Item* item = data.findItem(itemId)) return item->name;
    return itemId;
}

// 把 haystack 里所有的 needle 换成 replacement。标准库没有现成的。
void replaceAll(std::string& haystack, const std::string& needle,
                const std::string& replacement) {
    if (needle.empty() || needle == replacement) return;
    std::size_t at = haystack.find(needle);
    while (at != std::string::npos) {
        haystack.replace(at, needle.size(), replacement);
        at = haystack.find(needle, at + replacement.size());
    }
}

}  // namespace

int craftProficiency(const core::GameState& state, rules::CraftKind kind) {
    // 不加 default:，好让日后添第五艺时由 /W4（C4061/C4062）喊出来这里漏了一项。
    switch (kind) {
        case rules::CraftKind::Alchemy:   return state.alchemyProficiency;
        case rules::CraftKind::Talisman:  return state.talismanProficiency;
        case rules::CraftKind::Forge:     return state.forgeProficiency;
        case rules::CraftKind::Formation: return state.formationProficiency;
    }
    return 0;
}

void addCraftProficiency(core::GameState& state, rules::CraftKind kind, int gain) {
    if (gain <= 0) return;
    // 封顶在规则层的量程上。不封的话熟练度会一路涨到成功率早已顶死 95% 之后，
    // 而存档里留下一个 400 —— 之后任何按量程做的判断（配方门槛的校验、
    // 进度条）都要为这个数另写一条兜底。
    const auto bump = [&](int& slot) {
        slot = std::clamp(slot + gain, 0, rules::kMaxProficiency);
    };
    switch (kind) {
        case rules::CraftKind::Alchemy:   bump(state.alchemyProficiency); return;
        case rules::CraftKind::Talisman:  bump(state.talismanProficiency); return;
        case rules::CraftKind::Forge:     bump(state.forgeProficiency); return;
        case rules::CraftKind::Formation: bump(state.formationProficiency); return;
    }
}

std::string humanizeReason(const core::GameData& data, const rules::Recipe& recipe,
                           std::string reason) {
    if (reason.empty()) return reason;

    // 只替换这张方子里真的出现过的 id。按 id 长度降序替换：一个 id 若是另一个
    // 的前缀（data 里眼下没有，但没有任何东西拦着日后出现），先换短的会把长的
    // 切成两截。产出 id 一并换 —— 眼下 canCraft 的话里没有它，
    // 但「这张方子残缺不全」将来若改成点名产出，这里不必再动。
    std::vector<std::string> ids;
    ids.reserve(recipe.inputs.size() + 1);
    for (const rules::Ingredient& need : recipe.inputs) ids.push_back(need.itemId);
    ids.push_back(recipe.productId);
    std::sort(ids.begin(), ids.end(), [](const std::string& a, const std::string& b) {
        return a.size() > b.size();
    });

    for (const std::string& id : ids) {
        const core::Item* item = data.findItem(id);
        // 查不到就原样留着。硬换成一个笼统的「某物」，反而把数据错藏起来了。
        if (item == nullptr || item->name.empty()) continue;
        replaceAll(reason, id, item->name);
    }
    return reason;
}

AlchemyScene::AlchemyScene(rules::CraftKind kind, int toolGrade)
    : kind_(kind), toolGrade_(std::clamp(toolGrade, 0, rules::kMaxToolGrade)) {}

std::vector<const rules::Recipe*> AlchemyScene::visibleRecipes(
    const std::map<std::string, rules::Recipe>& all, rules::CraftKind kind) {
    std::vector<const rules::Recipe*> picked;
    // std::map 按 key 字典序遍历，所以行序是确定的。跟着哈希序走的话，
    // 同一间丹房每次进来行序都不一样，而玩家记住的是「第二行是回气丹方」。
    for (const auto& entry : all) {
        if (entry.second.kind != kind) continue;
        picked.push_back(&entry.second);
    }
    return picked;
}

std::vector<ui::ListItem> AlchemyScene::buildRecipeItems(
    const core::GameData& data, const core::GameState& state, rules::CraftKind kind,
    int toolGrade, const std::vector<const rules::Recipe*>& recipes) {
    std::vector<ui::ListItem> items;
    items.reserve(recipes.size() + 1);

    const int proficiency = craftProficiency(state, kind);
    for (const rules::Recipe* recipe : recipes) {
        ui::ListItem row;
        row.label = recipe->name;
        // 成算与产出摆在行上。看不见成功率，玩家无从判断这一炉值不值得开，
        // 而炼丹失败是**材料尽毁**——那是本项目最肉疼的一种代价。
        row.detail = "成算 " +
                     std::to_string(rules::successChance(*recipe, proficiency, state.aptitude,
                                                         toolGrade)) +
                     "%　得 " + itemName(data, recipe->productId) + " ×" +
                     std::to_string(recipe->productCount);

        // **禁用与否、理由是什么，只问 rules::canCraft 这一个函数。**
        // 面板另写一套判据就是「面板说能点、点下去被回绝」的来处——灵田面板为此
        // 专门空转一次 rules::matureHerb，战斗菜单为此把 refusePlayerAction 抽出来，
        // 这里是同一条纪律。craftAt 走的是同一个 canCraft。
        const core::Result<std::string> gate =
            rules::canCraft(*recipe, state, proficiency, rules::hasToolOfGrade(toolGrade));
        if (!gate.ok) {
            row.enabled = false;
            row.disabledReason = humanizeReason(data, *recipe, gate.error);
        }
        items.push_back(std::move(row));
    }

    ui::ListItem back;
    back.label = "离开";
    items.push_back(std::move(back));
    return items;
}

std::uint32_t AlchemyScene::seedFor(const core::GameState& state,
                                    const rules::Recipe& recipe) const {
    // FNV-1a，与 BattleScene 由战斗 id 派生种子是同一个办法：纯整数、跨平台
    // 结果一致、雪崩性够用。
    std::uint32_t seed = 2166136261u;
    const auto mix = [&seed](const std::string& text) {
        for (const char ch : text) {
            seed = (seed ^ static_cast<std::uint8_t>(ch)) * 16777619u;
        }
    };
    const auto mixInt = [&seed](int value) {
        const auto bits = static_cast<std::uint32_t>(value);
        for (int shift = 0; shift < 32; shift += 8) {
            seed = (seed ^ ((bits >> shift) & 0xFFu)) * 16777619u;
        }
    };
    mix(recipe.id);
    mixInt(state.day);
    // 熟练度每炉都涨（成功 3-8、失败 1-3，恒为正），所以同一天连炼两炉的种子
    // 必然不同——这一条不依赖下面那个计数器，计数器只是第二道。
    mixInt(craftProficiency(state, kind_));
    mixInt(attempts_);
    return seed;
}

bool AlchemyScene::craftAt(Application& app, int recipeIndex) {
    if (recipeIndex < 0 || static_cast<std::size_t>(recipeIndex) >= recipeIds_.size()) {
        feedback_ = "手边并无这张方子。";
        return false;
    }

    // 就地按 id 重查，不信任上一帧建的那份列表：中间可能已经炼过一炉、
    // 或者剧情脚本刚拿走了什么（与商店 sellAt 同一个理由）。
    const auto& all = app.recipes();
    const auto found = all.find(recipeIds_[static_cast<std::size_t>(recipeIndex)]);
    if (found == all.end()) {
        feedback_ = "手边并无这张方子。";
        return false;
    }
    const rules::Recipe& recipe = found->second;

    core::GameState& state = app.state();
    const int proficiency = craftProficiency(state, kind_);

    // 开不了工：一个字节都不动，把理由原样说出来。**静默失败是明令禁止的。**
    const core::Result<std::string> gate =
        rules::canCraft(recipe, state, proficiency, rules::hasToolOfGrade(toolGrade_));
    if (!gate.ok) {
        feedback_ = humanizeReason(app.data(), recipe, gate.error);
        return false;
    }

    ++attempts_;
    const rules::CraftResult result =
        rules::craft(recipe, state, proficiency, state.aptitude, toolGrade_, seedFor(state, recipe));

    // 落账的顺序：先扣料，再给产物，最后涨熟练度。
    //
    // 扣料必须走 rules::applyConsumption 而不是自己写循环：GameState::removeItem
    // 不认年份，它一律从年份最低的堆扣，拿它去扣一张「需百年以上」的方子会把
    // 玩家的十年草扣掉，而百年草原封不动留在包里（与商店卖出、绿液催熟是同一个坑）。
    rules::applyConsumption(state, result.consumed);
    if (result.success && !result.productId.empty() && result.productCount > 0) {
        state.addItem(result.productId, result.productCount, 0);
    }
    addCraftProficiency(state, kind_, result.proficiencyGain);

    // 成功与失败都要有明确反馈（契约第 3.2 节）。rules::craft 的 log 已经写好了
    // 「成了几份」与「亏了什么」那半句，这里只补上熟练度那半句——它是玩家开下一炉
    // 的依据，不写出来的话，「失败也长手艺」这条设定在游戏里根本看不见。
    feedback_ = result.log;
    if (result.proficiencyGain > 0) {
        feedback_ += "（" + std::string(rules::kindName(kind_)) + "熟练度 +" +
                     std::to_string(result.proficiencyGain) + "，现为 " +
                     std::to_string(craftProficiency(state, kind_)) + "）";
    }
    return true;
}

void AlchemyScene::enterList(Application& app) {
    const std::vector<const rules::Recipe*> recipes = visibleRecipes(app.recipes(), kind_);
    recipeIds_.clear();
    recipeIds_.reserve(recipes.size());
    for (const rules::Recipe* recipe : recipes) recipeIds_.push_back(recipe->id);

    list_.reset();
    list_.setPageSize(listPageRows(app.theme()));
    list_.setItems(buildRecipeItems(app.data(), app.state(), kind_, toolGrade_, recipes));
}

void AlchemyScene::onEnter(Application& app) {
    if (!rules::hasToolOfGrade(toolGrade_)) {
        // 没炉子。面板照开，每一张方子都会被 canCraft 挡住并写明缺的是炉子；
        // 这里再在状态栏上说一遍，免得玩家逐条去读禁用理由才明白。
        feedback_ = std::string(rules::toolMissingMessage(kind_));
    } else {
        feedback_ = std::string(rules::kindName(kind_)) + "之事，成败都在火候上。";
    }
    enterList(app);
}

void AlchemyScene::leave(Application& app) {
    if (done_) return;
    done_ = true;
    script::CommandResult result;
    result.ok = true;
    // 玩家从地图设施走进来的那条路上没有挂起的命令，completeCommand 会自己
    // 忽略掉这次回填（与 ShopScene::leave 同一个办法）。
    app.completeCommand(result);
}

bool AlchemyScene::update(Application& app, double) {
    if (done_) return false;

    if (list_.update(app.engine())) {
        const int index = list_.selection();
        if (index < 0 || static_cast<std::size_t>(index) >= recipeIds_.size()) {
            leave(app);   // 末项「离开」
            return false;
        }
        craftAt(app, index);
        // 材料变了、熟练度变了，成算与禁用理由得跟着重算。
        enterList(app);
        return true;
    }
    if (list_.cancelled()) {
        leave(app);
        return false;
    }
    return true;
}

void AlchemyScene::renderStatus(Application& app, const engine::Rect& area) const {
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
    line(std::string(rules::kindName(kind_)) + "熟练度　" +
         std::to_string(craftProficiency(app.state(), kind_)) + " / " +
         std::to_string(rules::kMaxProficiency));
    // 炉鼎品阶直接影响成功率（每品阶 5 点），摆在面上玩家才知道换炉子有用。
    line("炉鼎品阶　" + (rules::hasToolOfGrade(toolGrade_) ? std::to_string(toolGrade_) + " 品"
                                                           : std::string("无")));

    const engine::Rect note{area.x, y, area.w, std::max(0, area.y + area.h - y)};
    ui::drawTextBlock(engine, feedback_, note, theme.bodyFontSize, theme);
}

void AlchemyScene::render(Application& app) {
    engine::Engine& engine = app.engine();
    const ui::Theme& theme = app.theme();

    const std::string title = std::string(rules::kindName(kind_));
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
    list_.render(engine, listArea, theme);
}

int AlchemyScene::listPageRows(const ui::Theme& theme) {
    // 标题带按「有标题」算：本面板的标题从来不为空，render() 也是这么摆的。
    return ui::listRowsThatFit(kPanelH - ui::panelTitleBandHeight(true, theme), theme);
}

}  // namespace fanren::game
