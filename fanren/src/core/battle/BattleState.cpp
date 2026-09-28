// 战斗的生命周期、回合推进与行动序。动作合法性与结算在 BattleAction.cpp，
// AI 在 BattleAi.cpp——三者都是 BattleState 的成员，只是按关注点分文件。
#include "core/battle/Battle.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>

namespace fanren::core::battle {
namespace {

// 行动序的排序键要乘上「第几次行动」的分数 (slots − k) / slots。取 1..10 的最小公倍数
// 当公分母，于是十次行动以内的首领每一位都是整数，不引入浮点：浮点比较在边界上
// 左右横跳，而行动序一旦因为最后一位小数翻了个，回放就分叉了。
constexpr std::int64_t kOrderScale = 2520;

}  // namespace

void BattleState::setup(std::vector<Unit> units, std::uint32_t seed) {
    units_ = std::move(units);
    magics_.clear();
    items_.clear();
    order_.clear();
    orderPos_ = 0;
    slotUsed_ = false;
    log_.clear();
    events_.clear();
    lastActionBegin_ = 0;
    lastActionEnd_ = 0;
    round_ = 0;
    phase_ = BattlePhase::Ongoing;
    canEscape_ = true;
    devour_ = false;
    anyEnemyFled_ = false;
    spoilsPercent_ = 0;
    // 开战体积的底数。逃遁阈值与战果都相对它算，所以要在这里一次性留档。
    startMaxHp_.clear();
    startMaxHp_.reserve(units_.size());
    for (const Unit& u : units_) startMaxHp_.push_back(u.maxHp);

    // ---- 波次：onField 一律**由 wave 推出来**，不信调用方传进来的那一位 ----
    currentWave_ = 0;
    maxWave_ = 0;
    for (Unit& u : units_) {
        u.wave = std::max(0, u.wave);
        maxWave_ = std::max(maxWave_, u.wave);
        u.onField = u.wave <= currentWave_;
    }

    // ---- 破势与蓄劲的开局值：同样由编成推出来，不信调用方传进来的局面 ----
    // 架势开场是满的；劲我方 1 点、敌方 0；破势、守势、蓄势一概没有。已揭开的破绽
    // 只收 weaknesses 里真有的那几样（调用方从存档里读进来的可能比这个单位多）。
    for (Unit& u : units_) {
        u.acted = false;
        u.maxToughness = std::max(0, u.maxToughness);
        u.toughness = u.maxToughness;
        u.revealed &= u.weaknesses;
        u.tested |= u.revealed;
        u.breakRecoverRound = 0;
        u.bp = u.ally ? kBpStart : 0;
        u.boosted = false;
        u.guarding = false;
        u.priority = false;
        u.actions = std::max(1, u.actions);
        u.charging = false;
        u.roundsActed = 0;
    }

    // 种子是本模块唯一的随机来源，存档回放靠它复现。
    seed_ = seed;
    rng_.seed(seed);

    refreshPhase();   // 允许传入一开始就一边全灭的编队（脚本出错时不至于卡死）
    beginRound();
}

void BattleState::addMagic(const Magic& magic) { magics_[magic.id] = magic; }

void BattleState::addItem(const Item& item) { items_[item.id] = item; }

void BattleState::setCanEscape(bool canEscape) { canEscape_ = canEscape; }

void BattleState::setDevourMode(bool on) {
    devour_ = on;
    if (!on) return;
    // 梦里没有架势、破绽与劲。清成 0 而不是在每一处结算里各判一次「是不是吞噬模式」：
    // 结构上没有，就不可能在哪一处漏判——架势为 0 的单位永远不会破势，
    // 没揭开过任何破绽的单位也就没有东西可以写回存档。
    for (Unit& u : units_) {
        u.bp = 0;
        u.boosted = false;
        u.maxToughness = 0;
        u.toughness = 0;
        u.weaknesses = 0;
        u.revealed = 0;
        u.tested = 0;
        u.breakRecoverRound = 0;
        u.chargeEvery = 0;
        u.charging = false;
    }
}

std::vector<std::string> BattleState::registeredMagicIds() const {
    std::vector<std::string> ids;
    ids.reserve(magics_.size());
    for (const auto& entry : magics_) ids.push_back(entry.first);
    return ids;
}

std::vector<std::string> BattleState::registeredItemIds() const {
    std::vector<std::string> ids;
    ids.reserve(items_.size());
    for (const auto& entry : items_) ids.push_back(entry.first);
    return ids;
}

const Magic* BattleState::findMagic(const std::string& id) const {
    const auto it = magics_.find(id);
    return it == magics_.end() ? nullptr : &it->second;
}

const Item* BattleState::findItem(const std::string& id) const {
    const auto it = items_.find(id);
    return it == items_.end() ? nullptr : &it->second;
}

bool BattleState::validIndex(int index) const {
    return index >= 0 && static_cast<std::size_t>(index) < units_.size();
}

int BattleState::currentActor() const {
    if (phase_ != BattlePhase::Ongoing) return -1;
    if (orderPos_ >= order_.size()) return -1;
    return order_[orderPos_];
}

std::vector<BattleEvent> BattleState::lastActionEvents() const {
    const std::size_t end = std::min(lastActionEnd_, events_.size());
    const std::size_t begin = std::min(lastActionBegin_, end);
    return std::vector<BattleEvent>(events_.begin() + static_cast<std::ptrdiff_t>(begin),
                                    events_.begin() + static_cast<std::ptrdiff_t>(end));
}

// ---------------------------------------------------------------------------
// 行动序
// ---------------------------------------------------------------------------

std::uint64_t BattleState::mixBits(std::uint64_t value) noexcept {
    value += 0x9E3779B97F4A7C15ULL;
    value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31U);
}

