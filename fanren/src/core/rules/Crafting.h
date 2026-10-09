#pragma once
// 炼制四艺：炼丹、制符、炼器、布阵。纯规则，不依赖 SDL / JSON / Lua / 文件 IO。
//
// 配方以 Recipe 结构体传入而不在本层读盘：读 data/recipes/ 是 io 层的事，
// 规则层只在已解析的结构体上算，这样每条判定都能脱离文件系统单测。
// 随机数种子由调用方传入，同种子同输入必得同结果——炼制要能随存档回放。
//
// 四艺共用一套判定骨架，性格差异只落在两处：成功率里的炉鼎品阶，以及失败时的
// 材料损耗策略（failurePolicyOf）。差异收在这两处，日后添第五艺只需加一行策略，
// 不必再抄一遍判定流程。
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/Result.h"
#include "core/model/Types.h"

namespace fanren::rules {

enum class CraftKind { Alchemy, Talisman, Forge, Formation };

// 一味材料。
struct Ingredient {
    std::string itemId;
    int count = 1;
    int minAge = 0;      // 灵草最低年份要求，非灵草为 0

    // ---- 以下两位为 P2 追加（契约 v2 原文未覆盖），按 P1 全局约定第 4 条一律
    //      加在结构体末尾，既有的聚合初始化 {id, count, minAge} 不受影响。----

    // 实际扣的是哪一堆。仅在 CraftResult::consumed 中有意义：背包里同种灵草按
    // 年份分堆，只报「扣了 2 株」而不报「扣的哪一堆」，调用方就无从落账。
    // 在 Recipe::inputs 中恒为 0（配方只提门槛，不指定具体年份）。
    int herbAge = 0;

    // 「引」而非「料」：制符的妖丹、布阵的阵旗，入不了火也烧不掉，失败时能捡回来。
    // 没有这一位就只能靠物品 id 前缀去猜哪味是引，那等于把 data/ 的命名约定
    // 焊进 core 层。
    bool catalyst = false;

