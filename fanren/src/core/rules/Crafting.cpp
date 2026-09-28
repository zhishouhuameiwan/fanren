#include "core/rules/Crafting.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>

namespace fanren::rules {

namespace {

using core::BagEntry;
using core::GameState;

// 一次炼制内的库存账本：把背包里的堆抄一份，边分配边扣减。
//
// 为什么要抄一份而不是每次去查原背包：一张方子可以对同一味料写多条需求
// （「十年草 2 株 + 百年草 1 株」这种分级配方，正是「灵草按年份分堆」这条设定的
// 用武之地）。若每条需求各自去完整背包里算可用量，同一批堆会被两条需求重复认领
// ——材料不够也能开工，consumed 还会报出多于实际持有的数量，等于白送玩家一份产物。
// 账本让同一堆只能被花掉一次。
class Ledger {
public:
    explicit Ledger(const GameState& state) {
        stacks_.reserve(state.bag.size());
        for (const BagEntry& entry : state.bag) {
            if (entry.count > 0) stacks_.push_back(entry);
        }
        // 年份升序：认领一律从表头拿，就是「优先扣刚好够门槛的那一堆」，
        // 把高年份的留给更难的方子。
        std::stable_sort(stacks_.begin(), stacks_.end(),
                         [](const BagEntry& a, const BagEntry& b) {
                             return a.herbAge < b.herbAge;
                         });
    }

    // 当前还剩多少够门槛的。
    [[nodiscard]] int available(const Ingredient& need) const noexcept {
        int total = 0;
        for (const BagEntry& stack : stacks_) {
            if (stack.itemId == need.itemId && stack.herbAge >= need.minAge) total += stack.count;
        }
        return total;
    }

    // 认领 wanted 个，从年份最低的合格堆开始；返回实际认到的数量（不足时认尽为止）。
    // out 非空时逐堆记账——一味材料可能横跨两堆（十年的只剩一株，余下的从五十年的里补），
    // 所以记出来的条目数可以多于需求条数。
    int claim(const Ingredient& need, int wanted, std::vector<Ingredient>* out) {
        int remaining = wanted;
        for (BagEntry& stack : stacks_) {
            if (remaining <= 0) break;
            if (stack.itemId != need.itemId || stack.herbAge < need.minAge) continue;
            const int got = std::min(stack.count, remaining);
            if (out != nullptr) {
                Ingredient taken;
                taken.itemId = need.itemId;
                taken.count = got;
                taken.minAge = need.minAge;
                taken.herbAge = stack.herbAge;
                taken.catalyst = need.catalyst;
                out->push_back(std::move(taken));
            }
            stack.count -= got;
            remaining -= got;
        }
        return wanted - remaining;
    }

private:
    std::vector<BagEntry> stacks_;
};

// count <= 0 的条目视同不需要这味，各处一律用这个谓词，免得三处判断走岔。
bool isRequired(const Ingredient& need) noexcept { return need.count > 0; }

// 按 itemId 聚合总需求，保持首次出现的顺序——失败提示里报哪一味，
// 必须与方子上的书写顺序一致，不能随容器的哈希顺序漂移。
std::vector<std::pair<std::string, int>> aggregateDemand(const Recipe& recipe) {
    std::vector<std::pair<std::string, int>> demand;
    for (const Ingredient& need : recipe.inputs) {
        if (!isRequired(need)) continue;
        const auto it = std::find_if(demand.begin(), demand.end(),
                                     [&](const auto& row) { return row.first == need.itemId; });
        if (it == demand.end()) {
            demand.emplace_back(need.itemId, need.count);
        } else {
            it->second += need.count;
        }
    }
    return demand;
}

// 认领顺序：门槛由高到低。高门槛那条的可选堆是低门槛那条的子集，先满足受限最紧的
// 一条，才不会出现「低门槛把百年的堆先挑走，高门槛反倒无料可用」的假性失败。
// 稳定排序，门槛相同的仍按方子上的顺序来。
std::vector<std::size_t> claimOrder(const Recipe& recipe) {
    std::vector<std::size_t> order(recipe.inputs.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return recipe.inputs[a].minAge > recipe.inputs[b].minAge;
    });
    return order;
}

// 失败时这一味折损几个。
int failureLossOf(const FailurePolicy& policy, const Ingredient& need) noexcept {
    if (need.catalyst && policy.sparesCatalyst) return 0;
    if (policy.lossPercent <= 0) return 0;
    // 向上取整：三味材料折一半要折两味，不能让取整白送一味回来。
    // 中间量放宽到 long long：配方里的数量来自外部数据，乘 100 不该有溢出的可能。
    const long long scaled = static_cast<long long>(need.count) * policy.lossPercent + 99;
    return static_cast<int>(std::min<long long>(need.count, scaled / 100));
}

// 成功一律全扣；失败按本技艺的策略扣。
std::vector<Ingredient> planConsumption(const Recipe& recipe, const GameState& state,
                                        bool success) {
    const FailurePolicy policy = failurePolicyOf(recipe.kind);
    Ledger ledger(state);

    // 认领按门槛降序走，呈现却要按方子的原顺序——算法的需要不该泄漏到玩家眼前的
    // 消耗清单上，所以先按条分装，最后再按 inputs 顺序拼起来。
    std::vector<std::vector<Ingredient>> perInput(recipe.inputs.size());
    for (std::size_t index : claimOrder(recipe)) {
        const Ingredient& need = recipe.inputs[index];
        if (!isRequired(need)) continue;

        // 整份材料都下了炉，账本按整份扣：失败只折损一部分，但那一部分之外的
        // 也已经被这条需求占用，不能再被同一张方子的下一条需求认领。
        std::vector<Ingredient> drawn;
        drawn.reserve(2);
        ledger.claim(need, need.count, &drawn);

        // drawn 已是年份升序，从头上取损耗额度，折的就是最便宜的那些。
        int budget = success ? need.count : failureLossOf(policy, need);
        for (Ingredient& piece : drawn) {
            if (budget <= 0) break;
            piece.count = std::min(piece.count, budget);
            budget -= piece.count;
            perInput[index].push_back(std::move(piece));
        }
    }

    std::vector<Ingredient> consumed;
    for (std::vector<Ingredient>& group : perInput) {
        for (Ingredient& piece : group) consumed.push_back(std::move(piece));
    }
    return consumed;
}

std::string_view successFlavorOf(CraftKind kind) noexcept {
    switch (kind) {
        case CraftKind::Alchemy:   return "丹炉青烟三转，";
        case CraftKind::Talisman:  return "朱笔一收，";
        case CraftKind::Forge:     return "地火淬过三遍，";
        case CraftKind::Formation: return "阵旗归位，";
    }
    return "";
}

std::string_view failureFlavorOf(CraftKind kind) noexcept {
    switch (kind) {
        case CraftKind::Alchemy:   return "炉中一声炸响，";
        case CraftKind::Talisman:  return "笔锋走偏，";
        case CraftKind::Forge:     return "火候没压住，";
        case CraftKind::Formation: return "地脉没接上，";
    }
    return "";
}

// 失败代价的一句话交代。玩家要在同一行里看懂「亏了什么」，不必翻背包对账。
std::string_view failureCostOf(CraftKind kind) noexcept {
    switch (kind) {
        case CraftKind::Alchemy:   return "一炉药材尽毁。";
        case CraftKind::Talisman:  return "符纸灵墨尽废，妖丹尚在。";
        case CraftKind::Forge:     return "材料折损过半，残料尚可回炉。";
        case CraftKind::Formation: return "阵旗阵盘尽数收回。";
    }
    return "";
}

std::string makeLog(const Recipe& recipe, bool success, int productCount) {
    if (success) {
        return std::string(successFlavorOf(recipe.kind)) + "《" + recipe.name + "》成，得 " +
               std::to_string(productCount) + " 份。";
    }
    return std::string(failureFlavorOf(recipe.kind)) + "《" + recipe.name + "》未成，" +
           std::string(failureCostOf(recipe.kind));
}

}  // namespace

