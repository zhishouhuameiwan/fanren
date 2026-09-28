#pragma once
// 路径行动菜单（施工图 docs/octopath-overhaul.md 5.1；契约 docs/interfaces-octo-pathactions.md 5.3）。
//
// 面对一个在场的 NPC 按 E / Q，Application::openPathActions 压这一层：列出他身上此刻挂着的
// 条目（打探 / 求购 / 切磋），选一项照规则层的判定走——
//
//   阅历不足 ──→ 他说那一句回绝（refuse_key），话说完收起；
//   打探 ──────→ 施加效果清单（头一句就是那段情报），话说完收起；
//   求购 ──────→ 他先开价（text_key）→「买 / 不买」→ 买：再判一次钱，
//                钱够施加效果清单（成交那一句打头），钱不够说 poor_key；话说完收起。
//   切磋 ──────→ 施加效果清单：邀战那一句 → 开战（以回调开战）→ 收场按战果说胜 / 负那一句，
//                胜了发条目奖励、记赢过；负了（逃也算）什么也不记，可以再来。话说完收起。
//
// 说话一律经 Application::sayAs（与脚本 talk 同一个口径：说话人是那个 NPC 的 role，进对话回看），
// 改存档一律经 Application::applyPathEffects——这一层只管「列什么、选了哪一项、下一步等什么」，
// 于是「路径行动顺手改了点别的」这件事在这个文件里无处下手。
//
// 界面上任何地方都不出现境界名：阅历不足只有对方那一句回绝（契约 5.3）。
// 音效：移动 ui_cursor、确认 ui_confirm、作罢 ui_cancel；缺文件时引擎静默。
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "game/Scene.h"
#include "game/WorldView.h"
#include "ui/Widgets.h"

namespace fanren::game {

// 头顶气泡画哪一个（契约 5.2）：一条都没挂 → None；挂着的全是同一种 → 那一种；几种都有 → Generic。
// 世界层经 WorldScene::setPathActionHooks 的 probe 问到这里（钩子在 Application::init 里接上）。
[[nodiscard]] PathBubble pathBubbleFor(const std::vector<const core::PathAction*>& actions);

class PathActionScene : public Scene {
public:
    enum class Phase {
        Menu,      // 列着条目，等玩家选
        Offer,     // 他在开价（对话框压在上面）；说完换上「买 / 不买」
        Confirm,   // 「买 / 不买」
        Done,      // 最后一句话在对话框里；对话框一收，这一层跟着收起
    };

    // npcName：面前那个人的对象名（条目挂在它上面）；speakerRole：他的 role_id，说话人与面板标题都用它。
    PathActionScene(std::string npcName, std::string speakerRole);

    // 菜单一行：「打探」「求购」（右侧写价码）「切磋」。
    [[nodiscard]] static ui::ListItem rowFor(const core::GameData& data,
                                             const core::PathAction& action);
    // 求购的价码：「物品名 × 件数 · N 块」，灵草带年份。价钱只从 price 取——文案里不写钱数（契约 2.3）。
    [[nodiscard]] static std::string purchaseTerms(const core::GameData& data,
                                                   const core::PathAction& action);
    // 这一层自己的固有字（三种行动名、买 / 不买、按键提示、盘缠）。与画面取的是同一批 key，
    // 测试拿境界名去扫它才扫得到画面上真有的字。
    [[nodiscard]] static std::vector<std::string> fixedStrings(const core::GameData& data);

    // 逻辑动作，与按键解耦（ListView 同一个思路）：update 收到按键调它们，无头测试也直接调它们，
    // 于是被测的那条路就是上线的那条路。
    void choose(Application& app, int row);   // 选中菜单第 row 行
    void answer(Application& app, bool buy);  // 「买 / 不买」

    [[nodiscard]] Phase phase() const { return phase_; }
    // 菜单每一行是哪一条，次序即画面上的次序。
    //
    // 指针指进 data().pathActions。契约说规则层给的指针「即取即用」，防的是那张表被换掉；
    // 它开机读一次、运行期不变，菜单开着的这一小会儿拿着是安全的——换成按 id 回查反而要处理
    // 「查不到」这条永远走不到的路（id 只是章内唯一，查法还得带上章号）。
    [[nodiscard]] const std::vector<const core::PathAction*>& rows() const { return rows_; }

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    // 世界照常画在底下（压一层纱）：菜单只是站在那人面前的一问。
    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "PathAction"; }

private:
    void drawMenu(Application& app) const;
    void drawConfirm(Application& app) const;
    void drawKeys(Application& app, const engine::Rect& panel) const;
    [[nodiscard]] std::string title(Application& app) const;

    std::string npc_;
    std::string role_;
    Phase phase_ = Phase::Menu;
    std::vector<const core::PathAction*> rows_;
    const core::PathAction* chosen_ = nullptr;   // 求购：开了价、等「买 / 不买」的那一条
    ui::ListView menu_;
    ui::ListView confirm_;
};

}  // namespace fanren::game
