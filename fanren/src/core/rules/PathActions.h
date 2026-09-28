#pragma once
// 路径行动的规则：某个 NPC 身上此刻挂着哪几条、能不能做、做了会怎样。
//
// 契约 docs/interfaces-octo-pathactions.md 第 3 节。core 层硬约束：不含 SDL / JSON / Lua /
// 文件 IO，只在已解析的结构体上运算，于是可以不开窗口直接单测。
//
// ---------------------------------------------------------------------------
// 为什么全是纯函数、全取 const GameState&
// ---------------------------------------------------------------------------
// 与任务系统（rules/Quests.h）同一个口径：规则层只算，改存档的是游戏层。
// 「做了会怎样」给的是一张效果清单（PathEffect，纯数据），游戏层照单施加——
// 于是「路径行动顺手改了点剧情旗标」这件事从签名上就不成立，而清单本身可以直接断言。
#include <string>
#include <vector>

#include "core/model/Types.h"

namespace fanren::rules {

// 碎银就是这件物品。game/ShopScene.h 的 kSpiritStoneItemId 是同一个 id；core 引不到 game，
// 所以这里另写一份，tests/PathActionTests.cpp 钉着两处相等——一处改了另一处没跟，当场红。
inline constexpr const char* kPathCurrencyItemId = "material_lingshi";

// 时段开没开：when 全部成立，且 until 无一成立。
//
// **空的 when 不算成立**：按 AND 的数学口径它恒真，等于「一开局就挂着、挂到天荒地老」，
// 必是数据漏写（加载器不收，这里也不信它）。空的 until 就是「不收起」，那是 OR 的本义。
[[nodiscard]] bool pathWindowOpen(const core::PathAction& action, const core::GameState& state);

// 此刻挂不挂出来：
//   · 时段开着；
//   · 求购买成过、切磋赢过的**收起**（每条只买一次；赢过的人不再陪你打）；
//   · 编成还没建好的切磋（pending）**不挂**——挂出来就是一个按下去什么也打不起来的选项；
//   · 打探做过仍然挂着：再按一次是重看那段情报，效果不重发（见 pathActionEffects）。
[[nodiscard]] bool pathActionShown(const core::PathAction& action, const core::GameState& state);

// 某个 NPC 身上此刻挂着的条目，按 打探 → 求购 → 切磋、同类按 id 排好。
// 指针指进 actions，actions 一变就失效，即取即用。
[[nodiscard]] std::vector<const core::PathAction*> pathActionsAt(
    const std::vector<core::PathAction>& actions, const core::GameState& state,
    const std::string& mapId, const std::string& npc);

// 能不能做、为什么不能。
enum class PathBlock {
    None,
    RealmTooLow,     // 阅历不足：韩立的境界不到条目的门槛
    NotEnoughMoney,  // 求购：身上的碎银不够
};

struct PathVerdict {
    PathBlock block = PathBlock::None;
    // 对方怎么回绝：阅历不足是 refuseKey，钱不够是 poorKey。能做时为空。
    std::string reasonKey;

    [[nodiscard]] bool allowed() const { return block == PathBlock::None; }
};

// 先看阅历、再看钱：阅历不到，对方连价都不开，谈不上钱够不够。
// 不看时段——调用方先用 pathActionsAt 取到的才是挂着的；这里只回答「挂着的这一条做不做得成」。
[[nodiscard]] PathVerdict pathActionVerdict(const core::PathAction& action,
                                            const core::GameState& state);

// 一条效果。哪几个字段有意义由 kind 定，其余留空。
struct PathEffect {
    enum class Kind {
        Say,              // textKey：条目挂着的那个 NPC 说一句（说话人按那个 npc 对象的 role_id）
        RevealWeakness,   // reveal：揭开一类破绽（写进「已知破绽」归战斗路）
        GiveItem,         // item：给物件
        TakeItem,         // item：扣物件（求购付的碎银）
        SetFlag,          // flag：置为 1
        StartBattle,      // battleId：开战（切磋的编成，输了不死）
        GainCultivation,  // amount：加修为
    };
    Kind kind = Kind::Say;
    std::string textKey;
    core::PathReveal reveal;
    core::BagEntry item;
    std::string flag;
    std::string battleId;
    int amount = 0;
};

// 做这一条会发生什么（调用方已按 pathActionVerdict 确认做得成）：
//   · 打探，头一回：说情报 → 揭破绽 → 给物件 → 置旗标 → 记做过（doneFlag）；
//   · 打探，做过了：只说情报（重看，不重发）；
//   · 求购：说成交 → 付碎银 → 得物件 → 记买过；
//   · 切磋：说邀战 → 开战。收场另见 challengeResultEffects。
// 次序就是施加次序：记做过永远在最后，前面任何一步没施加成，这一条就不算做过。
[[nodiscard]] std::vector<PathEffect> pathActionEffects(const core::PathAction& action,
                                                        const core::GameState& state);

// 切磋收场：胜 → 说胜 → 加修为 → 给物件 → 记赢过；负（逃也算负）→ 只说负，不记，可以再来。
// action 不是切磋时返回空表。
[[nodiscard]] std::vector<PathEffect> challengeResultEffects(const core::PathAction& action,
                                                             bool won);

// 已经打探过的条目揭开了哪些破绽：按（roleId，category）排好、去重。
//
// 打探做没做过记在 doneFlag 上，于是「路径行动揭开了什么」本身就能从存档推出来——
// 战斗路合回后可以在开战时并进已知破绽，也可以在打探那一刻照 RevealWeakness 写进去，
// 两条路给出同一个答案（契约第 5 节）。
[[nodiscard]] std::vector<core::PathReveal> pathRevealedWeaknesses(
    const std::vector<core::PathAction>& actions, const core::GameState& state);

// 还标着 pending 的切磋（编成尚未建好），按章号、id 排好。门禁与测试用：它们在第 5 章收尾前必须清零。
[[nodiscard]] std::vector<const core::PathAction*> pendingChallenges(
    const std::vector<core::PathAction>& actions);

}  // namespace fanren::rules
