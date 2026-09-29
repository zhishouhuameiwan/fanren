#pragma once
// 横版回合制「破势与蓄劲」的规则核心。完整规则与数字见 docs/octopath-battle.md。
//
// core 层硬约束：不含 SDL / JSON / Lua / 文件 IO。随机数只有 setup() 传入的种子
// 这一个来源，因此「同种子 + 同动作序列」必然复现同一局；无头 bot 的回归断言与
// 战斗回放都建立在这个性质上，任何新增的随机来源都会悄悄破坏它。
//
// 与前一版（战棋）的关系：格子、移动、射程、相邻、寻路、阻挡整个拿掉了；伤害公式
// （Damage.h）、毒、波次、逃跑、识海吞噬、登记表、合法性单入口这几样原样沿用。
// 状态迁移一律通过 apply() 发生，绘制层只读 units() / log() / events()。
#include <cstddef>
#include <cstdint>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "core/Result.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"

namespace fanren::core::battle {

// 数值常量集中在此：调平衡时只动这里，不必翻结算代码。
inline constexpr int kEscapeBaseChance = 60;   // 逃跑基础成功率
inline constexpr int kEscapeMinChance = 15;
inline constexpr int kEscapeMaxChance = 90;

// ---- 劲（BP，docs/octopath-battle.md 2.3）----
// 我方每人开场 1 点；每回合开始 +1，上限 5；上一回合蓄过劲的人这一回合开始不加。
// 出手时蓄 0–3 点。敌方没有劲。
inline constexpr int kBpStart = 1;
inline constexpr int kBpMax = 5;
inline constexpr int kBoostMax = 3;

// ---- 破势（docs/octopath-battle.md 2.4）----
// 架势归零即破势：失去本回合（若还没出手）与下一整回合，到「再下一回合」开始时恢复、
// 架势回满。所以恢复的回合号 = 破势那一回合 + 2。破势期间受到的伤害 ×2。
inline constexpr int kBreakRecoverAfterRounds = 2;
inline constexpr int kBrokenDamagePercent = 200;
// 防御：从这一刻起到自己下一次出手之前，受伤减半；下一回合排在最前。
inline constexpr int kGuardDamagePercent = 50;

// ---- 行动序（docs/octopath-battle.md 2.1）----
// 身法乘一个确定性的小扰动：±10%（千分数），由种子、回合、单位下标、第几次行动派生，
// 不耗 rng_——于是「下一回合的行动序」现在就算得出来，预览与真用的是同一个函数。
inline constexpr int kSpeedJitterPermille = 100;

// ---- 识海之战：吞噬（P3 第 3 章，契约 docs/interfaces-p3-ch03.md 第 3 节）----
//
// 体积即实力，直接映射到 maxHp：咬下多少，攻方就长多少、守方就少多少。
// 不新起一套数值，第一场「靠体积轻易吞掉」与第二场「咬下三分之一」用的是同一条规则。
// 吞噬模式下没有架势、破绽与劲（setDevourMode 把它们清成 0）：那是梦里，
// 玩家刚学会的那一套在这里全不管用，这种失重感正是这场戏要的。
//
// 中毒不致死这一条的最低气血。毒只削不杀，理由写在 BattleState::tickPoison。
inline constexpr int kPoisonFloorHp = 1;

// 咬一口的深度 = 攻方体积 / 16。为什么是 16：
//   * 韩立入梦时体积 kDevourPlayerVolume(480) ÷ 16 = 30，正好是黄光球体积
//     （data/roles/mo_juren_yuanshen.json 的 120）的四分之一 —— 四口吞掉一个
//     「比自己小好几倍」的对手，对上原著 ch56 那句「很快就结束了」。
//   * 同时保证第二场的绿光球在逃走前至少挨六口。每一口都要玩家操作一次：
//     再稀一点（如 /8）它两三口就跑了，玩家来不及意识到自己在吞噬；
//     再密一点（如 /32）就成了重复点击十几次的磨蹭。
inline constexpr int kDevourBiteDivisor = 16;

// 识海里韩立元神的体积。刻意**不取存档里的 maxHp**：识海里比的是元神的大小，
// 与肉身气血无关。用肉身血量当体积，等于让「入梦前吃没吃丹药」决定梦里谁大谁小，
// 而原著那三条体积关系（小好几倍 / 大一圈 / 少三分之一）是剧情硬约束，不能随存档漂。
inline constexpr int kDevourPlayerVolume = 480;

// 敌方逃遁阈值：体积被咬掉三分之一就跑。这个数不是平衡旋钮，它就是原著的战果
// ——绿光球「每被咬住就把那块脱开继续跑」，最终逃脱时体积少了三分之一。
// 分数写成两个整数而不是 0.667，是为了让「少三分之一」在代码里一眼可认。
inline constexpr int kFleeBittenNumerator = 1;
inline constexpr int kFleeBittenDenominator = 3;

struct Unit {
    std::string id, name;
    int hp{}, maxHp{}, mp{}, maxMp{};
    int attack{}, defence{}, speed{};
    rules::Realm realm{};
    int element{};          // 五行 bitmask
    bool ally = false;
    // 本回合出过手（至少一次）。「非法动作不消耗回合」那一组断言看的就是它。
    bool acted = false;