int BattleState::speedJitter(int roundNumber, int unitIndex, int slot) const {
    std::uint64_t h = mixBits(seed_);
    h = mixBits(h ^ static_cast<std::uint64_t>(static_cast<std::uint32_t>(roundNumber)));
    h = mixBits(h ^ (static_cast<std::uint64_t>(static_cast<std::uint32_t>(unitIndex)) << 16U) ^
                static_cast<std::uint64_t>(static_cast<std::uint32_t>(slot)));
    constexpr std::uint64_t kSpan = 2 * kSpeedJitterPermille + 1;
    return static_cast<int>(h % kSpan) - kSpeedJitterPermille;
}

std::vector<int> BattleState::computeOrder(int roundNumber) const {
    // 一位 = 某个单位的第几次行动。首领 actions = 2 就占两位：第一位按全速排，
    // 第二位按半速排——两次行动于是散在这一回合的前后两头，而不是挨着连出两手
    //（挨着的话，玩家夹在中间打断蓄势的机会就没了）。
    struct Entry {
        int unit = 0;
        int slot = 0;
        bool first = false;       // 防御给的「下回合先手」
        std::int64_t key = 0;
    };
    std::vector<Entry> entries;
    for (std::size_t i = 0; i < units_.size(); ++i) {
        const Unit& u = units_[i];
        if (!u.alive()) continue;
        // 那一回合开始时仍在破势里的，整回合没有它的位置。恢复的那一回合（等号）照常排。
        if (u.breakRecoverRound > roundNumber) continue;
        const int slots = std::max(1, u.actions);
        for (int k = 0; k < slots; ++k) {
            const auto index = static_cast<int>(i);
            const std::int64_t jitter = 1000 + speedJitter(roundNumber, index, k);
            const std::int64_t key =
                static_cast<std::int64_t>(std::max(0, u.speed)) * jitter * (slots - k) * kOrderScale /
                slots;
            entries.push_back(Entry{index, k, u.priority && k == 0, key});
        }
    }
    // 全序：先手标记、排序键、单位下标、第几次行动。扰动之后同速几乎不再相等，
    // 真相等时按下标——回放不能因为排序不稳而分叉。
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        if (a.first != b.first) return a.first;
        if (a.key != b.key) return a.key > b.key;
        if (a.unit != b.unit) return a.unit < b.unit;
        return a.slot < b.slot;
    });
    std::vector<int> order;
    order.reserve(entries.size());
    for (const Entry& e : entries) order.push_back(e.unit);
    return order;
}

std::vector<int> BattleState::nextRoundOrder() const {
    if (phase_ != BattlePhase::Ongoing) return {};
    return computeOrder(round_ + 1);
}

