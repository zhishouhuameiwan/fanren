#include "game/CultivationScene.h"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

#include "core/rules/Bottle.h"
#include "core/rules/Calendar.h"
#include "game/Application.h"

// ---------------------------------------------------------------------------
// 余数账：为什么打坐要这么绕
//
// rules::meditate 只回整点修为，而炼气期升一层只要几十点，摊到每天不足
// 一点。直接把返回值加进修为，「打坐一日」永远得零——那个选项等于摆设。
// 规则层刻意不设每日保底（保底会被拆成一天一次打坐刷十倍），于是零头必须
// 由 game 层自己记账，这正是 GameState::cultivationRemainder 的用途。
//
// 记的是「天」而不是「点」：rules::CultivationGain 里没有小数位，拿不到
// 不足一点的那部分，唯一还原得出它的量就是累计打坐的天数。于是余数账存的
// 是「本轮累计已打坐的天数」，每次结算取同一条曲线上的一段差分：
//
//   本次进账 = meditate(累计天数 + 本次天数) − meditate(累计天数)   // 同一颗种子
//   累计天数 += 本次天数
//
// 这样连打 36 个十日与一次打坐 360 日结果**完全相同**：相邻两段的首尾项
// 两两抵消，总和恒等于 meditate(总天数)。零头一次也没有被抹掉。
//
// 曾经试过「扣掉刚好凑出这些点数所需的最少天数、只留零头」的写法，看着更
// 省事，但那一步会把「凑整时多付出的那一点点」丢掉：六十次一日打坐比一次
// 六十日少一点修为。差得不多，却正好是玩家会去验算的那种数。
//
// 代价是累计天数会一直涨。meditate 单次上限十年，涨到上限就换一次账，
// 那一次丢掉的仍然只有不足一点的零头——十个游戏年才发生一次。
//
// 还有一个前提同样要紧：**一轮账之内种子必须不变**（见 meditationSeed）。
// 差分落在同一条曲线上，长短两种切法才连顿悟都对得齐；种子若一按一换，
// 顿悟就成了各掷各的，读档重掷与切细刷顿悟这两个口子会一并打开。
// ---------------------------------------------------------------------------