    // 已习得法术。留空的语义有两种，由 magicsExhaustive 挑：缺省是「战场登记的
    // 法术都能用」（方便杂兵与单测少写一行），置位后是「一门都不会」。
    // game 层建场时一律置位，见那里的注释。
    std::vector<std::string> magics;

    // ---- 用毒（P3 第 3 章，契约第 2.1 节）----
    // 两个 int，不做抗性、不做毒的种类。
    //
    // **尸虫丸不是这里的毒。** 它是剧情状态（旗标 `ch03.shichong_wan` 加一个
    // 期限），不在战斗里结算、也解不掉。后人不要「顺手统一」这两样。
    int poison = 0;          // 剩余回合数
    int poisonPower = 0;     // 每回合掉血

    // 识海之战：这一团光球已经脱身逃走了。不复用 hp = 0：那是「倒地不起」，会让本场
    // 判成 Won，而逃走与被吞掉的后续剧情完全不同（见 BattlePhase::EnemyFled）。
    bool fled = false;

    // magics 那份清单是不是**穷尽**的。game 层建场时一律置 true：data/roles 里那份
    // magics 就是这个角色会什么的全部答案。不置的话，同场只要有人挂着一条法术，
    // 所有 magics 留空又有法力的单位都跟着会了。
    bool magicsExhaustive = false;

    // ---- 多波次（P3 第 4 章，契约 docs/interfaces-p3-ch04.md 第 2 节）----
    // wave 是编成（只读的意图），onField 是局面（会变的事实）。setup() 按
    // `onField = (wave <= 0)` 初始化，之后只有 BattleState 改 onField。
    int wave = 0;
    bool onField = true;

    // ---- 破势与蓄劲（docs/octopath-battle.md）----
    // 普攻可用的兵刃类别（位掩码，core::AttackCategory）。0 = 没有类别：打得出伤害，
    // 但打不中任何破绽。game 层按 data/roles 的 weapons 建敌人（缺省拳），所以敌方的普攻
    // 也带类别——只是我方没有架势与破绽，打不出什么来。
    int weapons = 0;
    int toughness = 0;        // 架势，当前
    int maxToughness = 0;     // 架势上限；0 = 没有架势（我方、吞噬战），永远不会破势
    int weaknesses = 0;       // 破绽
    int revealed = 0;         // 已揭开的破绽（⊆ weaknesses）
    // 打中过它的类别（⊇ revealed）。给我方 AI 探破绽用：一样兵刃砍过了没揭开什么，
    // 就不必再拿它去试。不跨战斗、不上存档——跨战斗记住的只有「揭开了什么」。
    int tested = 0;
    // 破势：到这一回合**开始时**恢复、架势回满。0 = 没有破势。
    int breakRecoverRound = 0;
    int bp = 0;               // 劲
    bool boosted = false;     // 这一回合蓄过劲：下一回合开始不加劲
    bool guarding = false;    // 防御中：受伤减半，护到自己下一次出手
    bool priority = false;    // 下一回合排最前（防御给的）
    int actions = 1;          // 每回合行动次数（首领可以不止一次）
    // 只有这几类要得了他的命（位掩码）；0 = 什么都要得了。别的类别打得再重，也只能把他
    // 压到 1——欧阳飞天的霸王甲：火弹能把他逼得狼狈，取首级只有祭剑符（金）那一下
    //（第 5 章校对 MEDIUM-1 的拍板，docs/octopath-battle.md 2.4）。
    int killableBy = 0;

