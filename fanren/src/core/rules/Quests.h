#pragma once
// 任务的状态：在一份任务表与一份存档之间，回答「这件事现在是什么状态、该做哪一步」。
//
// 契约 docs/interfaces-p3-ch05.md 第 1 节。core 层硬约束：不含 SDL / JSON / Lua /
// 文件 IO，只在已解析的结构体上运算，于是可以不开窗口直接单测。
//
// ---------------------------------------------------------------------------
// 为什么全是纯函数、全取 const GameState&
// ---------------------------------------------------------------------------
// 契约 Q2 / Q4：状态全由旗标与背包推出，**不开新的存档字段，也不发奖励**。
// 签名上就拿不到可改的存档，于是「任务系统顺手改了点什么」这件事在编译期就不成立，
// 不必靠谁记得。接、推进、交、过期都是剧情脚本本来就要做的事（置旗标、give / take）。
#include <vector>

#include "core/model/Types.h"

namespace fanren::rules {

// 四种状态（契约 1.3）。告示板只显示后三种。
enum class QuestStatus {
    NotAccepted,   // 未接：accept 不全成立
    Active,        // 进行中
    Completed,     // 已了结：accept 成立，complete 成立
    Failed,        // 已过期：accept 成立，complete 不成立，fail 非空且全成立
};

// 一条谓词成不成立。三种写法见 core::QuestCondition。
[[nodiscard]] bool conditionHolds(const core::QuestCondition& condition,
                                  const core::GameState& state);

// 一张谓词表（AND）成不成立。**空表为真**，按数学口径。
//
// 所以 accept / complete / 步骤的 done 在加载器里不许为空（空的就是「一开局就接了」
// 「什么都不做就算完」，必是数据漏写）；fail 为空另判为「不会过期」，不走这里的 AND
// ——见 questStatus。这是唯一一处例外，写在这里免得有人「统一」掉。
[[nodiscard]] bool allConditionsHold(const std::vector<core::QuestCondition>& conditions,
                                     const core::GameState& state);

// 这条任务现在是什么状态。
//
// 支线：判定次序**已接 → 了结 → 过期**。了结压过过期：第 5 章 Z1 的 fail 是 ch05.done，
// 交过医书的人章末照样置 ch05.done，他的任务必须是「已了结」而不是「已过期」。
// 没接的一律 NotAccepted，哪怕 complete / fail 已经成立——玩家从没听说过的事谈不上「消失」。
//
// 主线：**不另算**，照目标链判据（rules::currentObjective）。这一章的步骤在整条链上占
// [first, last]，当前进度下标 p（走完为链长）：p > last 已了结、first ≤ p ≤ last 进行中、
// p < first 未接。于是任意时刻至多一章主线在进行中，且正是 HUD 上那一步所在的那一章。
// mainChain 就是 GameData::objectives；支线不读它。
[[nodiscard]] QuestStatus questStatus(const core::Quest& quest,
                                      const std::vector<core::Objective>& mainChain,
                                      const core::GameState& state);

// 进行中那条任务眼下该做的那一步；不是 Active 时返回 nullptr。
//
// 支线：**走得最远的那一条已做完的步骤之后那一条**，与目标链同一个口径
// （Objectives.cpp 的 progressIndex）。物品谓词会回落（药吃掉了）：按「第一条没做完的」
// 算，步骤会倒退回去；按「走得最远的」算不会。全部做完而 complete 还不成立时返回 nullptr，
// 告示板改显示 summary。
//
// 主线：链上当前那一步在本任务里对应的那一条（按步骤 id 找）。
//
// 返回的指针指进 quest.steps，quest 一变就失效，即取即用。
[[nodiscard]] const core::QuestStep* currentQuestStep(const core::Quest& quest,
                                                      const std::vector<core::Objective>& mainChain,
                                                      const core::GameState& state);

// 告示板上的一条支线。
struct QuestEntry {
    const core::Quest* quest = nullptr;
    QuestStatus status = QuestStatus::NotAccepted;
    const core::QuestStep* step = nullptr;   // 只有 Active 才可能非空
};

// 告示板要列的支线：只收 kind == Side、只收 NotAccepted 以外的。
//
// 排序：**进行中 → 已过期 → 已了结**；组内按章号倒序（新的在上），同章按 id。
// 进行中的排最前，因为那是玩家打开告示板要找的东西；过期的紧随其后，因为契约 Q7 要它
// 「不许静默消失」——排在一长串已了结的后面，与消失也差不了多少。
//
// 指针指进 quests，quests 一变就失效。
[[nodiscard]] std::vector<QuestEntry> sideQuestJournal(const std::vector<core::Quest>& quests,
                                                       const core::GameState& state);

}  // namespace fanren::rules