    friend bool operator==(const Ingredient&, const Ingredient&) = default;
};

struct Recipe {
    std::string id;
    std::string name;
    CraftKind kind{};
    std::string productId;
    int productCount = 1;
    int difficulty = 0;              // 0-100
    int requiredProficiency = 0;     // 熟练度门槛
    // 同一个 itemId 允许出现多条，用来写分级配方（「十年草 2 株 + 百年草 1 株」）。
    // 判定时按 itemId 先聚合总量再比，绝不能让每条需求各自去完整背包里认领一遍。
    std::vector<Ingredient> inputs;
    // 门闸旗标（P3 第 7 章增补，契约 docs/interfaces-p3-ch07.md 第 1 节）：旗标未置 = 这张方子
    // 「还没得到」，面板不列、也开不了工（recipeKnown）。空串 = 没有门闸。追加在末尾，
    // 按字段顺序写的聚合初始化不受影响。
    std::string requireFlag;
};

// 炼制一次的结果。失败时材料是否损毁由 kind 决定：
// 炼丹失败材料尽毁，制符失败只废符纸，这是两种手艺的性格差异。
struct CraftResult {
    bool success = false;
    std::string productId;              // 失败时为空串：没成就是没产物，不留半个 id
    int productCount = 0;
    int proficiencyGain = 0;
    // 实际消耗掉的材料，逐堆列出，按 Recipe::inputs 的原顺序呈现。
    // 按 (itemId, herbAge) 汇总后不会超过背包里那一堆的现有数量。
    std::vector<Ingredient> consumed;
    std::string log;                    // 可直接显示给玩家的一行说明
};

// 失败时的材料损耗策略。四艺的性格差异就落在这张表上。
struct FailurePolicy {
    int lossPercent = 100;         // 每味材料按配方需求量损耗的百分比，向上取整
    bool sparesCatalyst = false;   // 是否保住「引」
    friend bool operator==(const FailurePolicy&, const FailurePolicy&) = default;
};

// ---- 成功率参数 ----

// 永远留 5% 失手与 5% 侥幸：全成功或全失败都会让玩法失去张力。
inline constexpr int kMinSuccessChance = 5;
inline constexpr int kMaxSuccessChance = 95;

// 底数。没有它，零熟练度的新手连最简单的丹方都只能靠 5% 的侥幸开张，
// 而熟练度又只能靠开工来涨——那不是难度，是死锁。
inline constexpr int kBaseSuccessChance = 40;

// 炉鼎每品阶折多少成功率。品阶量程 0-5，故上限 25 点，与资质（0-20 点）同量级：
// 换炉和换人资质带来的提升应当彼此可比，否则装备线会压掉养成线。
inline constexpr int kToolGradeWeight = 5;

// 三项输入的设计量程。上下界都要夹，只夹下界是不对称的防御：
// 若脏数据把熟练度或炉鼎品阶灌到接近 INT_MAX，`底数 + 熟练/2 + 资质/5 + 品阶×5`
// 这串乘加会先溢出，std::clamp 拿到的已经是垃圾（甚至是负数），反倒跳出 [5, 95]，
// 而那正是本模块的硬要求。夹上界同时把「炉鼎品阶 0-5」这条设定写进了代码：
// 没有上界，一件品阶被写坏的炉鼎能把最难的方子直接顶到 95%。
inline constexpr int kMaxProficiency = 100;
inline constexpr int kMaxAptitude = 100;
inline constexpr int kMaxToolGrade = 5;

// ---- 熟练度成长 ----

// 失败也涨，但涨得慢：一炉炸了，火候是记住了；只是不能让玩家靠故意炸炉刷熟练。
inline constexpr int kProficiencyGainOnSuccess = 3;
inline constexpr int kProficiencyGainOnFailure = 1;

// ---- 查询 ----

// 技艺名，UTF-8，可直接上屏。
[[nodiscard]] std::string_view kindName(CraftKind kind) noexcept;

// 该技艺缺炉鼎时的提示语。四艺缺的不是同一件东西，提示语也不该是同一句。
[[nodiscard]] std::string_view toolMissingMessage(CraftKind kind) noexcept;

[[nodiscard]] FailurePolicy failurePolicyOf(CraftKind kind) noexcept;

// 成功率 = 底数 + 熟练度/2 + 资质/5 + 炉鼎品阶×5 − 配方难度，夹在 [5, 95]。
// 熟练度折半计入：它的量程与难度一样大，若 1:1 计入，熟练度一满就把资质与炉鼎
// 挤成装饰品，三条养成线塌成一条。
// 四个入参先各自夹到设计量程内再参与运算，任何整数入参（含 INT_MIN / INT_MAX）
// 都不会让返回值跑出 [5, 95]。
[[nodiscard]] int successChance(const Recipe& recipe, int proficiency, int aptitude,
                                int toolGrade) noexcept;

// 本次炼制涨多少熟练度。难方子教得多：一万炉金疮药也炼不出丹道大师。
[[nodiscard]] int proficiencyGain(const Recipe& recipe, bool success) noexcept;

// craft 不另收 hasTool 参数，「有没有炉鼎」的桥就架在这里：品阶 0 即无炉。
// 两处判断共用同一个谓词，canCraft 与 craft 才不可能对同一局面给出不同答案。
[[nodiscard]] constexpr bool hasToolOfGrade(int toolGrade) noexcept { return toolGrade > 0; }

// 这张方子到手了没有（契约 docs/interfaces-p3-ch07.md 第 1 节）：没有门闸（requireFlag 为空），
// 或者那面旗标已置。炼制面板列不列、开不开得了工，只问它这一个函数。
[[nodiscard]] bool recipeKnown(const Recipe& recipe, const core::GameState& state);

// ---- 判定 ----

// 能否开工：配方是否完整、到手了没有、有没有炉鼎、熟练度够不够、材料够不够、年份够不够。
// 返回失败原因（可直接显示），成功时 value 为空串。
// 检查顺序：配方 → 已得 → 炉鼎 → 熟练度 → 材料 → 年份。按「在故事里先卡住哪一步」排：
// 方子都还没到手，谈不上生火；没炉子连火都生不起来，其次才轮到手艺和料。
// 这个顺序是对外承诺，有测试锁住。
//
// 材料一步按 itemId 聚合全部需求后再与背包总量比；年份一步按门槛从高到低逐条
// 认领，认领过的堆不再回到池子里。两步都不可省：
//   ·不聚合，「十年草 2 株 + 百年草 1 株」会把同一批堆认领两遍，材料不够也能开工；
//   ·不按门槛降序认领，低门槛那条会先把百年的堆挑走，高门槛那条假性失败。
[[nodiscard]] core::Result<std::string> canCraft(const Recipe& recipe,
                                                 const core::GameState& state,
                                                 int proficiency, bool hasTool);

// 执行炼制。canCraft 说不行的，这里一动不动：不摇骰、不扣料、连熟练度也不给
// （「没开工」和「开工失败」是两回事，只有后者该长经验）。
[[nodiscard]] CraftResult craft(const Recipe& recipe, const core::GameState& state,
                                int proficiency, int aptitude, int toolGrade,
                                std::uint32_t seed);

// 把 craft 的消耗清单落到背包上。
//
// 这件事没有交给调用方自己写循环，是因为 GameState::removeItem 不认年份——
// 它一律从年份最低的堆扣，拿它扣一张「需百年以上」的方子会把玩家的十年草扣掉，
// 而百年草原封不动留在包里。年份账必须由开出清单的人来平。
void applyConsumption(core::GameState& state, const std::vector<Ingredient>& consumed);

}  // namespace fanren::rules