    // ---- 首领的蓄势（docs/octopath-battle.md 2.6）----
    int chargeEvery = 0;      // 0 = 不会蓄势
    double chargeMult = 1.0;
    bool chargeAll = false;
    std::string chargeTextKey;
    bool charging = false;    // 已宣告，下一手是重招
    int roundsActed = 0;      // 出过手的回合数：蓄势的节奏按它数

    // 「在场且还站得住」。**不在场的单位一律当作不在**：行动序、选中、可否被打、
    // 胜负判定全靠这一条，分成两个谓词就必然有哪一处只问了其中一个。
    [[nodiscard]] bool alive() const { return hp > 0 && !fled && onField; }
    [[nodiscard]] bool broken() const { return breakRecoverRound > 0; }
};

// Charge 追加在末尾（首领宣告蓄势，只有 AI 用得上）。Move 随格子一起删掉。
enum class ActionKind { Attack, Cast, Item, Defend, Escape, Charge };

struct Action {
    ActionKind kind{};
    int actorIndex = -1;
    int targetIndex = -1;
    std::string magicId;     // Cast 用；Item 复用本字段传物品 id（契约未给 itemId）
    // Attack 用：出哪一样兵刃（单个类别位）。0 = 这个单位的第一样（位序最前的那一位）。
    int category = 0;
    // 蓄几点劲（0–3，不超过现有）。只有 Attack 与 Cast 蓄得了。
    int boost = 0;
};

// Escaped 是**玩家**跑了，EnemyFled 是**敌人**跑了，两者的后续剧情完全不同。
//
// 所有 switch (phase) 都要补上新分支，且**不要加 default:**——加了就消掉
// 「新增枚举项忘了处理」的 /W4 告警（C4061/C4062），而那正是这里要靠的东西。
enum class BattlePhase { Ongoing, Won, Lost, Escaped, EnemyFled };

// ---- 事件流（docs/octopath-battle.md 第 4 节）----
//
// 日志是给人读的一句话，事件是给画面路逐条播动画的结构化记录。两者都从 apply()
// 与回合推进里产生，**同一处代码同时写两份**，所以不会走散。
// 数值字段一律是「这件事发生之后」的值：画面按事件逐条更新自己显示的那一份，
// 播完最后一条就与规则层的局面一致。
enum class BattleEventKind {
    RoundStart,       // value = 回合数
    Act,              // actor 出手；value = ActionKind；text = 这一手的说法（日志那一句的开头）
    BoostSpent,       // actor 蓄劲；value = 点数
    Hit,              // actor 打中 target 一击：见下面各字段
    Break,            // target 破势；value = 还要失去几个回合（含本回合）
    ChargeInterrupt,  // target 的蓄势被打断（因为破势）
    Recover,          // target 恢复架势；toughness = 回满后的架势
    Poisoned,         // target 中毒；value = 回合，value2 = 每回合
    PoisonTick,       // target 毒发；value = 掉的气血，value2 = 余下回合
    Cured,            // target 毒解了
    Heal,             // target 回复；value = 气血，value2 = 法力
    Guard,            // actor 摆开守势
    GuardEnd,         // actor 轮到出手，守势散去
    Fall,             // target 倒地不起
    Fled,             // target 脱开被咬住的那块遁走（识海）
    Escape,           // actor 逃跑；value = 1 成功 / 0 失败
    WaveIn,           // value = 进场的是第几波（0 起算）
    ChargeDeclare,    // actor 宣告蓄势；text = 预告句的文案 key
    End,              // 战斗结束；value = BattlePhase
    // actor 施展看破（天眼术，Magic::effect == Reveal）揭开 target 的破绽。追加在末尾。
    // 本场每一个站着的敌人各一条，紧跟在那一手的 Act 之后；revealed = 这一下新揭开的，
    // value = 揭开之后它的全部已揭开（= 它的全部破绽）。不是一击：没有 Hit，不伤人、不削架势。
    Reveal,
};

struct BattleEvent {
    BattleEventKind kind{};
    int actor = -1;
    int target = -1;
    int value = 0;
    int value2 = 0;
    // ---- Hit 专用 ----
    int category = 0;         // 这一击的类别（位掩码；0 = 没有类别）
    bool weakness = false;    // 打中了破绽
    int revealed = 0;         // 这一击新揭开的破绽
    int hit = 0;              // 第几击（1 起）
    int hits = 0;             // 这一手共几击
    // ---- 之后的值（Hit / Break / Recover / PoisonTick / Heal）----
    int hp = 0;
    int toughness = 0;
    std::string text;
};

class BattleState {
public:
    // 横版没有战场尺寸：只要单位与种子。
    void setup(std::vector<Unit> units, std::uint32_t seed);

