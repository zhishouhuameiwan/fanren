#include "core/rules/Roster.h"

namespace fanren::rules {

std::vector<std::string> roster(const std::map<std::string, core::Companion>& companions,
                                const core::GameState& state) {
    std::vector<std::string> out;
    for (const auto& [id, companion] : companions) {
        if (!companion.metFlag.empty() && state.flag(companion.metFlag) != 0) out.push_back(id);
    }
    return out;
}

}  // namespace fanren::rules
