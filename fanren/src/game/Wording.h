#pragma once
// 面板用词的分阶段开关。
//
// 为什么需要这个东西：第 2 章的硬约束第 8 条写着「七玄门仍是**凡人武馆**，
// 不得出现修仙 / 灵根 / 法术 / 灵气 / 气感 / 炼气 / 筑基 / 真元 / 走火入魔
// 这一类修仙界字眼」。但面板是全局的——后面几章进了修仙界，同一批词又全是
// 正当的。所以不能删词，只能按阶段备两套说法，用同一个开关切。
//
// 这里只放**开关本身**与跨面板共用的那几个词（眼下是货币名）。各面板自己的
// 用词表放在各自的头文件里（修炼面板见 game/CultivationScene.h 的
// CultivationLexicon），免得这里长成一张谁都要改的大表。
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "core/model/Types.h"

namespace fanren::game {

enum class PanelStage {
    Mortal,     // 凡人阶段：他只知道「那段口诀」，口诀连名字都还没有
    Immortal,   // 修仙阶段：第 3 章墨大夫真身揭开之后
};

// 切换用的剧情旗标，登记在 data/flags.json。第 1–5 章都不置（技术债 G-13，docs/ch05-design.md），
// 这几章从面板到战斗一律是凡人用词。
//
// **触发条件取剧情旗标，不取境界**，理由三条：
//
//   1. 按境界切会漏。第 2 章末口诀练到第三层，引擎里就是 QiRefining3；而
//      七玄门那时仍是凡人武馆、墨大夫的真身要到第 3 章才揭开。按境界切，
//      玩家会在第 2 章最后一段忽然看见「炼气三层」「法力」——正是硬约束
//      #8 禁的那批词，而且是在这一章最不该出现的位置上。
//   2. 面板说的是**韩立自己**对这件事的理解，不是系统的客观描述。他没听说
//      过「灵气」这个词以前，面板就不该替他说出来；他听说的那一刻，正好就
//      是剧情事件发生的那一刻。旗标与这件事一一对应，境界不对应。
//   3. 默认值落在安全的一侧。旗标没置 = 凡人用词，**永远不会违约**；万一
//      第 3 章忘了置它，后果只是修士看着「口诀第七层」「碎银」这种土说法
//      （显眼，一眼就能发现），而不是凡人篇里蹦出「真元逆冲」（违约，且
//      多半没人会发现）。宁可错在会被看见的那一边。
inline constexpr const char* kXiuxianKnownFlag = "story.xiuxian_known";

[[nodiscard]] inline PanelStage wordingStage(const core::GameState& state) {
    return state.flag(kXiuxianKnownFlag) != 0 ? PanelStage::Immortal : PanelStage::Mortal;
}

// 货币的显示名。
//
// 字段名 GameState::spiritStones 内部原样不动——它是「钱」这个概念的存档位，
// 改它要动存档、商店、战利品一整串。换的只是屏幕上那两个字。
//
// 两个时代的钱**不换算**：凡人篇挣的是碎银，修仙界用的是灵石，游戏里也没有
// 跨时代的交易，所以不需要汇率，只需要一个名字。
//
// 「灵石」与「灵气」「真元」是同一类词，因此走同一个开关：原著 ch131 才出现
// 它，原文明写是「在修仙者中」买卖物品用的；第 2 章对应的 ch10-27 里韩立连
// 修仙两个字都没听过，同期原著用的是「一两多散银」「几钱的碎银子」。
[[nodiscard]] inline const char* currencyName(PanelStage stage) {
    return stage == PanelStage::Mortal ? "碎银" : "灵石";
}

[[nodiscard]] inline const char* currencyName(const core::GameState& state) {
    return currencyName(wordingStage(state));
}

// 「碎银 12 块」/「灵石 12 块」。两边都按「块」计：碎银本就是按块给的
// （原著同期写「几钱的碎银子」），游戏里不引入两 / 钱两级单位——多一级单位
// 就要多一套换算，而这一章根本没有需要换算的场面。
[[nodiscard]] inline std::string currencyAmount(PanelStage stage, int amount) {
    return std::string(currencyName(stage)) + " " + std::to_string(amount) + " 块";
}

[[nodiscard]] inline std::string currencyAmount(const core::GameState& state, int amount) {
    return currencyAmount(wordingStage(state), amount);
}

// 「法术」一栏与「法力」的叫法（终审 LOW-9）。主菜单在凡人阶段说「法门 / 气力」
//（ui.menu.magic.mortal、修炼面板的 mpLabel），战斗界面从前自己写死「法术 / 法力」——
// 同一个人、同一样东西，翻个面板就换了名字，而且「法术」「法力」正是凡人篇的禁词。
// 战斗界面一律从这里取词；与主菜单是否同词由 tests/GameWiringTests.cpp 钉着。
[[nodiscard]] inline const char* magicWord(PanelStage stage) {
    return stage == PanelStage::Mortal ? "法门" : "法术";
}

[[nodiscard]] inline const char* mpWord(PanelStage stage) {
    return stage == PanelStage::Mortal ? "气力" : "法力";
}

// 规则层（core/battle）的理由与战报是按修仙说法写死的：「法力不足，施展不了【…】」
// 「回复 5 点法力」「【…】不是伤人的法术」。规则层不该知道剧情旗标，所以换词落在转述的这一侧：
// 界面把规则层的句子端上屏之前过一道这里，凡人阶段换成主菜单那一套，修仙阶段原样。
[[nodiscard]] inline std::string stageWords(PanelStage stage, std::string text) {
    if (stage == PanelStage::Immortal) return text;
    for (const auto& [from, to] : {std::pair<std::string_view, std::string_view>{"法术", magicWord(stage)},
                                   std::pair<std::string_view, std::string_view>{"法力", mpWord(stage)}}) {
        for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size())) {
            text.replace(at, from.size(), to);
        }
    }
    return text;
}

}  // namespace fanren::game
