#include "game/Guide.h"

#include "core/rules/Objectives.h"

namespace fanren::game::guide {

bool upgradedObjective(const core::Objective& step) {
    return !step.hintKey.empty() || !step.detailKey.empty() ||
           step.progress.kind != core::ObjectiveProgress::Kind::None;
}

std::string objectiveActionLine(const core::GameData& data, const core::GameState& state,
                                const core::Objective& step) {
    std::string line = data.lookupText(step.textKey);
    rules::ProgressCount count;
    if (rules::objectiveProgress(step, state, count)) line += "　" + rules::progressLabel(count);
    return line;
}

std::string viaClause(const std::vector<std::string>& mapNames) {
    if (mapNames.empty()) return {};
    std::string out = "（经";
    for (std::size_t i = 0; i < mapNames.size() && i < kMaxVia; ++i) {
        if (i > 0) out += "、";
        out += mapNames[i];
    }
    if (mapNames.size() > kMaxVia) out += "…";
    return out + "）";
}

}  // namespace fanren::game::guide