    // ---- 扩展接口：必须在 setup() 之后调用（setup 会清空这些登记）----
    void addMagic(const Magic& magic);
    void addItem(const Item& item);
    void setCanEscape(bool canEscape);   // 剧情硬仗可关掉逃跑

    // ---- 登记表的只读视图 ----
    // 「这一场登记了什么」本身就是一条要被钉住的行为（识海那一场必须一件都没有）。
    [[nodiscard]] bool hasMagic(const std::string& id) const { return magics_.count(id) != 0; }
    [[nodiscard]] bool hasItem(const std::string& id) const { return items_.count(id) != 0; }
    [[nodiscard]] std::size_t registeredMagicCount() const { return magics_.size(); }
    [[nodiscard]] std::size_t registeredItemCount() const { return items_.size(); }
    // 按 id 字典序（std::map 的遍历序），因此菜单的行序是确定的。
    [[nodiscard]] std::vector<std::string> registeredMagicIds() const;
    [[nodiscard]] std::vector<std::string> registeredItemIds() const;
    [[nodiscard]] const Magic* findMagic(const std::string& id) const;
    [[nodiscard]] const Item* findItem(const std::string& id) const;

    // 识海之战的吞噬模式。必须在 setup() 之后调用。
    //
    // 开启后本场只剩体积这一个数：伤害不再走攻防与五行，而是按攻方体积算一口
    // 咬多深（devourBite）。开战体积大过吞噬者的敌人，被咬掉三分之一就脱身逃走，
    // 本场以 EnemyFled 结束。架势、破绽与劲一并清成 0：梦里没有这一套。
    void setDevourMode(bool on);
    [[nodiscard]] bool devourMode() const { return devour_; }

    // ---- 多波次 ----
    // **当前这一波的敌人全部倒下时，下一波才进场**；最后一波打完才算 Won。
    // 波与波之间不回血、不重置：推波只改 onField 与 currentWave_，一个字节也不碰
    // 既有单位——要破坏这一条得专门去写一段回血。新进场的单位**下一回合**才轮到行动。
    [[nodiscard]] int currentWave() const { return currentWave_; }
    [[nodiscard]] int waveCount() const { return maxWave_ + 1; }