void BattleState::beginRound() {
    ++round_;
    order_.clear();
    orderPos_ = 0;
    slotUsed_ = false;
    if (phase_ != BattlePhase::Ongoing) return;

    emit(BattleEvent{BattleEventKind::RoundStart, -1, -1, round_});

    for (std::size_t i = 0; i < units_.size(); ++i) {
        Unit& u = units_[i];
        u.acted = false;
        if (!u.alive()) continue;
        const auto index = static_cast<int>(i);

        // 破势到期：架势回满，这一回合照常出手（computeOrder 用的是同一个判据）。
        if (u.breakRecoverRound != 0 && u.breakRecoverRound <= round_) {
            u.breakRecoverRound = 0;
            u.toughness = u.maxToughness;
            log_.push_back(u.name + " 重整架势（架势 " + std::to_string(u.toughness) + "）");
            BattleEvent recover{BattleEventKind::Recover, -1, index};
            recover.toughness = u.toughness;
            recover.hp = u.hp;
            emit(std::move(recover));
        }

        // 中毒是「这一回合开始时身上发生的事」。毒不致死，所以结算完他仍站着。
        tickPoison(index);

        // 劲：每回合开始 +1，上限 5；上一回合蓄过劲的这一回合不加。第一回合的那一点
        // 是 setup 给的开场劲，这里不再加。
        if (u.ally && !devour_ && round_ > 1) {
            if (u.boosted) {
                u.boosted = false;
            } else {
                u.bp = std::min(kBpMax, u.bp + 1);
            }
        }
    }

    order_ = computeOrder(round_);
    // 「下回合先手」用掉了。放在排序之后清：排序正要读它。
    for (Unit& u : units_) u.priority = false;
    skipIneligibleActors();
    beginActorTurn();
}

void BattleState::skipIneligibleActors() {
    while (orderPos_ < order_.size()) {
        const Unit& u = unitRef(order_[orderPos_]);
        if (u.alive() && !u.broken()) break;
        // 这一回合里刚被打倒、刚被打破势的，排到它时一律跳过：破势的那一回合
        //「若还没出手就失去本回合」说的就是这里。
        ++orderPos_;
    }
}

void BattleState::beginActorTurn() {
    const int actor = currentActor();
    if (actor < 0) return;
    Unit& u = unitRef(actor);
    if (!u.guarding) return;
    // 日志上说一句：守势是玩家花掉一整手换来的，它什么时候没的得看得见。
    u.guarding = false;
    log_.push_back(u.name + " 收起守势");
    emit(BattleEvent{BattleEventKind::GuardEnd, actor});
}

void BattleState::tickPoison(int unitIndex) {
    Unit& unit = unitRef(unitIndex);
    if (unit.poison <= 0) {
        // 回合数归零时把强度一并清干净，不留一个「毒 0 回合、每回合 6」的残影。
        unit.poisonPower = 0;
        return;
    }

    // 毒不走防御减半：守势挡的是外力，毒已经在体内了。反过来的话，「防御一下就不用
    // 解毒」会让解毒药与整条备毒线一起失去意义。
    //
    // **毒不致死，最低留 1 点气血。** 三条理由：
    //   1. 第 3 章的毒是韩立拿来对付墨居仁的手段，而墨居仁必须死在识海反噬上
    //      （大纲硬约束第 6 条）。毒能直接毒死，那一章的高潮就能被一瓶药绕过去。
    //   2. 回合开始时无人行动，此刻判死会让胜负在日志上找不到凶手。
    //   3. 这条口径能被强断言钉住：中毒的单位挨上一百回合，气血必须恒为 1。
    const int dealt = std::max(0, std::min(unit.poisonPower, unit.hp - kPoisonFloorHp));
    unit.hp -= dealt;
    --unit.poison;

    std::string line = unit.name + " 毒发，气血 -" + std::to_string(dealt);
    if (unit.poison > 0) {
        line += "（毒性尚余 " + std::to_string(unit.poison) + " 回合）";
    } else {
        unit.poisonPower = 0;
        line += "，毒性就此散尽";
    }
    log_.push_back(std::move(line));
    BattleEvent tick{BattleEventKind::PoisonTick, -1, unitIndex, dealt, unit.poison};
    tick.hp = unit.hp;
    tick.toughness = unit.toughness;
    emit(std::move(tick));
}

