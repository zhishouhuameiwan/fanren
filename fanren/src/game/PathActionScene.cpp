#include "game/PathActionScene.h"

#include <utility>

#include "core/rules/PathActions.h"
#include "game/Application.h"
#include "game/ShopScene.h"
#include "game/Wording.h"

namespace fanren::game {
namespace {

// 摆位：面板横向居中、底边贴着屏幕下沿，落在平时对话框的那一带——镜头跟着韩立，
// 人与他面前那个 NPC 都在屏幕当中，菜单摆在中间就把问话的两个人盖住了。
constexpr int kPanelW = 520;
constexpr int kPanelBottomGap = 48;
// 「买 / 不买」上面那一截：价码一行、盘缠一行、一道分隔线。
constexpr int kTermsH = 86;
constexpr int kTermsFont = 22;
constexpr int kPurseFont = 18;
constexpr int kKeysFont = 16;

// 「买 / 不买」两行里「买」在上面。
constexpr int kBuyRow = 0;

// 音效 id（docs/audio.md 第 4 节），与主菜单同一套。
constexpr const char* kSfxCursor = "ui_cursor";
constexpr const char* kSfxConfirm = "ui_confirm";
constexpr const char* kSfxCancel = "ui_cancel";

// 三种行动名的 key。**不要加 default:**：将来多一种行动时让编译器告警，而不是悄悄画成空行。
[[nodiscard]] const char* kindKey(core::PathActionKind kind) {
    switch (kind) {
        case core::PathActionKind::Inquire: return "ui.path.inquire";
        case core::PathActionKind::Purchase: return "ui.path.purchase";
        case core::PathActionKind::Challenge: return "ui.path.challenge";
    }
    return "";
}

// 面板的矩形：高度 = 标题带 + 列表上面另占的一截 + 列表本身（listAreaHeight 已含上下内边距）。
[[nodiscard]] engine::Rect panelArea(bool hasTitle, int rows, int extraH, const ui::Theme& theme) {
    const int h =
        ui::panelTitleBandHeight(hasTitle, theme) + extraH + ui::listAreaHeight(rows, theme);
    return engine::Rect{(engine::kLogicalWidth - kPanelW) / 2,
                        engine::kLogicalHeight - kPanelBottomGap - h, kPanelW, h};
}

}  // namespace

PathBubble pathBubbleFor(const std::vector<const core::PathAction*>& actions) {
    if (actions.empty()) return PathBubble::None;
    // pathActionsAt 已按行动排好：头尾同一种，就只有这一种。
    if (actions.front()->kind != actions.back()->kind) return PathBubble::Generic;
    switch (actions.front()->kind) {
        case core::PathActionKind::Inquire: return PathBubble::Inquire;
        case core::PathActionKind::Purchase: return PathBubble::Purchase;
        case core::PathActionKind::Challenge: return PathBubble::Challenge;
    }
    return PathBubble::Generic;
}

PathActionScene::PathActionScene(std::string npcName, std::string speakerRole)
    : npc_(std::move(npcName)), role_(std::move(speakerRole)) {}

std::string PathActionScene::purchaseTerms(const core::GameData& data,
                                           const core::PathAction& action) {
    const core::Item* item = data.findItem(action.goods.itemId);
    std::string terms = item != nullptr ? item->name : action.goods.itemId;
    // 灵草的年份写法与主菜单物品页同一个（MenuScene::itemRows）。
    if (action.goods.herbAge > 0) terms += "（" + std::to_string(action.goods.herbAge) + " 年）";
    // 按「块」计，与 Wording::currencyAmount 同一个单位；钱的名字不写——价码这一行说的是
    // 「几块」，身上有多少另起一行按阶段说（碎银 / 灵石）。
    return terms + " × " + std::to_string(action.goods.count) + " · " + std::to_string(action.price) +
           " 块";
}

ui::ListItem PathActionScene::rowFor(const core::GameData& data, const core::PathAction& action) {
    ui::ListItem row;
    row.label = data.lookupText(kindKey(action.kind));
    if (action.kind == core::PathActionKind::Purchase) row.detail = purchaseTerms(data, action);
    return row;
}

std::vector<std::string> PathActionScene::fixedStrings(const core::GameData& data) {
    std::vector<std::string> out;
    using Kind = core::PathActionKind;
    for (const Kind kind : {Kind::Inquire, Kind::Purchase, Kind::Challenge}) {
        out.push_back(data.lookupText(kindKey(kind)));
    }
    for (const char* key : {"ui.path.buy", "ui.path.decline", "ui.path.keys", "ui.menu.money"}) {
        out.push_back(data.lookupText(key));
    }
    return out;
}

void PathActionScene::onEnter(Application& app) {
    rows_ = app.pathActionsFor(npc_);
    std::vector<ui::ListItem> items;
    items.reserve(rows_.size());
    for (const core::PathAction* action : rows_) items.push_back(rowFor(app.data(), *action));
    menu_.reset();
    menu_.setPageSize(static_cast<int>(items.size()));
    menu_.setItems(std::move(items));

    ui::ListItem buy;
    buy.label = app.text("ui.path.buy");
    ui::ListItem decline;
    decline.label = app.text("ui.path.decline");
    confirm_.reset();
    confirm_.setPageSize(2);
    confirm_.setItems({std::move(buy), std::move(decline)});
    phase_ = Phase::Menu;
}

void PathActionScene::choose(Application& app, int row) {
    const core::PathAction& action = *rows_[static_cast<std::size_t>(row)];
    const rules::PathVerdict verdict = rules::pathActionVerdict(action, app.state());
    // 阅历不足：他连价都不开，只说那一句回绝。钱够不够要等开了价、说「买」时才问（契约 5.3）。
    if (verdict.block == rules::PathBlock::RealmTooLow) {
        app.sayAs(role_, verdict.reasonKey);
        phase_ = Phase::Done;
        return;
    }
    switch (action.kind) {
        case core::PathActionKind::Purchase:
            chosen_ = &action;
            app.sayAs(role_, action.textKey);
            phase_ = Phase::Offer;
            return;
        case core::PathActionKind::Inquire:
        case core::PathActionKind::Challenge:
            // 切磋：邀战那一句压在那一仗上面先演，仗打完 Application 按战果再施加胜 / 负那一套
            //（以回调开战，见 Application::startBattle）。这一层照旧是 Done：胜负那句话说完，
            // 它露出来就收起。
            app.applyPathEffects(action, rules::pathActionEffects(action, app.state()), role_);
            phase_ = Phase::Done;
            return;
    }
}

void PathActionScene::answer(Application& app, bool buy) {
    phase_ = Phase::Done;
    if (!buy) return;
    // 契约 5.3：说「买」时再取一次判定。开价那一刻与此刻之间钱不会变，但价码认的是成交这一刻。
    const rules::PathVerdict verdict = rules::pathActionVerdict(*chosen_, app.state());
    if (!verdict.allowed()) {
        app.sayAs(role_, verdict.reasonKey);
        return;
    }
    app.applyPathEffects(*chosen_, rules::pathActionEffects(*chosen_, app.state()), role_);
}

bool PathActionScene::update(Application& app, double) {
    // 只有栈顶会被 update：走到这里，压在上面的对话框已经收了。
    if (phase_ == Phase::Done) return false;
    if (phase_ == Phase::Offer) {
        // 开价那一句刚说完：这一帧只换上「买 / 不买」，不吃按键——收对话框的那一下确认
        // 若还按着，连发会替玩家把「买」按下去。
        phase_ = Phase::Confirm;
        return true;
    }

    engine::Engine& e = app.engine();
    ui::ListView& list = phase_ == Phase::Menu ? menu_ : confirm_;
    const int before = list.selection();
    const bool confirmed = list.update(e);
    if (list.selection() != before) e.playSfx(kSfxCursor);
    if (list.cancelled()) {
        // 菜单里作罢是收起；「买 / 不买」里作罢就是不买，同样收起，不另说一句。
        e.playSfx(kSfxCancel);
        return false;
    }
    if (!confirmed) return true;
    e.playSfx(kSfxConfirm);
    if (phase_ == Phase::Menu) {
        choose(app, list.selection());
    } else {
        answer(app, list.selection() == kBuyRow);
    }
    return true;
}

// ---------------------------------------------------------------------------
// 画
// ---------------------------------------------------------------------------

void PathActionScene::render(Application& app) {
    // Offer / Done：他正在说话，对话框压在上面。这一层什么也不画——菜单留在底下只会与
    // 对话框叠成两层字，而问完了的那一问也不该还挂在屏幕上。
    if (phase_ == Phase::Menu) drawMenu(app);
    if (phase_ == Phase::Confirm) drawConfirm(app);
}

std::string PathActionScene::title(Application& app) const {
    return role_.empty() ? std::string{} : app.speakerName(role_);
}

void PathActionScene::drawMenu(Application& app) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const std::string name = title(app);
    // 镜头贴着地图下沿、人站在屏幕下半时，贴底的面板会盖住他与跟前的 NPC（终审 LOW-6）：那时整体上移让开。
    const engine::Rect panel =
        keepHeroInSight(panelArea(!name.empty(), menu_.count(), 0, theme), app.heroScreenRect(), kPanelBottomGap);
    ui::drawScrim(e, theme);
    ui::drawPanel(e, panel, name, theme);
    menu_.render(e, ui::panelContentArea(panel, name, theme), theme);
    drawKeys(app, panel);
}