    // 战果：逃走那一方被咬掉的体积百分比（0-100），没人逃走时为 0。
    [[nodiscard]] int devourSpoilsPercent() const { return spoilsPercent_; }

    [[nodiscard]] const std::vector<Unit>& units() const { return units_; }
    [[nodiscard]] int currentActor() const;   // -1 表示本回合所有人都动完了
    [[nodiscard]] int round() const { return round_; }
    [[nodiscard]] BattlePhase phase() const { return phase_; }

    // ---- 行动序 ----
    // 本回合的完整行动序（单位下标；首领一回合几次行动就出现几次），含已经动过的。
    [[nodiscard]] const std::vector<int>& roundOrder() const { return order_; }
    // 本回合走到行动序的第几位；等于 roundOrder().size() 表示本回合已经走完。
    [[nodiscard]] std::size_t orderPosition() const { return orderPos_; }
    // 下一回合的行动序，按**此刻**的局面算。下一回合开始时用的就是同一个函数，
    // 所以界面上这一行预览不会骗人：只要这一刻之后局面不再变（没人再倒下、
    // 再防御、再破势），下一回合就是这个序。
    [[nodiscard]] std::vector<int> nextRoundOrder() const;

    // 战斗日志，供 UI 直接显示；apply() 的返回值也会追加到这里。
    [[nodiscard]] const std::vector<std::string>& log() const { return log_; }

    // 事件流：从开战起的全部事件，只增不减。画面记一个游标，每帧取游标之后的。
    [[nodiscard]] const std::vector<BattleEvent>& events() const { return events_; }
    // 最近一次**成功的** apply() 产生的那一段（不含之后 endTurn 带出来的回合事件）。
    [[nodiscard]] std::vector<BattleEvent> lastActionEvents() const;

    [[nodiscard]] bool isLegal(const Action& action) const;
    // 同一条判定，另外把拒绝的理由带出来（UTF-8，可直接显示给玩家）。菜单画的禁用
    // 理由与 apply 回绝时给的那句话，必须字字相同。
    [[nodiscard]] bool isLegal(const Action& action, std::string* why) const;
    // 非法动作返回 failure，且不改变任何状态、不消耗任何资源。
    core::Result<std::string> apply(const Action& action);   // value 为战斗日志行

    // 推进到下一个行动者；已是最后一个时 currentActor() 变 -1，
    // 再调一次才开新回合（让驱动方有机会观察到「回合结束」这个状态）。
    void endTurn();

    // 纯函数式：不修改任何状态，只根据当前局面挑一个动作。敌我两方各有一套
    // （docs/octopath-battle.md 第 3 节）。无动作可做时返回 actorIndex == -1 的
    // 空动作，驱动方据此直接 endTurn()。
    [[nodiscard]] Action decideAi(int actorIndex) const;

    // ---- 给界面与 AI 的只读判断（与结算走同一份口径）----
    // 这一手打的是哪几类：普攻 = 所出的兵刃；法术 = 五行（带毒加「毒」）；毒药 = 毒。
    // 其余动作、吞噬模式一律 0。
    [[nodiscard]] int hitCategories(const Action& action) const;
    // 这一手打几击：普攻 1+蓄劲；连发型法术 1+蓄劲；其余 1（毒药也是一击）。
    [[nodiscard]] int hitCount(const Action& action) const;
    // 普攻出的是哪一样兵刃：action.category 非 0 就是它，否则取这个单位位序最前的那一样。
    [[nodiscard]] int attackCategory(const Action& action) const;
    // 这一击对 target 估计打多少（含破势 ×2、防御减半、蓄势倍数；不含之后的变化）。
    // AI 挑目标与菜单报数都走它，不另算一份。
    [[nodiscard]] int estimateHitDamage(const Action& action, int targetIndex) const;

private:
    [[nodiscard]] Unit& unitRef(int index) { return units_[static_cast<std::size_t>(index)]; }
    [[nodiscard]] const Unit& unitRef(int index) const {
        return units_[static_cast<std::size_t>(index)];
    }
    [[nodiscard]] bool validIndex(int index) const;