std::string_view kindName(CraftKind kind) noexcept {
    switch (kind) {
        case CraftKind::Alchemy:   return "炼丹";
        case CraftKind::Talisman:  return "制符";
        case CraftKind::Forge:     return "炼器";
        case CraftKind::Formation: return "布阵";
    }
    return "炼制";
}

std::string_view toolMissingMessage(CraftKind kind) noexcept {
    switch (kind) {
        case CraftKind::Alchemy:   return "身边没有丹炉，无从起火。";
        case CraftKind::Talisman:  return "手边没有符笔，画不成符。";
        case CraftKind::Forge:     return "未曾引来地火，炼器无从谈起。";
        case CraftKind::Formation: return "没有阵盘，阵旗无处安放。";
    }
    return "没有称手的器具。";
}

FailurePolicy failurePolicyOf(CraftKind kind) noexcept {
    switch (kind) {
        // 炸炉，药力尽散，连药渣都不剩。炼丹是四艺里最肉疼的一门，
        // 也正因为肉疼，掌天瓶催熟出来的灵草才有分量。
        case CraftKind::Alchemy:   return FailurePolicy{100, false};
        // 妖丹只是引，落笔走偏废的是纸墨。贵的那一味保得住，制符才敢让玩家多试手。
        case CraftKind::Talisman:  return FailurePolicy{100, true};
        // 地火只烧掉半成，残料回炉再炼。炼器的材料本就难得（本命法宝走这条线），
        // 一次失手就清空会把整条法宝线卡死。
        case CraftKind::Forge:     return FailurePolicy{50, false};
        // 布阵是四艺里唯一不改变材料形态的一门：旗插下去没接上灵脉，拔回来还是那面旗。
        // 它的成本在时间与地脉勘察上（由 game 层按天结算），不在材料上。
        case CraftKind::Formation: return FailurePolicy{0, true};
    }
    return FailurePolicy{};
}

