#pragma once
// 修炼面板：打坐与冲关。
//
// 面板只做三件事：把存档状态翻成人话、把玩家的选择翻成对 rules 层的调用、
// 把 rules 层的结果翻回人话。修为怎么涨、关口多难，一概在 core/rules 里，
// 这里不许再写第二套数值——界面各算一套是数值失控最常见的来源。
#include <cstdint>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Cultivation.h"
#include "game/Scene.h"
#include "game/Wording.h"
#include "ui/Widgets.h"

namespace fanren::game {

// ---------------------------------------------------------------------------
// 修炼面板的两套用词
// ---------------------------------------------------------------------------
//
// 阶段开关本身（PanelStage / kXiuxianKnownFlag / wordingStage）与它为什么取
// 剧情旗标而不取境界，见 game/Wording.h；那里也放着同一个开关管着的货币名。
// 这里只放修炼面板自己那一张用词表。
//
// 面板的固有文案，一个阶段一份。
//
// docs/tech-debt.md 记过一条债：面板字符串是源码字面量而非文案 key。这里不
// 顺手去改文案系统（那是另一件事），只把**这一个面板**的字符串收拢进一张表，
// 于是「凡人阶段有没有漏出修仙词」变成可以一条断言扫完的事情。
struct CultivationLexicon {
    // 列表项
    const char* meditateLabel;
    const char* meditateDetail;
    const char* pushLabel;
    const char* pushCapped;         // 已无下一关时的置灰理由
    const char* pushShortPrefix;    // 「尚差 」，后接点数
    const char* pushShortSuffix;    // 「 点修为」
    const char* leaveLabel;
    const char* backLabel;

    // 状态栏
    const char* realmLabel;
    const char* aptitudeLabel;
    const char* dateLabel;
    const char* cultivationLabel;
    const char* cappedNote;
    const char* carryPrefix;
    const char* carrySuffix;
    const char* hpLabel;
    const char* mpLabel;

    // 反馈
    const char* enterHint;
    const char* sitPrefix;          // 「静坐 」
    const char* sitMiddle;          // 「 日，修为增 」
    const char* sitSuffix;          // 「 点。」
    const char* tooShort;
    const char* insight;
    const char* dailyPracticePrefix;   // 日课补账：「这些时日的日课也记上，……增 」
    const char* dailyPracticeSuffix;
    const char* notReady;
    const char* breakSuccessPrefix;
    const char* breakSuccessMiddle;
    const char* breakSuccessSuffix;
    const char* breakFailPrefix;
    const char* breakFailSuffix;
    const char* backlashPrefix;
    const char* backlashSuffix;