    // isLegal 与 apply 共用同一套判定，杜绝两者漂移。why 可为空。
    [[nodiscard]] bool checkLegal(const Action& action, std::string* why) const;
    [[nodiscard]] bool checkAttack(const Action& a, const Unit& u, std::string* why) const;
    [[nodiscard]] bool checkCast(const Action& a, const Unit& u, std::string* why) const;
    [[nodiscard]] bool checkItem(const Action& a, const Unit& u, std::string* why) const;
    [[nodiscard]] bool checkEscape(const Unit& u, std::string* why) const;
    [[nodiscard]] bool checkCharge(const Unit& u, std::string* why) const;
    [[nodiscard]] bool checkBoost(const Action& a, const Unit& u, std::string* why) const;
    [[nodiscard]] bool checkEnemyTarget(int targetIndex, const Unit& actor, std::string* why) const;

    std::string applyAttack(const Action& a);
    std::string applyCast(const Action& a);
    // 看破（Magic::effect == Reveal）：扣法力，本场每一个站着的敌人破绽全部揭开。
    // 不算一次攻击——不判破绽、不削架势、不打断蓄势、不扣劲（蓄了也只当没蓄）。
    std::string applyReveal(const Action& a, const Magic& magic);
    std::string applyItem(const Action& a);
    std::string applyDefend(const Action& a);
    std::string applyEscape(const Action& a);
    std::string applyCharge(const Action& a);

    // 一击的后果。结算函数只改局面、把发生了什么记在这里；事件由 emitStrike 按
    // 「先这一击、再破势（与打断蓄势）、再倒下 / 遁走」的顺序一次写出——画面逐条播的
    // 就是这个顺序，先看见这一刀，再看见他倒下。
    struct StrikeOutcome {
        int damage = 0;
        int revealed = 0;         // 这一击新揭开的破绽
        bool weakness = false;    // 打中了破绽
        bool broke = false;       // 这一击把它打到破势
        bool interrupted = false; // 顺带打断了它的蓄势
        bool fell = false;
        bool fled = false;
    };

    // 扣劲并记一笔（Attack / Cast 共用）。
    void spendBoost(int actorIndex, int boost);
    // 这一手打谁：重招且 all 为真时是对面全体，否则就是 action.targetIndex。
    [[nodiscard]] std::vector<int> strikeTargets(int actorIndex, int targetIndex) const;
    // 一击：先按此刻的状态算伤害落下去，再判破绽、揭开、削架势、破势。
    // 返回追加到日志里的半句（不带开头的「，」）。
    std::string strike(int attackerIndex, int targetIndex, int baseDamage, int category, int hit,
                       int hits);
    // 不带伤害的一击（毒药撒过去）：只判破绽。返回追加的半句，可能为空。
    std::string probe(int attackerIndex, int targetIndex, int category);
    // 破绽判定：揭开（同 id 的单位一起揭开）、削一点架势、够了就破势。返回追加的半句。
    std::string resolveWeakness(int targetIndex, int category, StrikeOutcome& out);
    void breakUnit(int targetIndex, std::string& text, StrikeOutcome& out);
    void emitStrike(int attackerIndex, int targetIndex, int category, int hit, int hits,
                    const StrikeOutcome& out);

    // 这一击的最终伤害：基数乘上破势 ×2、防御减半、重招倍数，至少 1。
    [[nodiscard]] int finalDamage(int baseDamage, const Unit& attacker, const Unit& target) const;
    // 伤害落下去：扣血、吞噬转移、倒下。返回给玩家看的那半句话。lethal 为假时
    // （这一击的类别不在目标的 killableBy 里）最多把他压到 1。
    std::string inflict(int targetIndex, int damage, int attackerIndex, StrikeOutcome& out,
                        bool lethal);

