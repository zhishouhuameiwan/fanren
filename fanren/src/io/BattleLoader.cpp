#include "io/BattleLoader.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace fanren::io {
namespace {

using nlohmann::json;

[[nodiscard]] std::string readWholeFile(const std::string& path, bool& ok) {
    std::ifstream stream(path, std::ios::binary);
    ok = static_cast<bool>(stream);
    if (!ok) return {};
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

[[nodiscard]] int readInt(const json& node, const char* key, int fallback) {
    if (!node.contains(key) || !node[key].is_number_integer()) return fallback;
    return node[key].get<int>();
}

[[nodiscard]] bool readBool(const json& node, const char* key, bool fallback) {
    if (!node.contains(key) || !node[key].is_boolean()) return fallback;
    return node[key].get<bool>();
}

[[nodiscard]] std::string readString(const json& node, const char* key) {
    if (!node.contains(key) || !node[key].is_string()) return {};
    return node[key].get<std::string>();
}

}  // namespace

core::Result<core::BattleSetup> loadBattle(const std::string& path) {
    using Out = core::Result<core::BattleSetup>;

    bool readable = false;
    const std::string raw = readWholeFile(path, readable);
    if (!readable) return Out::failure("战斗配置读不到：" + path);

    json parsed;
    try {
        parsed = json::parse(raw);
    } catch (const json::parse_error& error) {
        return Out::failure(std::string("战斗配置 JSON 语法错误：") + path + " — " + error.what());
    }
    if (!parsed.is_object()) return Out::failure("战斗配置必须是对象：" + path);

    core::BattleSetup setup;
    setup.id = readString(parsed, "id");
    if (setup.id.empty()) return Out::failure("战斗配置缺少 id：" + path);

    setup.name = readString(parsed, "name");
    setup.chapter = readInt(parsed, "chapter", 1);
    setup.terrain = readString(parsed, "terrain");
    // width / height / player_spawn 与单位的 x / y 不再读：横版战斗没有格子
    //（docs/octopath-battle.md 第 5 节）。数据里残留的由 tools/validate.py 报错。
    //
    // 战斗背景（画面路用）：可选，写了就得是字符串——写成别的类型的人本意是指定一张，
    // 悄悄当成「没写」会让这一场默默退回按地形推的那一张，而那只在截图里看得出来。
    if (parsed.contains("backdrop")) {
        if (!parsed["backdrop"].is_string()) {
            return Out::failure("战斗 " + setup.id + " 的 backdrop 必须是字符串：" + path);
        }
        setup.backdrop = parsed["backdrop"].get<std::string>();
    }
    setup.canEscape = readBool(parsed, "can_escape", true);
    setup.defeatIsFatal = readBool(parsed, "defeat_is_fatal", true);
    setup.introKey = readString(parsed, "intro_key");

    // ---- 韩立不在场（契约 docs/interfaces-p3-ch05.md 第 2 节）----
    //
    // 缺字段按 false 收：现有全部编成一个字不改。**写了却不是布尔当场报错**，不走
    // readBool 那条「类型不对就用缺省」的路：写 "hero_absent": "true" 的人本意明明是
    // 开，悄悄按关收下，夺帮那一夜韩立就会从客栈的床上出现在四平帮的院子里，
    // 而那只在真打起来时才看得见。
    if (parsed.contains("hero_absent")) {
        if (!parsed["hero_absent"].is_boolean()) {
            return Out::failure("战斗 " + setup.id + " 的 hero_absent 必须是 true / false：" + path);
        }
        setup.heroAbsent = parsed["hero_absent"].get<bool>();
    }

    if (!parsed.contains("units") || !parsed["units"].is_array()) {
        return Out::failure("战斗 " + setup.id + " 缺少 units 数组");
    }
    for (const json& entry : parsed["units"]) {
        if (!entry.is_object()) continue;
        core::BattleUnitSpec unit;
        unit.roleId = readString(entry, "role_id");
        if (unit.roleId.empty()) {
            return Out::failure("战斗 " + setup.id + " 有单位缺少 role_id");
        }
        // 阵营缺省按敌方处理：编成里绝大多数是敌人，写错时宁可多一个敌人，
        // 也好过悄悄把敌人算成友军让战斗永远打不完。
        unit.ally = readString(entry, "faction") == "ally";

        // ---- 波次（P3 第 4 章，契约 docs/interfaces-p3-ch04.md 第 2.2 节）----
        //
        // **缺字段按 0 收**：没写波次的单位开场就在场上。这一条有测试钉着。
        //
        // 非整数或负数当场报错，不夹到 0 悄悄收下：写 "wave": -1 的人本意多半是
        // 「最后一波」，夹成 0 会让他写的那一波从开场就站在场上——而那正好是
        // 多波次唯一要做对的那件事的反面，且症状只在真打起来时才看得见。
        if (entry.contains("wave") && !entry["wave"].is_number_integer()) {
            return Out::failure("战斗 " + setup.id + " 的单位 " + unit.roleId +
                                " 的 wave 不是整数：" + path);
        }
        unit.wave = readInt(entry, "wave", 0);
        if (unit.wave < 0) {
            return Out::failure("战斗 " + setup.id + " 的单位 " + unit.roleId +
                                " 的 wave 为负（波次从 0 起算）：" + path);
        }
        setup.units.push_back(std::move(unit));
    }
    if (setup.units.empty()) {
        return Out::failure("战斗 " + setup.id + " 的 units 是空的");
    }

    // ---- 波次号必须是 0..N 连续无空档 ----
    //
    // 这道闸拦的是 "wave": 2 这种笔误（本想写第二波，而波次从 0 起算）。
    // 引擎本身容得下空档（BattleState::refreshPhase 会把空的一波直接推过去），
    // 但那意味着战斗中途凭空拖一拍、日志上写一句「却不见人影」——玩家看到的是
    // 「打完了却没结束」，而没有任何工具会说这是数据写错了。
    //
    // 同时保住「必有第 0 波」：全员都排在后面的编成开场无人，第一件事就是推波，
    // 那与直接把他们写成第 0 波是同一场仗，只是多了一行谁也不懂的日志。
    {
        // 负波次在上面那道闸就被拦掉了，所以这里的 wave 必然非负。
        //
        // **但本段不假设那道闸还在**：下面拿 wave 当下标索引 present，一个负数
        // 会被 static_cast 成一个天文数字，然后越界写进 std::vector<bool>。
        // 这不是假设出来的风险——本次的突变验证（把那道闸改成 if (false)）当场
        // 换来一个 0xc0000005 访问违例。判定顺序是会被后人重排的，而重排之后
        // 症状不是一条错误消息，是一次崩溃。所以这里自带一道下标闸。
        int maxWave = 0;
        for (const core::BattleUnitSpec& unit : setup.units) {
            if (unit.wave < 0) continue;
            maxWave = std::max(maxWave, unit.wave);
        }
        std::vector<bool> present(static_cast<std::size_t>(maxWave) + 1, false);
        for (const core::BattleUnitSpec& unit : setup.units) {
            if (unit.wave < 0 || unit.wave > maxWave) continue;
            present[static_cast<std::size_t>(unit.wave)] = true;
        }
        for (std::size_t w = 0; w < present.size(); ++w) {
            if (present[w]) continue;
            return Out::failure("战斗 " + setup.id + " 的波次号不连续：最大波次 " +
                                std::to_string(maxWave) + "，但第 " + std::to_string(w) +
                                " 波一个单位也没有（波次从 0 起算，0 号开场即在场上）：" + path);
        }
    }

    // ---- 韩立不在场的仗，开场必须有友军 ----
    //
    // 一个友军都没有，或者友军全排在后面几波：开场场上我方无人，BattleState::setup 里
    // 那一次 refreshPhase 当场判负——那不是一场仗，是一句「你输了」。契约 2.1 要它
    // 在加载期就报出来，不许开出一场只有敌人的仗。
    if (setup.heroAbsent) {
        const bool allyAtStart =
            std::any_of(setup.units.begin(), setup.units.end(),
                        [](const core::BattleUnitSpec& unit) { return unit.ally && unit.wave == 0; });
        if (!allyAtStart) {
            return Out::failure("战斗 " + setup.id +
                                " 开了 hero_absent（韩立不在场），第 0 波里却一个 ally 也没有："
                                "开场即判负，那不是一场仗：" + path);
        }
    }

    if (parsed.contains("rewards") && parsed["rewards"].is_object()) {
        const json& rewards = parsed["rewards"];
        setup.reward.cultivation = readInt(rewards, "cultivation", 0);
        setup.reward.spiritStones = readInt(rewards, "spirit_stones", 0);
        if (rewards.contains("drops") && rewards["drops"].is_array()) {
            for (const json& drop : rewards["drops"]) {
                if (!drop.is_object()) continue;
                core::BagEntry item;
                item.itemId = readString(drop, "item_id");
                if (item.itemId.empty()) continue;
                item.count = readInt(drop, "count", 1);
                item.herbAge = readInt(drop, "herb_age", 0);
                setup.reward.drops.push_back(std::move(item));
            }
        }
    }

    return Out::success(std::move(setup));
}

core::Result<std::map<std::string, core::BattleSetup>> loadBattles(const std::string& battlesRoot) {
    using Out = core::Result<std::map<std::string, core::BattleSetup>>;
    namespace fs = std::filesystem;

    std::map<std::string, core::BattleSetup> all;
    std::error_code ec;
    if (!fs::exists(battlesRoot, ec)) {
        // 目录不存在按 0 场处理：早期章节还没有战斗不该让整局加载失败。
        return Out::success(std::move(all));
    }

    std::map<std::string, std::string> sourceOf;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(battlesRoot, ec)) {
        if (ec) break;
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;

        auto one = loadBattle(entry.path().string());
        if (!one) return Out::failure(one.error);

        const std::string id = one.value.id;
        const auto existing = sourceOf.find(id);
        if (existing != sourceOf.end()) {
            return Out::failure("战斗 id 重复：" + id + "，见 " + existing->second + " 与 " +
                                entry.path().string());
        }
        sourceOf.emplace(id, entry.path().string());
        all.emplace(id, std::move(one.value));
    }

    return Out::success(std::move(all));
}

}  // namespace fanren::io