void BattleState::endTurn() {
    if (phase_ != BattlePhase::Ongoing) return;
    if (orderPos_ >= order_.size()) {
        // 上一次 endTurn 已把 currentActor 推到 -1（回合结束），这次才开新回合。
        beginRound();
        return;
    }
    ++orderPos_;
    slotUsed_ = false;
    skipIneligibleActors();
    beginActorTurn();
}

// ---------------------------------------------------------------------------
// 波次与胜负
// ---------------------------------------------------------------------------

void BattleState::deployNextWave() {
    if (!hasPendingWave()) return;
    ++currentWave_;

    std::vector<std::string> names;
    for (Unit& u : units_) {
        if (u.wave != currentWave_ || u.onField) continue;
        // 还没进场就已经没了（编成把 hp 写成 0）的不放上场：放上去只会多一具尸体。
        if (u.hp <= 0 || u.fled) continue;
        u.onField = true;
        // 刚走上战场的这一回合不该还带着「已行动」。行动序是回合开始时排的，
        // 中途进场的排不进去，下一回合才轮到——他们刚走上战场。
        u.acted = false;
        names.push_back(u.name);
    }

    // 界面上必须说得出来（契约第 2.3 节）：玩家正打着忽然多出六个敌人而一个字也不提，
    // 是本项目明令禁止的那类观感。所以这一句日志是实现的一部分，不是装饰。
    std::string line = "第 " + std::to_string(currentWave_ + 1) + " 波杀到";
    if (names.empty()) {
        line += "：却不见人影";
    } else {
        line += "：";
        for (std::size_t i = 0; i < names.size(); ++i) {
            if (i > 0) line += "、";
            line += names[i];
        }
    }
    log_.push_back(std::move(line));
    emit(BattleEvent{BattleEventKind::WaveIn, -1, -1, currentWave_});
}

void BattleState::refreshPhase() {
    if (phase_ != BattlePhase::Ongoing) return;   // Escaped 之类的终局不被覆盖
    const auto sides = [this](bool& allyAlive, bool& foeAlive) {
        allyAlive = false;
        foeAlive = false;
        for (const auto& u : units_) {
            if (!u.alive()) continue;
            (u.ally ? allyAlive : foeAlive) = true;
        }
    };
    bool allyAlive = false;
    bool foeAlive = false;
    sides(allyAlive, foeAlive);

    // 推波排在胜负判定**之前**：顺序反过来，最后一个敌人倒下的那一刻就已经判成 Won。
    // 写成 while：一波若一个人也没有，场上仍然没有敌人，要继续往下推。有 maxWave_ 封顶。
    while (!foeAlive && hasPendingWave()) {
        deployNextWave();
        sides(allyAlive, foeAlive);
    }

    if (!foeAlive) {
        // 场上一个敌人都不剩：有人是逃走的（识海第二场），落点就是 EnemyFled，不是 Won。
        endBattle(anyEnemyFled_ ? BattlePhase::EnemyFled : BattlePhase::Won);
    } else if (!allyAlive) {
        endBattle(BattlePhase::Lost);
    }
}

void BattleState::endBattle(BattlePhase outcome) {
    phase_ = outcome;
    emit(BattleEvent{BattleEventKind::End, -1, -1, static_cast<int>(outcome)});
}

int BattleState::escapeChance(const Unit& runner) const {
    double own = 0.0;
    double foe = 0.0;
    int ownCount = 0;
    int foeCount = 0;
    for (const auto& u : units_) {
        if (!u.alive()) continue;
        if (u.ally == runner.ally) {
            own += u.speed;
            ++ownCount;
        } else {
            foe += u.speed;
            ++foeCount;
        }
    }
    own /= std::max(1, ownCount);
    foe /= std::max(1, foeCount);
    // 身法差每点折 2%，夹在 [15, 90]：再慢也有一线生机，再快也跑不脱十成。
    const double chance = kEscapeBaseChance + (own - foe) * 2.0;
    return std::clamp(static_cast<int>(std::lround(chance)), kEscapeMinChance, kEscapeMaxChance);
}

}  // namespace fanren::core::battle