    // 吞噬：守方掉体积，我方一侧的攻击者长体积，够条件就让守方脱身。返回追加的日志。
    std::string devourTransfer(int attackerIndex, int targetIndex, int hpDamage, StrikeOutcome& out);

    // 回合开始的中毒结算：扣血、回合数递减、散尽时清干净。
    void tickPoison(int unitIndex);
    // 给目标挂上毒，返回追加到日志里的那半句。turns / power 任一为 0 即无事发生。
    std::string applyPoison(int targetIndex, int turns, int power);

    void beginRound();
    // 跳过这一位上动不了的人：倒下的、不在场的、破势的。
    void skipIneligibleActors();
    // 轮到当前行动者出手：他上一次摆开的守势就此散去。**刻意不放进
    // skipIneligibleActors()**：apply() 之后不会换人，只有 beginRound() 与 endTurn()
    // 里真的换了人才走这一步——放进去的话，刚摆开的守势会在同一次调用里被抹掉。
    void beginActorTurn();
    // 按回合号 roundNumber 排出行动序。beginRound 与 nextRoundOrder 走的是同一个。
    [[nodiscard]] std::vector<int> computeOrder(int roundNumber) const;
    // 身法扰动（千分数，±kSpeedJitterPermille），由种子、回合、单位、第几次行动派生。
    [[nodiscard]] int speedJitter(int roundNumber, int unitIndex, int slot) const;
    // 位混合（splitmix64 的收尾那一步）。行动序扰动与 AI 的「随机」都从它派生，
    // 都不碰 rng_：rng_ 只给逃跑掷骰，免得多掷一次就把后面所有的骰子挪一位。
    [[nodiscard]] static std::uint64_t mixBits(std::uint64_t value) noexcept;

    // ---- 推波 ----
    [[nodiscard]] bool hasPendingWave() const { return currentWave_ < maxWave_; }
    void deployNextWave();
    void refreshPhase();
    void endBattle(BattlePhase outcome);
    [[nodiscard]] int escapeChance(const Unit& runner) const;

    void emit(BattleEvent event) { events_.push_back(std::move(event)); }

    // ---- AI 的分步决策，全部 const（BattleAi.cpp）----
    [[nodiscard]] Action enemyDecision(int actorIndex) const;
    [[nodiscard]] Action allyDecision(int actorIndex) const;
    // 敌方挑目标：能击倒的优先；都击不倒就按种子随机，偏向气血低的。
    [[nodiscard]] int enemyTarget(int actorIndex, const std::vector<Action>& perTarget) const;
    // 由种子派生的一个确定性「随机数」：不耗 rng_，decideAi 因此仍是纯函数。
    [[nodiscard]] std::uint64_t aiRoll(int actorIndex) const;

    std::vector<Unit> units_;
    std::map<std::string, Magic> magics_;
    std::map<std::string, Item> items_;

    std::vector<int> order_;              // 本回合行动序（单位下标）
    std::size_t orderPos_ = 0;
    // 当前这一位已经出过手了。首领一回合几次行动，行动序里就有它几位，
    // 所以「本回合已经行动过」按**位**判，不按人判。
    bool slotUsed_ = false;
    int round_ = 0;
    BattlePhase phase_ = BattlePhase::Ongoing;
    bool canEscape_ = true;

    // ---- 吞噬模式的记账 ----
    bool devour_ = false;
    // 各单位开战时的体积。逃遁阈值与战果都是相对开战体积算的。
    std::vector<int> startMaxHp_;
    bool anyEnemyFled_ = false;
    int spoilsPercent_ = 0;

    // ---- 波次记账 ----
    int currentWave_ = 0;
    int maxWave_ = 0;

    std::uint32_t seed_ = 0;
    std::mt19937 rng_;
    std::vector<std::string> log_;
    std::vector<BattleEvent> events_;
    std::size_t lastActionBegin_ = 0;
    std::size_t lastActionEnd_ = 0;
};

}  // namespace fanren::core::battle