int successChance(const Recipe& recipe, int proficiency, int aptitude, int toolGrade) noexcept {
    // 先各自夹到设计量程再参与运算：夹过之后这串乘加的取值范围是 [-60, 135]，
    // 无论入参多离谱都不可能溢出，最终的 [5, 95] 才是结构性成立的。
    const int prof = std::clamp(proficiency, 0, kMaxProficiency);
    const int apt = std::clamp(aptitude, 0, kMaxAptitude);
    const int grade = std::clamp(toolGrade, 0, kMaxToolGrade);
    const int difficulty = std::clamp(recipe.difficulty, 0, 100);
    const int raw = kBaseSuccessChance + prof / 2 + apt / 5 + grade * kToolGradeWeight - difficulty;
    return std::clamp(raw, kMinSuccessChance, kMaxSuccessChance);
}

int proficiencyGain(const Recipe& recipe, bool success) noexcept {
    const int difficulty = std::clamp(recipe.difficulty, 0, 100);
    // 两条斜率都随难度走，但成功那条恒在失败之上（最难时 8 比 3，最易时 3 比 1）：
    // 失败有所得，又绝不划算。
    return success ? kProficiencyGainOnSuccess + difficulty / 20
                   : kProficiencyGainOnFailure + difficulty / 50;
}

core::Result<std::string> canCraft(const Recipe& recipe, const core::GameState& state,
                                   int proficiency, bool hasTool) {
    using Gate = core::Result<std::string>;

    // 用聚合后的需求判「残缺」，而不是只看 inputs 是否为空：一条材料被写成
    // count: 0 的方子等于凭空出产物，与「材料不够也能开工」是同一类经济漏洞。
    const std::vector<std::pair<std::string, int>> demand = aggregateDemand(recipe);
    if (demand.empty() || recipe.productId.empty() || recipe.productCount <= 0) {
        return Gate::failure("这张方子残缺不全，无从下手。");
    }
    if (!hasTool) {
        return Gate::failure(std::string(toolMissingMessage(recipe.kind)));
    }

    const int prof = std::max(0, proficiency);
    if (prof < recipe.requiredProficiency) {
        return Gate::failure(std::string(kindName(recipe.kind)) + "火候未到：此方需熟练度 " +
                             std::to_string(recipe.requiredProficiency) + "，现有 " +
                             std::to_string(prof) + "。");
    }

    // 先比总量：同一味料的多条需求加总了再比，报的「现有」也是玩家翻背包能对上的数。
    for (const auto& [itemId, wanted] : demand) {
        const int owned = state.itemCount(itemId);
        if (owned < wanted) {
            return Gate::failure("材料不足：" + itemId + " 需 " + std::to_string(wanted) +
                                 "，现有 " + std::to_string(owned) + "。");
        }
    }

    // 总量够了才轮到年份：按门槛从高到低认领，认领过的堆不再回到池子里。
    Ledger ledger(state);
    for (std::size_t index : claimOrder(recipe)) {
        const Ingredient& need = recipe.inputs[index];
        if (!isRequired(need)) continue;
        const int usable = ledger.available(need);
        if (ledger.claim(need, need.count, nullptr) < need.count) {
            return Gate::failure("灵草年份不足：" + need.itemId + " 需 " +
                                 std::to_string(need.minAge) + " 年以上 " +
                                 std::to_string(need.count) + " 株，够年份的只有 " +
                                 std::to_string(usable) + " 株。");
        }
    }

    return Gate::success(std::string{});
}

CraftResult craft(const Recipe& recipe, const core::GameState& state, int proficiency,
                  int aptitude, int toolGrade, std::uint32_t seed) {
    CraftResult result;

    const core::Result<std::string> gate =
        canCraft(recipe, state, proficiency, hasToolOfGrade(toolGrade));
    if (!gate.ok) {
        // 一动不动：consumed 空、产物为 0、熟练度不涨，只把拒绝的理由原样带出去。
        result.log = gate.error;
        return result;
    }

    const int chance = successChance(recipe, proficiency, aptitude, toolGrade);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d100(1, 100);
    result.success = d100(rng) <= chance;

    result.consumed = planConsumption(recipe, state, result.success);
    result.proficiencyGain = proficiencyGain(recipe, result.success);
    if (result.success) {
        result.productId = recipe.productId;
        result.productCount = recipe.productCount;
    }
    result.log = makeLog(recipe, result.success, result.productCount);
    return result;
}

void applyConsumption(core::GameState& state, const std::vector<Ingredient>& consumed) {
    for (const Ingredient& gone : consumed) {
        int remaining = gone.count;
        for (BagEntry& entry : state.bag) {
            if (remaining <= 0) break;
            // 认年份：清单是 craft 按同一个背包开出来的，这里按 (id, 年份) 精确销账。
            if (entry.itemId != gone.itemId || entry.herbAge != gone.herbAge) continue;
            const int take = std::min(entry.count, remaining);
            entry.count -= take;
            remaining -= take;
        }
    }
    // 空堆就地清掉，但不重排背包：玩家的物品顺序不该因为炼了一炉丹而洗牌。
    state.bag.erase(std::remove_if(state.bag.begin(), state.bag.end(),
                                   [](const BagEntry& entry) { return entry.count <= 0; }),
                    state.bag.end());
}

}  // namespace fanren::rules