    // 剧情境界上限（技术债 G-14）。追加在末尾，两张表按位置初始化，照样对得上。
    //
    // 这两句要与「火候未到」（notReady）、「没冲过去」（breakFail*）说成**完全不同的事**：
    // 那两种是「再坐一阵」「运气差」，这一种是「坐多久都没用，得等」。说成一句的话，
    // 玩家会对着一个永远按不动的按钮一直打坐下去。
    const char* pushAtStoryCap;     // 到了上限时「下一层 / 冲关」那一行的置灰理由
    const char* atStoryCap;         // 到了上限还硬按时的那句反馈
};

[[nodiscard]] const CultivationLexicon& cultivationLexicon(PanelStage stage);

// 境界的显示名。凡人阶段把「炼气三层」说成「第三层」——同一个数，换一张嘴。
[[nodiscard]] std::string realmText(PanelStage stage, rules::Realm realm);

// 这一阶段面板上玩家能看到的全部固有字符串（用词表 + 全部境界名 + 打坐档位）。
// 供禁词扫描测试用；它必须与面板真正用的那些串同源，否则那条测试什么也没测。
[[nodiscard]] std::vector<std::string> cultivationPanelStrings(PanelStage stage);

// 打坐可选的时长。规则层写明「打坐的最小有意义粒度是一旬」，因此这里不给
// 「打坐一个时辰」之类的选项：那种粒度下产出恒为零，玩家点十次也看不到
// 数字动一下。保留「一日」是因为余数账让它确实有收益，只是要攒。
struct MeditateOption {
    std::string label;
    int days = 0;
};

// 一次打坐兑现之后的结果。
//
// cultivation 是**真正进账**的点数，可能小于 rules::meditate 的返回值：
// 不足一点的零头留在 GameState::cultivationRemainder 上，下次接着攒。
struct MeditateOutcome {
    int cultivation = 0;
    int days = 0;
    bool insight = false;
};

// 打坐效率。功法、灵根、洞府灵气将来都要并进这里；眼下只有基准值 100。
// 入口先具名留好——散在各面板里现算，迟早两处算得不一样。
[[nodiscard]] int meditationEffectiveness(const core::GameState& state);

// 冲关的丹药加成（百分点）。筑基丹之类尚未实现，恒为 0；具名出来是为了
// 将来接丹药时只有一处可改。
[[nodiscard]] int breakthroughPillBonus(const core::GameState& state);

// ---------------------------------------------------------------------------
// 日课：让「四年苦修」真的是四年的修行，而不是按几次按钮
// ---------------------------------------------------------------------------
//
// 第 2 章的四年绝大部分是剧情推的（五个段末合计 advance_days 约 1100 天），
// 而 advance_days 只结算灵田与凝液，**修为一分不给**。于是那四年在修为账上
// 完全是空的，玩家要的修为只能靠面板上按「闭关一年」现攒——「四年苦修」就
// 塌成了按钮次数。
//
// 日课把这段补上：凡人（口诀）阶段，每过去的日子都算他照旧把那段口诀默了
// 一遍。但杂役的活计、药圃的功课、师父的差遣占去大半，**五日的日课只折三日
// 静坐**——所以「主动去打坐」仍然明显更划算，日课不是替代品。
//
// 为什么只在凡人阶段计：这一段是全作唯一由剧情整年整年地替玩家推日历的地方
// （四年苦修写在大纲里），日历既然不由玩家掌握，修为就不能只认玩家的点击。
// 进了修仙界之后闭关多久是玩家自己的决定，那时再叠一层被动收益，等于把后面
// 每一章的节奏都改了——那正是不该动的东西。
//
// 折算按整块结算，零头留在水位上：连着结十次与攒够了一次结，结果完全相同。
inline constexpr int kDailyPracticeBlockDays = 5;
inline constexpr int kDailyPracticeSessionsPerBlock = 3;

// 把 state.lastPracticeDay 到今天之间欠的日课补进修为，返回本次进账的点数。
// 水位为 0（新档，或这次改动之前存下的老档）时只把水位拨到当天、不补账：
// 老档不该因为一次升级白得几百天修为。
int settleDailyPractice(core::GameState& state);

class CultivationScene : public Scene {
public:
    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    // 本面板的列表页大小：面板高度去掉标题带之后画得满几行。
    // enter*() 设页大小与 render() 摆列表区必须走同一份高度，否则会出现
    // 「区域明明画得下十几行，却因为页大小是默认的 8 而只显示 8 行」——
    // 多出来的那几行玩家得按方向键才找得到，而他多半不知道下面还有。
    // 公开是为了让无头测试能直接断言「今天这张表一行都不会被藏起来」。
    [[nodiscard]] static int listPageRows(const ui::Theme& theme);

    // 面板是覆盖层：底下的世界照画，玩家才知道自己是在哪儿打坐。
    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "Cultivation"; }

    // ---- 纯逻辑，公开供无头测试直接驱动 ----
    // （与 WorldScene 的三个判定同一个理由：藏在实现文件里的规则没人测得到。）

    // 凡人阶段不列「闭关一年」：一个十来岁的武馆杂役没有闭关一年的条件，
    // 而那一个选项正是「四年苦修按一次按钮就过去了」的来源。
    [[nodiscard]] static const std::vector<MeditateOption>& meditateOptions(PanelStage stage);
    [[nodiscard]] static std::vector<ui::ListItem> buildMainItems(const core::GameState& state);

    // 把 days 天打坐的收益兑现进 state.cultivation，零头留在余数账上。
    // 余数账的算法与「长短打坐等价」的保证见 .cpp 顶部的长注释。
    static MeditateOutcome bankMeditation(core::GameState& state, int days, std::uint32_t seed);

    // 打坐一次：兑现收益 + 推进日历。日历一律走 Application::advanceDays。
    MeditateOutcome meditateFor(Application& app, int days);

    // 冲关一次。返回规则层的原始判定，顺带把它翻成 feedback() 里的一句话。
    // 修为不足时不摇骰子、不扣分，返回全零的 attempt。
    rules::BreakthroughAttempt breakthrough(Application& app);

    // 最近一次操作的反馈，可直接上屏。
    [[nodiscard]] const std::string& feedback() const { return feedback_; }

private:
    // 两级菜单：主菜单选动作，打坐再选时长。
    enum class Mode { Main, Duration };

    // 带上 theme 是为了按面板高度设列表页大小：页大小与摆位必须同一份几何，
    // 只传 state 的话这里只能猜一个魔数，而魔数与面板高度迟早对不上。
    void enterMain(const core::GameState& state, const ui::Theme& theme);
    void enterDuration(const core::GameState& state, const ui::Theme& theme);
    void renderStatus(Application& app, const engine::Rect& area) const;

    Mode mode_ = Mode::Main;
    ui::ListView main_;
    ui::ListView duration_;
    std::string feedback_;
};

}  // namespace fanren::game