namespace fanren::game {
namespace {

// 面板占屏中央偏上的一大块，底下留出世界层让玩家认得出自己在哪儿。
constexpr int kPanelX = 120;
constexpr int kPanelY = 60;
constexpr int kPanelW = 1040;
constexpr int kPanelH = 600;

// 灵根测定的旗标（第 6 章太南谷测过灵根时置；契约 docs/interfaces-p3-ch06.md 第 2 节）。
// 旗标名只写在这一处。
constexpr const char* kSpiritRootFlag = "ch06.linggen";

// 左侧状态栏占的比例。列表要留得下「尚差 1234 点修为」这种长理由。
constexpr int kStatusPercent = 55;

// 动作种子的盐。两个动作分开取盐，免得「同一天打坐与冲关」共用一次掷骰。
constexpr std::uint32_t kMeditateSalt = 0x4D454431u;      // 'MED1'
constexpr std::uint32_t kBreakthroughSalt = 0x42524B31u;  // 'BRK1'

// 主菜单的三项。下标写死在 update 里，用具名常量而不是字面量。
constexpr int kActionMeditate = 0;
constexpr int kActionBreakthrough = 1;
constexpr int kActionLeave = 2;

[[nodiscard]] std::uint32_t hashSeed(std::initializer_list<std::uint32_t> values) {
    std::uint64_t hash = 0x9E3779B97F4A7C15ull;
    for (const std::uint32_t value : values) {
        hash ^= value + 0x9E3779B97F4A7C15ull + (hash << 6) + (hash >> 2);
    }
    return static_cast<std::uint32_t>(hash ^ (hash >> 32));
}

// 打坐的种子：**刻意只看境界与资质**，一轮余数账之内恒定不变。
//
// 这是「长短打坐等价」能够无条件成立的前提，不是图省事。种子若跟着天数或
// 修为走，每按一次打坐就换一条顿悟曲线：36 次十日与一次 360 日的顿悟次数
// 不再相等，玩家还能靠读档把顿悟重掷到满意为止。钉死成一条曲线之后，顿悟
// 落在累计的第几天是定死的，怎么切都切不出便宜，读档也换不来第二次机会。
//
// 代价是同一境界内顿悟的位置不随存档变化。要让它既不可刷又有变化，得让
// GameState 存一颗按轮更新的种子——那是 core 层的改动，见交付报告。
[[nodiscard]] std::uint32_t meditationSeed(const core::GameState& state) {
    return hashSeed({kMeditateSalt, static_cast<std::uint32_t>(rules::toValue(state.realm)),
                     static_cast<std::uint32_t>(state.aptitude)});
}

// 冲关的种子。同一存档、同一天、同一境界必得同一结果（契约第 5 节的可复现
// 要求）。混入修为是为了让「冲关失败之后立刻再冲」不是同一次掷骰的重播。
[[nodiscard]] std::uint32_t breakthroughSeed(const core::GameState& state) {
    return hashSeed({kBreakthroughSalt, static_cast<std::uint32_t>(state.day),
                     static_cast<std::uint32_t>(state.cultivation),
                     static_cast<std::uint32_t>(rules::toValue(state.realm))});
}

// 结算一段（累计天数不超过 rules::kMaxMeditateDays）的打坐，返回本段进账的修为。
// 只改余数账，修为由调用方一次加上——两处都改会让「进账多少」失去唯一来源。
[[nodiscard]] int settleMeditation(core::GameState& state, int carry, int chunk, int aptitude,
                                   int effectiveness, std::uint32_t seed, bool& insight) {
    const rules::Realm realm = state.realm;

    // 两次调用必须用同一颗种子，差分才落在同一条曲线上：顿悟是逐日判定的，
    // 换了种子，攒着的那几天会被重掷，差值里就掺进了本次没发生的运气。
    const rules::CultivationGain base =
        carry > 0 ? rules::meditate(realm, aptitude, effectiveness, carry, seed)
                  : rules::CultivationGain{};
    const rules::CultivationGain full =
        rules::meditate(realm, aptitude, effectiveness, carry + chunk, seed);

    // 顿悟只在「新添的这几天里头一回出现」时报。base 已经有顿悟时分不出新旧
    // （CultivationGain 只给了一个布尔，没给次数），宁可少报一次也不要每次
    // 打坐都弹同一句「心有所悟」——那样这句话就彻底不值钱了。
    if (full.insight && !base.insight) insight = true;

    state.cultivationRemainder = carry + chunk;
    return std::max(0, full.cultivation - base.cultivation);
}

// 这一次突破补上的气血与法力。
struct RealmGrowth {
    int hp = 0;
    int mp = 0;
};

// 把境界该给的气血 / 法力上限补齐（口径与数值依据见 core/rules/Realm.h）。
//
// 在这之前突破只做一件事：把 `state.realm` 换成下一档。maxHp / maxMp 一个字
// 不动，而全作也没有别的地方让它们涨——玩家从第 1 章练到第 3 章的炼气三层，
// 生存能力与开局一模一样，照这个走法到第 14 章还是那 10 点气血。修为于是只是
// 一道剧情闸门，不是「变强」。
//
// 补齐这件事只落在**两个**地方：这里（突破当场）与 io/SaveFile.cpp 的 v3→v4
// 迁移（老存档）。两条路都走 `rules::liftToFloor`，同一条「只补不削 + 当前值
// 同步抬高」的规则不写第二遍——界面各算一套是数值失控最常见的来源。
[[nodiscard]] RealmGrowth applyRealmAttributes(core::GameState& state) {
    const rules::Vitals hp =
        rules::liftToFloor({state.hp, state.maxHp}, rules::realmMaxHp(state.realm));
    const rules::Vitals mp =
        rules::liftToFloor({state.mp, state.maxMp}, rules::realmMaxMp(state.realm));
    const RealmGrowth growth{hp.max - state.maxHp, mp.max - state.maxMp};
    state.hp = hp.current;
    state.maxHp = hp.max;
    state.mp = mp.current;
    state.maxMp = mp.max;
    // 瓶子容量的下限跟着大境界走（契约 docs/interfaces-p3-ch07.md 3.3 第 2 处；另两处是 Application
    // 的 RealmAdvance 与 SaveFile 读档，同一个函数）。只补不削，不送液；同档内升层不变。
    state.bottle.capacity =
        std::max(state.bottle.capacity, rules::bottleCapacityFloor(rules::tierOf(state.realm)));
    return growth;
}

// 把百分数说成人话。玩家读「三成把握」比读「30%」更有分量，但精确值也得给，
// 否则丹药加成加了三个点在界面上看不出来。
[[nodiscard]] std::string chanceText(int chance) {
    return std::to_string(chance) + "%（约 " + std::to_string(chance / 10) + " 成把握）";
}

// ---- 两套用词 ----
//
// 凡人那一套的判据只有一条：**放进第 2 章的文案里，校对的禁词表扫不出东西**
// （修仙/灵根/法术/法力/灵气/气感/炼气/筑基/真元/走火入魔/境界…）。
// 改这张表时请连 tests/PanelTests.cpp 的禁词扫描一起看——那条测试才是这条
// 约束真正的守门人，改坏了它会立刻转红。
constexpr CultivationLexicon kMortalLexicon{
    /*meditateLabel*/        "打坐",
    /*meditateDetail*/       "按旬月计，日日不辍",
    /*pushLabel*/            "试下一层",
    /*pushCapped*/           "这册口诀只到这里了",
    /*pushShortPrefix*/      "尚差 ",
    /*pushShortSuffix*/      " 点火候",
    /*leaveLabel*/           "离开",
    /*backLabel*/            "返回",

    /*realmLabel*/           "口诀　",
    /*aptitudeLabel*/        "资质　",
    /*dateLabel*/            "日期　",
    /*cultivationLabel*/     "火候　",
    /*cappedNote*/           "（口诀只到这里）",
    /*carryPrefix*/          "积日　",
    /*carrySuffix*/          " 日功夫在身，尚未凝成火候",
    /*hpLabel*/              "气血　",
    /*mpLabel*/              "气力　",

    /*enterHint*/            "凝神静气，可择时打坐；火候足时可试下一层。",
    /*sitPrefix*/            "静坐 ",
    /*sitMiddle*/            " 日，火候增 ",
    /*sitSuffix*/            " 点。",
    /*tooShort*/             "这点功夫尚不足一分火候，且先记在心里，积日方见。",
    /*insight*/              "一直拗口的那一句忽然顺了下来，这一段功课抵得寻常月余。",
    /*dailyPracticePrefix*/  "这些时日照旧夜夜默过一遍，零零碎碎也积了 ",
    /*dailyPracticeSuffix*/  " 点火候。",
    /*notReady*/             "火候未到，硬往下练只是白白折损。",
    /*breakSuccessPrefix*/   "那一段拗口的地方一气贯了下来，口诀自 ",
    /*breakSuccessMiddle*/   " 到 ",
    /*breakSuccessSuffix*/   "。",
    /*breakFailPrefix*/      "这一层没接上，前些日子的功夫散了 ",
    /*breakFailSuffix*/      " 点。",
    /*backlashPrefix*/       "胸口猛地一闷，眼前发黑，前些日子的功夫散了 ",
    /*backlashSuffix*/       " 点。",
    /*pushAtStoryCap*/       "卡在这一层了，火候再足也推不上去",
    /*atStoryCap*/           "口诀默到这一层就接不下去了，火候攒得再足也推不过去。"
                             "不是运气差——是还没到时候。",
    /*spiritRootLabel*/      "",
    /*spiritRootValue*/      "",
    /*siteBonusPrefix*/      "此处静坐格外见效，火候多得 ",
    /*siteBonusSuffix*/      "%",
};

constexpr CultivationLexicon kImmortalLexicon{
    /*meditateLabel*/        "打坐",
    /*meditateDetail*/       "按旬月计，聚气归元",
    /*pushLabel*/            "冲关",
    /*pushCapped*/           "已至本作境界之极",
    /*pushShortPrefix*/      "尚差 ",
    /*pushShortSuffix*/      " 点修为",
    /*leaveLabel*/           "离开",
    /*backLabel*/            "返回",

    /*realmLabel*/           "境界　",
    /*aptitudeLabel*/        "资质　",
    /*dateLabel*/            "日期　",
    /*cultivationLabel*/     "修为　",
    /*cappedNote*/           "（已至境界之极）",
    /*carryPrefix*/          "积日　",
    /*carrySuffix*/          " 日功夫在身，尚未凝成修为",
    /*hpLabel*/              "气血　",
    /*mpLabel*/              "法力　",

    /*enterHint*/            "凝神静气，可择时打坐；修为足时方能冲关。",
    /*sitPrefix*/            "静坐 ",
    /*sitMiddle*/            " 日，修为增 ",
    /*sitSuffix*/            " 点。",
    /*tooShort*/             "这点功夫尚不足一分修为，且先记在心里，积日方见。",
    /*insight*/              "忽而心有所悟，周身灵气自行汇聚，这一段功课抵得寻常月余。",
    /*dailyPracticePrefix*/  "这些时日的日课一并记上，修为增 ",
    /*dailyPracticeSuffix*/  " 点。",
    /*notReady*/             "火候未到，强行冲关只是白白折损。",
    /*breakSuccessPrefix*/   "关隘一冲而破，自 ",
    /*breakSuccessMiddle*/   " 入 ",
    /*breakSuccessSuffix*/   "。",
    /*breakFailPrefix*/      "冲关未成，气机溃散，修为倒退 ",
    /*breakFailSuffix*/      " 点。",
    /*backlashPrefix*/       "真元逆冲，走火入魔！气血翻涌，修为倒退 ",
    /*backlashSuffix*/       " 点。",
    /*pushAtStoryCap*/       "卡在瓶颈上，修为再足也冲不过去",
    /*atStoryCap*/           "卡在瓶颈上了。修为攒得再多也冲不过这一关——"
                             "不是运气差，是时机未到。",
    /*spiritRootLabel*/      "灵根　",
    /*spiritRootValue*/      "四属性缺金·伪灵根",
    /*siteBonusPrefix*/      "此地灵气充沛，修为多得 ",
    /*siteBonusSuffix*/      "%",
};

// 凡人阶段的层数名。不从 rules::nameOf 里截字符串：那是按字节切 UTF-8，
// 改一个字就静默错位，而错位的样子是屏幕上半个汉字。
constexpr std::array<std::string_view, 13> kMortalLayerNames{
    "第一层", "第二层", "第三层", "第四层", "第五层", "第六层", "第七层",
    "第八层", "第九层", "第十层", "第十一层", "第十二层", "第十三层",
};

}  // namespace

const CultivationLexicon& cultivationLexicon(PanelStage stage) {
    return stage == PanelStage::Mortal ? kMortalLexicon : kImmortalLexicon;
}

std::string realmText(PanelStage stage, rules::Realm realm) {
    if (stage != PanelStage::Mortal) return std::string(rules::nameOf(realm));

    const std::int32_t value = rules::toValue(realm);
    if (value == 0) return "尚未入门";
    if (value >= 1 && value <= static_cast<std::int32_t>(kMortalLayerNames.size())) {
        return std::string(kMortalLayerNames[static_cast<std::size_t>(value - 1)]);
    }
    // 凡人阶段走不到筑基以上。真走到了也不许把「筑基初期」漏出去——兜底的
    // 说法照样不带一个禁词，宁可含糊也不违约。
    return "已非这册口诀所载";
}

std::vector<std::string> cultivationPanelStrings(PanelStage stage) {
    const CultivationLexicon& words = cultivationLexicon(stage);
    std::vector<std::string> out{
        words.meditateLabel,      words.meditateDetail,      words.pushLabel,
        words.pushCapped,         words.pushShortPrefix,     words.pushShortSuffix,
        words.leaveLabel,         words.backLabel,           words.realmLabel,
        words.aptitudeLabel,      words.dateLabel,           words.cultivationLabel,
        words.cappedNote,         words.carryPrefix,         words.carrySuffix,
        words.hpLabel,            words.mpLabel,             words.enterHint,
        words.sitPrefix,          words.sitMiddle,           words.sitSuffix,
        words.tooShort,           words.insight,             words.dailyPracticePrefix,
        words.dailyPracticeSuffix, words.notReady,           words.breakSuccessPrefix,
        words.breakSuccessMiddle, words.breakSuccessSuffix,  words.breakFailPrefix,
        words.breakFailSuffix,    words.backlashPrefix,      words.backlashSuffix,
        words.pushAtStoryCap,     words.atStoryCap,         words.spiritRootLabel,
        words.spiritRootValue,    words.siteBonusPrefix,    words.siteBonusSuffix,
    };

    // 境界名也是面板上的字。全境界都列进来——漏了哪一档，那一档就是没人看着
    // 的一条缝，而「炼气三层」正是从这条缝里漏出去的。
    for (const std::int32_t value : {0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,
                                     12, 13, 21, 22, 23, 31, 32, 33}) {
        out.push_back(realmText(stage, rules::fromValue(value)));
    }
    for (const MeditateOption& option : CultivationScene::meditateOptions(stage)) {
        out.push_back(option.label);
    }
    // 把握几成那一行是现拼的，取一个样本一并扫。
    out.push_back(chanceText(85));
    return out;
}

int settleDailyPractice(core::GameState& state) {
    // 进了修仙阶段就不再有日课：那之后日历由玩家自己掌握，被动收益会把后面
    // 每一章的节奏都改掉。水位仍拨到当天，免得日后旗标被清又忽然补一大笔。
    if (wordingStage(state) != PanelStage::Mortal || !rules::isValid(state.realm)) {
        state.lastPracticeDay = state.day;
        return 0;
    }
    // 水位为 0 是「还没起课」：新档从今天起算（剧情上正是师父把口诀教下来、
    // 第一次打坐的那一晚——第 2 章节点 2 的职责就是把这个面板教给玩家）；
    // 这次改动之前存下的老档同样从今天起算，不倒补。
    // 水位跑到未来只能是存档被改坏，同样就地归位而不是倒扣。
    if (state.lastPracticeDay <= 0 || state.lastPracticeDay > state.day) {
        state.lastPracticeDay = state.day;
        return 0;
    }

    const int elapsed = state.day - state.lastPracticeDay;
    const int blocks = elapsed / kDailyPracticeBlockDays;
    if (blocks <= 0) return 0;   // 不足一块就一天也不推水位，零头留着

    const int sessions = blocks * kDailyPracticeSessionsPerBlock;
    const int gained =
        CultivationScene::bankMeditation(state, sessions, meditationSeed(state)).cultivation;
    state.lastPracticeDay += blocks * kDailyPracticeBlockDays;
    return gained;
}

int meditationEffectiveness(const core::GameState&) {
    // 100 是 rules::meditate 的基准。功法、灵根接进来之前就是它。
    // 洞府灵脉不并进这里：那是地点给的，乘在外面（bankMeditation 的 sitePercent）。
    return 100;
}

int breakthroughPillBonus(const core::GameState&) {
    return 0;
}

const std::vector<MeditateOption>& CultivationScene::meditateOptions(PanelStage stage) {
    // 凡人阶段最长只到一月。一个十来岁的武馆杂役，药圃的活计、师父的功课都
    // 在身上，没有「闭关一年」这回事；而这一个选项正是「四年苦修按一次按钮
    // 就过去了」的来源——按一次 360 天，凡人那 20 点门槛当场就够。
    static const std::vector<MeditateOption> kMortalOptions{
        {"打坐一日", 1},
        {"打坐十日", 10},
        {"打坐一月", rules::kDaysPerMonth},
    };
    static const std::vector<MeditateOption> kImmortalOptions{
        {"打坐一日", 1},
        {"打坐十日", 10},
        {"打坐一月", rules::kDaysPerMonth},
        {"闭关一年", rules::kDaysPerYear},
    };
    return stage == PanelStage::Mortal ? kMortalOptions : kImmortalOptions;
}

std::string CultivationScene::spiritRootLine(const core::GameState& state) {
    // 两道闸缺一不可：凡人阶段「灵根」是禁词；进了修仙界而还没测过（第 6 章前半段）也不说。
    // 「资质」那一行不动：aptitude 是修炼速度的输入，与灵根是两件事（Cultivation.cpp 的注释）。
    if (wordingStage(state) != PanelStage::Immortal || state.flag(kSpiritRootFlag) == 0) return {};
    const CultivationLexicon& words = cultivationLexicon(PanelStage::Immortal);
    return std::string(words.spiritRootLabel) + words.spiritRootValue;
}

std::string CultivationScene::siteBonusLine(const core::GameState& state) const {
    // 普通蒲团不画这一行：平时多一行「多得 0%」只是噪音。
    if (sitePercent_ == kDefaultSitePercent) return {};
    const CultivationLexicon& words = cultivationLexicon(wordingStage(state));
    return std::string(words.siteBonusPrefix) + std::to_string(sitePercent_ - kDefaultSitePercent) +
           words.siteBonusSuffix;
}

std::vector<ui::ListItem> CultivationScene::buildMainItems(const core::GameState& state) {
    const CultivationLexicon& words = cultivationLexicon(wordingStage(state));
    std::vector<ui::ListItem> items;

    ui::ListItem meditate;
    meditate.label = words.meditateLabel;
    meditate.detail = words.meditateDetail;
    items.push_back(std::move(meditate));

    ui::ListItem push;
    push.label = words.pushLabel;
    // 按不按得下去、为什么，问的是规则层那一个函数（rules::breakthroughBlock）——
    // 真按下去时 breakthrough() 走的 rules::tryBreakthrough 问的也是它。两处各判一套，
    // 「面板说能按、按下去被回绝」就会出现。
    const int need = rules::cultivationNeeded(state.realm);
    switch (rules::breakthroughBlock(state.realm, state.cultivation, state.realmCap)) {
        case rules::BreakthroughBlock::SeriesMax:
            push.enabled = false;
            push.disabledReason = words.pushCapped;
            break;
        case rules::BreakthroughBlock::StoryCap:
            // 剧情给的上限到了：与「尚差几点」分开说。差的不是点数，说点数就是骗人。
            push.enabled = false;
            push.disabledReason = words.pushAtStoryCap;
            break;
        case rules::BreakthroughBlock::NotEnough:
            push.enabled = false;
            // 只把字变灰，玩家会当成 bug；差多少必须写出来，这是他下一步的目标。
            push.disabledReason = std::string(words.pushShortPrefix) +
                                  std::to_string(need - state.cultivation) + words.pushShortSuffix;
            break;
        case rules::BreakthroughBlock::None:
            push.detail = chanceText(rules::breakthroughChance(state.realm, state.cultivation,
                                                               state.aptitude,
                                                               breakthroughPillBonus(state)));
            break;
    }
    items.push_back(std::move(push));

    ui::ListItem leave;
    leave.label = words.leaveLabel;
    items.push_back(std::move(leave));
    return items;
}

MeditateOutcome CultivationScene::bankMeditation(core::GameState& state, int days,
                                                 std::uint32_t seed, int sitePercent) {
    MeditateOutcome outcome;
    if (days <= 0) return outcome;
    outcome.days = days;

    const int aptitude = std::clamp(state.aptitude, 0, 100);
    // 此地的加成（百分比）乘在功法、灵根那一路（meditationEffectiveness）外面（契约
    // docs/interfaces-p3-ch08.md 1.4）。整数先乘后除：缺省 100 时乘了再除回来，与没有这一路时逐字相同。
    // 余数账记的是天数、差分按此刻的效率取：换一处坐时已进账的不回算，只有零头按新效率重折，差不到一点。
    const int effectiveness =
        meditationEffectiveness(state) * std::clamp(sitePercent, 0, rules::kMaxEffectiveness) / 100;
    // 效率为零（功法不契、洞府无灵气）时坐穿蒲团也是白坐：天数照过，一分不记，
    // 更不该往余数账上攒——攒了也永远兑不出来，只会让账面无限膨胀。
    if (effectiveness <= 0 || !rules::isValid(state.realm)) return outcome;

    // 存档里的余数账被改坏（或旧版本留下的别的语义）时就地归零：拿它去算
    // 天数会把 meditate 的入参撑出上限，而那是一条查不出来的错。
    int carry = state.cultivationRemainder;
    if (carry < 0 || carry > rules::kMaxMeditateDays) carry = 0;

    int remaining = days;
    while (remaining > 0) {
        // 累计天数顶到 meditate 的单次上限（十年）就换一次账。丢掉的只有
        // 不足一点的零头，十个游戏年才发生一次。
        if (carry >= rules::kMaxMeditateDays) carry = 0;

        // 分段点只由「离上限还差多少天」决定，与调用方一次传几天无关，
        // 换账之后也照旧用同一颗种子。两者合起来，长打坐与短打坐连跨过
        // 换账的那一刻都走在同一条曲线上——否则十年整这个边界会成为唯一
        // 一处「切法不同、收益不同」的缝。
        const int chunk = std::min(remaining, rules::kMaxMeditateDays - carry);
        outcome.cultivation +=
            settleMeditation(state, carry, chunk, aptitude, effectiveness, seed, outcome.insight);
        carry += chunk;
        remaining -= chunk;
    }

    state.cultivation += outcome.cultivation;
    return outcome;
}

MeditateOutcome CultivationScene::meditateFor(Application& app, int days) {
    core::GameState& state = app.state();
    const CultivationLexicon& words = cultivationLexicon(wordingStage(state));

    // 先把欠的日课补上，再算这一次静坐：两笔账落在同一颗种子的同一条曲线上，
    // 顺序反过来会让「先打坐再补账」与「先补账再打坐」算出不同的数。
    const int daily = settleDailyPractice(state);

    const MeditateOutcome outcome =
        bankMeditation(state, days, meditationSeed(state), sitePercent_);
    // 日历一律走这一个入口：灵田、掌天瓶这些按天走的系统全在那里结算，
    // 面板各推各的必然有人漏掉一项。
    app.advanceDays(days);
    // 这几天已经按静坐全额结过了，日课的水位跟着推过去，不再重复计一遍。
    // 用加法而不是直接赋成今天：上面 settleDailyPractice 留下的那点零头
    // （不足一块的几天）要原样留着，赋值会把它抹掉。
    if (state.lastPracticeDay > 0) state.lastPracticeDay += outcome.days;

    feedback_ = std::string(words.sitPrefix) + std::to_string(outcome.days) + words.sitMiddle +
                std::to_string(outcome.cultivation) + words.sitSuffix;
    if (outcome.cultivation == 0) {
        // 「什么也没得到」必须说清是「还没攒够」而不是「坏了」，否则玩家
        // 只会得出「打坐一日没用」这个错误结论，再也不点它。
        feedback_ += std::string("\n") + words.tooShort;
    }
    if (daily > 0) {
        feedback_ += std::string("\n") + words.dailyPracticePrefix + std::to_string(daily) +
                     words.dailyPracticeSuffix;
    }
    if (outcome.insight) {
        feedback_ += std::string("\n") + words.insight;
    }
    return outcome;
}

rules::BreakthroughAttempt CultivationScene::breakthrough(Application& app) {
    core::GameState& state = app.state();
    const PanelStage stage = wordingStage(state);
    const CultivationLexicon& words = cultivationLexicon(stage);
    rules::BreakthroughAttempt attempt;

    // 欠的日课先补上再判门槛。不补的话，玩家隔了半年回来，明明够了却被告知
    // 「火候未到」，只有再点一次打坐才追认——那是一条查不出来的怪事。
    settleDailyPractice(state);

    const std::string before = realmText(stage, state.realm);
    // **唯一入口**（rules::tryBreakthrough）：本作上限、剧情上限、修为门槛都在那里判，
    // 按不下去就连骰子都不摇、一分不扣。面板不另写一份上限判定（技术债 G-14）。
    attempt = rules::tryBreakthrough(state.realm, state.cultivation, state.realmCap,
                                     state.aptitude, breakthroughPillBonus(state),
                                     breakthroughSeed(state));
    switch (attempt.blocked) {
        case rules::BreakthroughBlock::StoryCap:
            // 瓶颈与运气差分开说：这一句不许与 notReady / breakFail 同一个意思。
            feedback_ = words.atStoryCap;
            return attempt;
        case rules::BreakthroughBlock::SeriesMax:
        case rules::BreakthroughBlock::NotEnough:
            // 界面上「点了没反应还掉修为」无从解释，玩家只会认为是 bug——所以规则层
            // 连骰子都不摇，这里只说一句为什么。
            feedback_ = words.notReady;
            return attempt;
        case rules::BreakthroughBlock::None:
            break;
    }
    if (attempt.success) {
        rules::Realm next{};
        if (rules::tryNext(state.realm, next)) state.realm = next;
        state.cultivation = std::max(0, state.cultivation - attempt.cultivationSpent);
        // 与剧情升境（Application 的 RealmAdvance）同一声：打坐面板突破成功也是一次升境，
        // 两条路一个响、一个不响，玩家会以为面板这一次没算数。
        app.engine().playSfx("realm_up");
        const RealmGrowth growth = applyRealmAttributes(state);
        feedback_ = std::string(words.breakSuccessPrefix) + before + words.breakSuccessMiddle +
                    realmText(stage, state.realm) + words.breakSuccessSuffix;
        if (growth.hp > 0 || growth.mp > 0) {
            // 属性涨了就必须说出来。境界推进在这次改动之前只改一个编号，玩家
            // 看不出自己强在哪儿；现在气血与法力真的涨了，那就把涨了多少写在
            // 同一条反馈里——状态栏的两根条子也会跟着动，两处对得上。
            //
            // 复用用词表里已有的 hpLabel / mpLabel，不新起字符串：凡人阶段的
            // 禁词扫描（tests/PanelTests.cpp）扫的是那张表，新起一条就是新开
            // 一条没人看着的缝，而「炼气三层」正是从这种缝里漏出去过一次的。
            feedback_ += std::string("\n") + words.hpLabel + "+" + std::to_string(growth.hp) +
                         "　" + words.mpLabel + "+" + std::to_string(growth.mp);
        }
        return attempt;
    }

    state.cultivation = std::max(0, state.cultivation - attempt.cultivationLost);
    if (attempt.backlash) {
        // 走火入魔是「失败且重伤」。只弹一句话而气血不动，玩家会把它当成
        // 普通失败；伤害只能记在 game 层——rules 不认识 hp。
        state.hp = std::max(1, state.hp / 2);
        feedback_ = std::string(words.backlashPrefix) +
                    std::to_string(attempt.cultivationLost) + words.backlashSuffix;
        return attempt;
    }
    feedback_ = std::string(words.breakFailPrefix) + std::to_string(attempt.cultivationLost) +
                words.breakFailSuffix;
    return attempt;
}

void CultivationScene::enterMain(const core::GameState& state, const ui::Theme& theme) {
    mode_ = Mode::Main;
    main_.reset();
    main_.setPageSize(listPageRows(theme));
    main_.setItems(buildMainItems(state));
}

void CultivationScene::enterDuration(const core::GameState& state, const ui::Theme& theme) {
    mode_ = Mode::Duration;
    const CultivationLexicon& words = cultivationLexicon(wordingStage(state));

    std::vector<ui::ListItem> items;
    for (const MeditateOption& option : meditateOptions(wordingStage(state))) {
        ui::ListItem item;
        item.label = option.label;
        item.detail = std::to_string(option.days) + " 日";
        items.push_back(std::move(item));
    }
    ui::ListItem back;
    back.label = words.backLabel;
    items.push_back(std::move(back));

    duration_.reset();
    duration_.setPageSize(listPageRows(theme));
    duration_.setItems(std::move(items));
}

void CultivationScene::onEnter(Application& app) {
    core::GameState& state = app.state();
    const CultivationLexicon& words = cultivationLexicon(wordingStage(state));

    // 打开面板就先把欠的日课结清。剧情整年整年地推日历（第 2 章五个段末合计
    // 一千一百来天），那些日子的功课不能等到玩家想起来点一次打坐才追认。
    const int daily = settleDailyPractice(state);

    feedback_ = words.enterHint;
    if (daily > 0) {
        feedback_ += std::string("\n") + words.dailyPracticePrefix + std::to_string(daily) +
                     words.dailyPracticeSuffix;
    }
    enterMain(state, app.theme());
}

bool CultivationScene::update(Application& app, double) {
    engine::Engine& engine = app.engine();

    if (mode_ == Mode::Duration) {
        if (duration_.update(engine)) {
            const int index = duration_.selection();
            // 与 enterDuration 取同一个阶段：面板是模态的，中途换不了旗标，
            // 但两处各取一次仍要保证取的是同一张表，否则下标就对不上了。
            const std::vector<MeditateOption>& options =
                meditateOptions(wordingStage(app.state()));
            if (index >= 0 && static_cast<std::size_t>(index) < options.size()) {
                meditateFor(app, options[static_cast<std::size_t>(index)].days);
            }
            // 选了「返回」也走这里：打坐完与放弃打坐都该回到主菜单。
            enterMain(app.state(), app.theme());
            return true;
        }
        if (duration_.cancelled()) enterMain(app.state(), app.theme());
        return true;
    }

    if (main_.update(engine)) {
        switch (main_.selection()) {
            case kActionMeditate:
                enterDuration(app.state(), app.theme());
                return true;
            case kActionBreakthrough:
                breakthrough(app);
                // 境界与修为都变了，菜单上的把握与差额得跟着重算。
                enterMain(app.state(), app.theme());
                return true;
            case kActionLeave:
            default:
                return false;
        }
    }
    if (main_.cancelled()) return false;
    return true;
}

void CultivationScene::renderStatus(Application& app, const engine::Rect& area) const {
    engine::Engine& engine = app.engine();
    const ui::Theme& theme = app.theme();
    const core::GameState& state = app.state();
    if (area.w <= 0 || area.h <= 0) return;

    const int row = theme.bodyFontSize + theme.lineSpacing;
    int y = area.y;

    const PanelStage stage = wordingStage(state);
    const CultivationLexicon& words = cultivationLexicon(stage);

    const rules::Calendar calendar{state.day};
    const auto line = [&](const std::string& textLine) {
        engine.drawText(textLine, area.x, y, theme.bodyFontSize, theme.paper, theme.bodyStyle);
        y += row;
    };
    line(std::string(words.realmLabel) + realmText(stage, state.realm));
    line(std::string(words.aptitudeLabel) + std::to_string(state.aptitude));
    if (const std::string root = spiritRootLine(state); !root.empty()) line(root);
    line(std::string(words.dateLabel) + "第 " + std::to_string(calendar.year()) + " 年 " +
         std::to_string(calendar.monthOfYear()) + " 月 " +
         std::to_string(calendar.dayOfMonth()) + " 日");

    // 修为条。已至上限时没有「下一关」可言，条画满并写明，不要画一条空条。
    const int need = rules::cultivationNeeded(state.realm);
    const bool capped = need < 0;
    const std::string progress =
        capped ? words.cultivationLabel + std::to_string(state.cultivation) + words.cappedNote
               : words.cultivationLabel + std::to_string(state.cultivation) + " / " +
                     std::to_string(need);
    const auto gauge = [&](const std::string& label, int current, int maximum,
                           const engine::Color& fill) {
        engine.drawText(label, area.x, y, theme.bodyFontSize, theme.paper, theme.bodyStyle);
        y += row;
        ui::drawGauge(engine, engine::Rect{area.x, y, area.w, theme.bodyFontSize / 2}, current,
                      maximum, fill, theme);
        y += row;
    };
    // 三根条的颜色收进主题（施工图 1.6）：修为金、气血翠、法力（气力）蓝。
    gauge(progress, capped ? 1 : state.cultivation, capped ? 1 : need, theme.gold);
    // 此地的打坐加成（灵眼之泉这类洞府灵脉）：写在修为条下面，玩家才知道「在这儿坐」划算在哪儿。
    if (const std::string site = siteBonusLine(state); !site.empty()) line(site);

    // 炼气期打坐一日不足一点修为，界面上就是「按了没动」。账其实记着，
    // 但不画出来玩家只会当按钮坏了。有零头才显示，免得平时多一行噪音。
    if (!capped && state.cultivationRemainder > 0) {
        line(words.carryPrefix + std::to_string(state.cultivationRemainder) + words.carrySuffix);
    }
    gauge(words.hpLabel + std::to_string(state.hp) + " / " + std::to_string(state.maxHp), state.hp,
          state.maxHp, theme.jade);
    gauge(words.mpLabel + std::to_string(state.mp) + " / " + std::to_string(state.maxMp), state.mp,
          state.maxMp, theme.azure);

    const engine::Rect note{area.x, y, area.w, std::max(0, area.y + area.h - y)};
    ui::drawTextBlock(engine, feedback_, note, theme.bodyFontSize, theme);
}

void CultivationScene::render(Application& app) {
    engine::Engine& engine = app.engine();
    const ui::Theme& theme = app.theme();

    const engine::Rect panel{kPanelX, kPanelY, kPanelW, kPanelH};
    const std::string title = "修炼";
    ui::drawScrim(engine, theme);
    ui::drawPanel(engine, panel, title, theme);

    const engine::Rect content = ui::panelContentArea(panel, title, theme);
    const int statusW = content.w * kStatusPercent / 100;
    // 状态栏让出左右内边距：字贴着金线外框写，读起来像是框没画完。
    renderStatus(app, engine::Rect{content.x + theme.padding, content.y + theme.lineSpacing,
                                   statusW - theme.padding * 2, content.h - theme.lineSpacing});

    const engine::Rect listArea{content.x + statusW, content.y, content.w - statusW, content.h};
    ui::drawRuleV(engine, static_cast<float>(listArea.x), static_cast<float>(content.y),
                  static_cast<float>(content.h), theme);
    if (mode_ == Mode::Duration) {
        duration_.render(engine, listArea, theme);
    } else {
        main_.render(engine, listArea, theme);
    }
}

int CultivationScene::listPageRows(const ui::Theme& theme) {
    // 标题带按「有标题」算：本面板的标题从来不为空，render() 也是这么摆的。
    return ui::listRowsThatFit(kPanelH - ui::panelTitleBandHeight(true, theme), theme);
}

}  // namespace fanren::game
