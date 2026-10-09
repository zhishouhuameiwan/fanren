#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include "../vendor/json.hpp"

namespace fanren::test {

// Text files are flat JSON objects. Return every string pair; callers retain their own scope gates.
inline std::map<std::string,std::string> textKeyValues(const std::string& source) {
    const auto document=nlohmann::json::parse(source);
    if (!document.is_object()) throw std::invalid_argument("Text keys require a JSON object");
    std::map<std::string,std::string> result;
    for (const auto& [key,value] : document.items())
        if (value.is_string()) result.emplace(key,value.get<std::string>());
    return result;
}
} // namespace fanren::test
