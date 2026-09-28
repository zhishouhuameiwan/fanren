#include "core/rules/Quests.h"

#include <algorithm>
#include <cstddef>

#include "core/rules/Objectives.h"

namespace fanren::rules {
namespace {

// 支线的进度：走得最远的那一条已做完的步骤之后那一格的下标。
//
// 与 Objectives.cpp 的 progressIndex 同一个口径，理由也一样，外加一条支线独有的：
// 物品谓词会回落（清灵散在潇湘院用掉了），按「第一条没做完的」算，当前步骤会倒退回
// 「去备一份清灵散」，而玩家早就走过那一步了。
//
// done 为空的步骤一律当作没做完（与目标链里空 doneFlag 同一个理由）：加载器不许它为空，
// 这里只兜「数据绕过了加载器」这一种情形。
[[nodiscard]] std::size_t sideProgress(const core::Quest& quest, const core::GameState& state) {
    std::size_t next = 0;
    for (std::size_t i = 0; i < quest.steps.size(); ++i) {
        const core::QuestStep& step = quest.steps[i];
        if (!step.done.empty() && allConditionsHold(step.done, state)) next = i + 1;
    }
    return next;
}

// 主线任务在整条链上占的那一段 [first, last]，与当前进度下标。
struct MainSpan {
    bool present = false;
    std::size_t first = 0;
    std::size_t last = 0;
    std::size_t progress = 0;   // 当前那一步的下标；走完为链长
};

[[nodiscard]] MainSpan mainSpan(const core::Quest& quest,
                                const std::vector<core::Objective>& mainChain,
                                const core::GameState& state) {
    MainSpan span;
    for (std::size_t i = 0; i < mainChain.size(); ++i) {
        if (mainChain[i].chapter != quest.chapter) continue;
        if (!span.present) span.first = i;
        span.last = i;
        span.present = true;
    }
    // 进度只问 currentObjective 一个：HUD 问的就是它，主线任务的状态若另算一遍，
    // 两处早晚会说着两件不同的事。指针指进 mainChain，换算成下标即可。
    const core::Objective* current = currentObjective(mainChain, state);
    span.progress = current == nullptr
                        ? mainChain.size()
                        : static_cast<std::size_t>(current - mainChain.data());
    return span;
}

[[nodiscard]] int statusRank(QuestStatus status) {
    // 告示板的分组次序。**不要加 default:**：新增一种状态时让编译器告警，
    // 而不是悄悄落进某一组。
    switch (status) {
        case QuestStatus::Active: return 0;
        case QuestStatus::Failed: return 1;
        case QuestStatus::Completed: return 2;
        case QuestStatus::NotAccepted: return 3;
    }
    return 3;
}

}  // namespace

bool conditionHolds(const core::QuestCondition& condition, const core::GameState& state) {
    // 空的主语是数据写漏了（加载器不收），这里判不成立：一条「旗标名为空串」的谓词
    // 若按 flag("") == 0 去比，`== 0` 那种写法会恒真，一条漏写的谓词就成了一句空话。
    if (condition.subject.empty()) return false;
    switch (condition.op) {
        case core::QuestCondition::Op::FlagAtLeast:
            return state.flag(condition.subject) >= condition.value;
        case core::QuestCondition::Op::FlagEquals:
            return state.flag(condition.subject) == condition.value;
        case core::QuestCondition::Op::ItemAtLeast:
            return state.itemCount(condition.subject) >= condition.value;
    }
    return false;
}

bool allConditionsHold(const std::vector<core::QuestCondition>& conditions,
                       const core::GameState& state) {
    return std::all_of(conditions.begin(), conditions.end(),
                       [&state](const core::QuestCondition& c) { return conditionHolds(c, state); });
}

QuestStatus questStatus(const core::Quest& quest, const std::vector<core::Objective>& mainChain,
                        const core::GameState& state) {
    if (quest.kind == core::QuestKind::Main) {
        const MainSpan span = mainSpan(quest, mainChain, state);
        // 链上没有这一章的步骤：这条主线无事可做，不显示。
        if (!span.present) return QuestStatus::NotAccepted;
        if (span.progress > span.last) return QuestStatus::Completed;
        if (span.progress >= span.first) return QuestStatus::Active;
        return QuestStatus::NotAccepted;
    }

    // 支线。空的 accept 是数据漏写（加载器不收）；按 AND 口径它恒真，等于「一开局就接了」，
    // 所以这里不信它：没有接取条件的支线永远不算接了。
    if (quest.accept.empty() || !allConditionsHold(quest.accept, state)) {
        return QuestStatus::NotAccepted;
    }
    if (!quest.complete.empty() && allConditionsHold(quest.complete, state)) {
        return QuestStatus::Completed;
    }
    // fail 为空 = 不会过期。这里**不能**写成 allConditionsHold(quest.fail, …)：
    // 空表的 AND 为真，那样每一条不会过期的任务一接下就是「已过期」。
    if (!quest.fail.empty() && allConditionsHold(quest.fail, state)) {
        return QuestStatus::Failed;
    }
    return QuestStatus::Active;
}

const core::QuestStep* currentQuestStep(const core::Quest& quest,
                                        const std::vector<core::Objective>& mainChain,
                                        const core::GameState& state) {
    if (questStatus(quest, mainChain, state) != QuestStatus::Active) return nullptr;

    if (quest.kind == core::QuestKind::Main) {
        const core::Objective* current = currentObjective(mainChain, state);
        if (current == nullptr) return nullptr;
        // 按 id 找而不是按下标换算：同一章里步骤 id 唯一（门禁规则 22），
        // 而下标换算要假定「本任务的步骤与链上那一段一一对齐」，那是加载器的事，不是这里的。
        for (const core::QuestStep& step : quest.steps) {
            if (step.id == current->id) return &step;
        }
        return nullptr;
    }

    const std::size_t next = sideProgress(quest, state);
    return next < quest.steps.size() ? &quest.steps[next] : nullptr;
}

std::vector<QuestEntry> sideQuestJournal(const std::vector<core::Quest>& quests,
                                         const core::GameState& state) {
    // 支线不读目标链；传一条空链给 questStatus / currentQuestStep 即可。
    static const std::vector<core::Objective> kNoChain;
    std::vector<QuestEntry> entries;
    for (const core::Quest& quest : quests) {
        if (quest.kind != core::QuestKind::Side) continue;
        const QuestStatus status = questStatus(quest, kNoChain, state);
        if (status == QuestStatus::NotAccepted) continue;
        entries.push_back(QuestEntry{&quest, status, currentQuestStep(quest, kNoChain, state)});
    }
    std::stable_sort(entries.begin(), entries.end(), [](const QuestEntry& a, const QuestEntry& b) {
        const int ra = statusRank(a.status);
        const int rb = statusRank(b.status);
        if (ra != rb) return ra < rb;
        if (a.quest->chapter != b.quest->chapter) return a.quest->chapter > b.quest->chapter;
        return a.quest->id < b.quest->id;
    });
    return entries;
}

}  // namespace fanren::rules
