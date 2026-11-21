#pragma once
// 同道录（第 7 章第二期 2a，docs/ch07-tongdaolu.md 第 3 节；接口文档 docs/ch07-tongdaolu-k.md）。
//
// core 层硬约束：不含 SDL / JSON / Lua / 文件 IO。纯函数，读写的只有传进来的 GameState。
// 新状态一律放 flags（每面旗登记在 data/flags.json）与现有的 bonds，不加存档字段（总则 2.1）。
//
// 这里收五样东西，别处不许各写一套：
//   1. 生效判据 rosterEffectsActive：新法术解锁、伙伴被动、在册加成、同修的劲都只在第 7 章生效；
//   2. 档名按线取（bondTierNameKey）；
//   3. 被动与在册加成的种类表（perkTable）——数据里只许引用表里的；
//   4. 在册加成：谁的解锁了、按境界封顶、超额时按勾选取（activeRosterBonuses）；
//   5. 相处的记账与每日限制：同修、切磋、请教、叙话，以及不入队也能记的赠礼。
// 战斗里怎么套用（建场时）见 battleBonuses / companionKit，由 game 层的 BattleScene 调。
#include <map>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Companions.h"
#include "core/rules/Realm.h"

namespace fanren::rules {

// ---------------------------------------------------------------------------
// 1. 生效判据（施工图 2.1）
// ---------------------------------------------------------------------------

// 「第 7 章里」：第 6 章已演完、第 7 章还没演完。与对白头像、手绘人物限定在第 7 章的口径相同
// （按章节表的 done_flag；不看 GameState::chapter——那个字段剧情不维护）。
inline constexpr const char* kRosterEffectsStartFlag = "ch06.done";
inline constexpr const char* kRosterEffectsEndFlag = "ch07.done";

// 第 7 章的战斗与历练里为真。按档解锁的法术、伙伴被动、各段 roles[i].duo、在册加成、同修的劲都只在为真时套用；
// 为假时战斗与改版前逐项相同。
[[nodiscard]] bool rosterEffectsActive(const core::GameState& state);

// ---------------------------------------------------------------------------
// 2. 档名（施工图 3.2）
// ---------------------------------------------------------------------------

// 线的数据名：general / hongyan / moni / beast。
[[nodiscard]] const char* bondLineName(core::BondLine line);
// 认得的数据名写进 out 并返回 true。
[[nodiscard]] bool bondLineFromName(const std::string& name, core::BondLine& out);

// 第 tier 档（1–5，越界夹住）的档名文案 key。通用线沿用改版前的 ui.bond.tier<N>（初识…同心），
// 其余三条是 ui.bond.<line>.tier<N>（data/text/ui_companions.json）。
[[nodiscard]] std::string bondTierNameKey(core::BondLine line, int tier);

// 这位伙伴此刻最高能到第几档（封顶没解开时是 capTier，否则 5）。同道录里「机缘未到」那一格据此画。
[[nodiscard]] int bondTierCap(const core::Companion& companion, const core::GameState& state);

// ---------------------------------------------------------------------------
// 3. 被动与在册加成的种类表（施工图 3.3、3.6）
// ---------------------------------------------------------------------------

// 一种被动能放在哪一格：伙伴自己的被动（roles[i].passive），还是在册加成（roster_bonus）。
inline constexpr int kPerkSlotPassive = 1;
inline constexpr int kPerkSlotRoster = 2;

struct PerkInfo {
    core::PerkKind kind = core::PerkKind::None;
    const char* name = "";      // 数据里写的名字
    int slots = 0;              // kPerkSlotPassive / kPerkSlotRoster 的位或
    int minValue = 1;           // value（与 value5）的取值范围，加载器查
    int maxValue = 1;
    bool needsElement = false;  // 要写 element
    const char* summary = "";   // 给维护者看的一句话（界面文案另由路 U 定）
};

// 整张表，按 PerkKind 的次序（不含 None）。
[[nodiscard]] const std::vector<PerkInfo>& perkTable();
[[nodiscard]] const PerkInfo* perkInfo(core::PerkKind kind);
// 按数据名查；认不出返回 nullptr（加载器据此报错）。
[[nodiscard]] const PerkInfo* perkByName(const std::string& name);

// 伙伴被动在第 tier 档的数：三档前 0，三、四档 value，五档 value5（没写 value5 按 value）。
[[nodiscard]] int passiveValueAtTier(const core::CompanionPerk& perk, int tier);

inline constexpr int kPassiveMinTier = 3;
inline constexpr int kPassiveStrongTier = 5;

// ---------------------------------------------------------------------------
// 角色那一段：按档解锁的法术、被动、合招
// ---------------------------------------------------------------------------

// 伙伴表里 roleId 那一段；不属于她返回 nullptr。
[[nodiscard]] const core::CompanionRole* companionRoleEntry(const core::Companion& companion,
                                                            const std::string& roleId);

// 她用 roleId 这一段、默契 bond 时上场带的东西。active = rosterEffectsActive(state)。
struct CompanionKit {
    std::vector<std::string> extraMagics;   // 角色表 magics 之外、够档解锁的那几门，按数据次序
    std::string duo;                        // 合招（这一段写了 duo 且 active 时用它，否则伙伴顶层的 duo）
    core::CompanionPerk passive;            // 够三档的被动；value 已换成这一档的数（五档是 value5）；不够档 kind None
};
[[nodiscard]] CompanionKit companionKit(const core::Companion& companion, const std::string& roleId, int bond,
                                        bool active);

// ---------------------------------------------------------------------------
// 旗标（data/flags.json 登记；伙伴 id 进旗标名）
// ---------------------------------------------------------------------------

[[nodiscard]] std::string qingjiaoFlag(const std::string& companionId);   // 请教过：1
[[nodiscard]] std::string xuhuaFlag(const std::string& companionId);      // 叙话演到第几档：0 / 2–5
[[nodiscard]] std::string sparDayFlag(const std::string& companionId);    // 上一次切磋的日子
[[nodiscard]] std::string sparPrizeFlag(const std::string& companionId);  // 首胜奖品领到第几档：0 / 2–5
[[nodiscard]] std::string bonusOnFlag(const std::string& companionId);    // 在册加成勾选：1 = 勾上
// 同修（全局一面）：做过同修的那一天；与「第二天第一场战斗多 1 点劲」那一天（用掉清 0）。
inline constexpr const char* kTongxiuDayFlag = "tongdao.tongxiu_day";
inline constexpr const char* kTongxiuBonusDayFlag = "tongdao.tongxiu_bp_day";

// ---------------------------------------------------------------------------
// 4. 在册加成（施工图 3.6）
// ---------------------------------------------------------------------------

// 同时生效的条数：炼气（及以下）4，筑基及以上 6。
inline constexpr int kRosterBonusSlotsQi = 4;
inline constexpr int kRosterBonusSlotsFoundation = 6;
[[nodiscard]] int rosterBonusSlots(Realm realm);

// 她的在册加成解锁了没有：她在名册上、表里写了 roster_bonus，而且——人物请教过；灵兽默契到二档。
[[nodiscard]] bool rosterBonusUnlocked(const core::Companion& companion, const core::GameState& state);

// 此刻生效的在册加成是谁的（伙伴 id，按伙伴 id 排）。解锁的不超过名额：全部生效。
// 超过名额：先取勾上的（bonusOnFlag），再按次序补没勾的，取满名额为止。
// 与 rosterEffectsActive 无关——这里只答「谁的算数」，套不套用由建场时的判据定。
[[nodiscard]] std::vector<std::string> activeRosterBonuses(const std::map<std::string, core::Companion>& companions,
                                                           const core::GameState& state);

// ---------------------------------------------------------------------------
// 战斗修正：建场时套用（game 层 BattleScene::buildFromSetup 调）
// ---------------------------------------------------------------------------

struct StaggerBonus {
    int categories = 0;   // 这几类的一击（AttackCategory 位）
    int amount = 0;       // 打中破绽时多削几点架势
};

// 一场仗开场的修正。全是「加在改版前的规矩上」：缺省全 0，等于没套。
struct BattleBonuses {
    int openingBp = 0;             // 我方全员开场多几点劲；与先手的 1 点不叠加，取大
    int ambushBp = 0;              // 先手（偷袭明雷得手）时我方全员再多几点；不是先手不读
    int victoryHealPercent = 0;    // 打赢后我方站着的人各回几成（百分比）
    int talismanPowerPercent = 0;  // 符施展的威力加几成（百分比）
    std::vector<StaggerBonus> heroStagger;   // 主角的法术打中破绽多削的架势
    bool usesTongxiu = false;      // 这一场用掉了同修的劲（调用方把 kTongxiuBonusDayFlag 清 0）
};

// 同修的劲今天有没有（第二天第一场战斗用；用掉就没了）。
[[nodiscard]] bool tongxiuBonusReady(const core::GameState& state);

// 这一场的修正。fieldedCompanions 是上场伙伴的（伙伴 id，此刻的被动 kit）。
// 判据不成立（rosterEffectsActive 为假）、切磋、主角不在场时一律全 0——调用方只管照套。
[[nodiscard]] BattleBonuses battleBonuses(const std::map<std::string, core::Companion>& companions,
                                          const core::GameState& state,
                                          const std::vector<core::CompanionPerk>& fieldedPassives,
                                          bool spar, bool heroPresent);

// ---------------------------------------------------------------------------
// 5. 相处（施工图 3.4、3.5）
// ---------------------------------------------------------------------------

// 门槛（施工图 3.4 的表）。
inline constexpr int kSparMinTier = 2;
inline constexpr int kQingjiaoMinTier = 3;
inline constexpr int kXuhuaMinTier = 2;
// 记多少。
inline constexpr int kBondTongxiu = 4;
inline constexpr int kBondSparWin = 4;
inline constexpr int kBondSparLoss = 2;
inline constexpr int kTongxiuOpeningBp = 1;

// 能不能做、为什么不能。机器码（tongdaoCheckCode）给脚本与界面分支用，界面文案另定。
enum class TongdaoCheck {
    Ok,
    NoCompanion,   // "no_companion"  data/companions 里没有这位
    NotMet,        // "not_met"       还没进同道录
    NotPerson,     // "not_person"    灵兽不做这件事（同修 / 切磋 / 请教 / 叙话只给人物）
    TierLow,       // "tier_low"      默契不够档
    DoneToday,     // "done_today"    今天做过了（同修 / 切磋）
    AlreadyDone,   // "already"       只做一次的已经做过（请教）；叙话没有待演的
    NoSpar,        // "no_spar"       她的表里没写切磋的编成
};
[[nodiscard]] const char* tongdaoCheckCode(TongdaoCheck check);

// 她在不在同道录里（met_flag 置了）。
[[nodiscard]] bool inRoster(const core::Companion& companion, const core::GameState& state);

// 同修：一档、人物、今天还没同修过。做了默契 +4（受封顶），记下今天，并把「明天第一场战斗多 1 点劲」挂到明天。
// **不过日子**：调用方紧接着走 Application::advanceDays(1)（平常过一天的那条路）。
[[nodiscard]] TongdaoCheck canTongxiu(const std::map<std::string, core::Companion>& companions,
                                      const std::string& companionId, const core::GameState& state);
struct BondGain {
    TongdaoCheck check = TongdaoCheck::Ok;   // 不是 Ok 时一个字节都没写
    int gained = 0;                          // 实际加了多少（封顶之后）
};
BondGain tongxiu(const std::map<std::string, core::Companion>& companions, const std::string& companionId,
                 core::GameState& state);

// 切磋：二档、人物、表里有编成、今天还没跟她切磋过。
[[nodiscard]] TongdaoCheck canSpar(const std::map<std::string, core::Companion>& companions,
                                   const std::string& companionId, const core::GameState& state);
struct SparOutcome {
    TongdaoCheck check = TongdaoCheck::Ok;
    int gained = 0;                       // 默契实际加了多少
    int prizeTier = 0;                    // 这一场领了第几档的首胜奖品；0 = 没领
    std::vector<core::BagEntry> prize;    // 领到的东西（已放进背包）
};
// 打完记账：记下今天；赢 +4、输 +2（受封顶）；赢了且开打时这一档的首胜还没领过、表里这一档有奖品，
// 东西放进背包、记下这一档。首胜按开打时的档算（这一场把她推上新档的，下一场赢了才领新档那一份）；
// 跳过去的档不补发。canSpar 不成立时什么都不写。
SparOutcome recordSpar(const std::map<std::string, core::Companion>& companions, const std::string& companionId,
                       core::GameState& state, bool won);

// 请教：三档、人物、还没请教过。记下之后她的在册加成解锁（rosterBonusUnlocked）。
[[nodiscard]] TongdaoCheck canQingjiao(const std::map<std::string, core::Companion>& companions,
                                       const std::string& companionId, const core::GameState& state);
TongdaoCheck recordQingjiao(const std::map<std::string, core::Companion>& companions,
                            const std::string& companionId, core::GameState& state);

// 叙话：下一段该演第几档的事件——二档起、不高于此刻的档、比演过的高的最低那一档；没有返回 0。
// 一次升了两档，就分两次叙话各演一段。
[[nodiscard]] int xuhuaPending(const std::map<std::string, core::Companion>& companions,
                               const std::string& companionId, const core::GameState& state);
// 记「第 tier 档那段演过了」（只升不降）。tier 不在 [2, 此刻的档] 里、或不是人物，返回不成立的那一条、不写。
TongdaoCheck recordXuhua(const std::map<std::string, core::Companion>& companions, const std::string& companionId,
                         core::GameState& state, int tier);

// 赠礼（账照旧在 giveGift）：对在册的人物记账，不必在队伍里。封顶照 bondCap。
struct GiftResult {
    TongdaoCheck check = TongdaoCheck::Ok;   // NoCompanion / NotMet 时什么都没写
    GiftOutcome outcome;
};
GiftResult giveGiftTo(const std::map<std::string, core::Companion>& companions, const std::string& companionId,
                      const std::string& itemId, core::GameState& state);

}  // namespace fanren::rules