void PathActionScene::drawConfirm(Application& app) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const core::GameState& state = app.state();
    const std::string name = title(app);
    const engine::Rect panel = keepHeroInSight(panelArea(!name.empty(), confirm_.count(), kTermsH, theme),
                                               app.heroScreenRect(), kPanelBottomGap);
    ui::drawScrim(e, theme);
    ui::drawPanel(e, panel, name, theme);

    // 价码在上、身上的钱在下：「买不买得起」一眼对得上，不必去翻主菜单。
    const engine::Rect content = ui::panelContentArea(panel, name, theme);
    const int x = content.x + theme.padding;
    e.drawText(purchaseTerms(app.data(), *chosen_), x, content.y + 12, kTermsFont, theme.goldBright,
               theme.bodyStyle);
    const std::string purse =
        app.text("ui.menu.money") + "　" + currencyAmount(state, spiritStones(state));
    e.drawText(purse, x, content.y + 12 + kTermsFont + 14, kPurseFont, theme.paperDim,
               theme.bodyStyle);
    ui::drawRuleH(e, static_cast<float>(x), static_cast<float>(content.y + kTermsH - 4),
                  static_cast<float>(content.w - 2 * theme.padding), theme);
    confirm_.render(e, engine::Rect{content.x, content.y + kTermsH, content.w, content.h - kTermsH},
                    theme);
    drawKeys(app, panel);
}

void PathActionScene::drawKeys(Application& app, const engine::Rect& panel) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const std::string keys = app.text("ui.path.keys");
    const engine::Point size = e.measureText(keys, kKeysFont);
    e.drawText(keys, panel.x + panel.w - size.x, panel.y + panel.h + 12, kKeysFont, theme.paperDim,
               theme.bodyStyle);
}

}  // namespace fanren::game
