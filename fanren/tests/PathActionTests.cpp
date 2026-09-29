// 路径行动（契约 docs/interfaces-octo-pathactions.md；施工图 docs/octopath-overhaul.md 5.1）。
//
// 三组，外加一组「真数据的设计意图」：
//   · 规则层 —— 时段（when 全成立、until 无一成立）、挂不挂出来、门槛与钱的先后、
//               效果清单的次序、打探只发一次、切磋赢了才记；
//   · 加载器 —— 好文件读得进；形状的拒收表逐项一条负例；谓词与任务加载器同进同退；
//               引用（文案、物品、旗标、角色、地图、NPC、编成）逐类一条负例；
//   · 真数据 —— 第 1–5 章：种类按章开放、门槛够得着、价钱付得起、切磋奖励不压过本章的剧情切磋、
//               文案长度与禁词、第 5 章的章内先后按 when 推出来的节点判、pending 的待办闸门。
//
// 判据的形状（docs/README.md「判据自己会说谎」那张表逐条对过）：
//   · 每一条「不成立」都配一条同一处的「成立」，反之亦然；
//   · 真数据那一组先验分母：五章都读到了、每章条目数 > 0、文案真有内容；
//   · 预算与门槛写死成数字、出处写在旁边；数字从哪儿来（存档夹具）另有一条漂移检查——
//     夹具里的钱少了，这里的预算就不再是「玩家手里至少有的」，要红给人看。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/PathActions.h"
#include "core/rules/Realm.h"
#include "game/ShopScene.h"
#include "io/BattleLoader.h"
#include "io/DataLoader.h"
#include "io/PathActionLoader.h"
#include "io/QuestLoader.h"
#include "io/SaveFile.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::BagEntry;
using fanren::core::GameData;
using fanren::core::GameState;
using fanren::core::PathAction;
using fanren::core::PathActionKind;
using fanren::core::PathReveal;
using fanren::core::QuestCondition;
using fanren::rules::PathBlock;
using fanren::rules::PathEffect;
using fanren::rules::Realm;
using fanren::test::TempDir;
using Effect = PathEffect::Kind;
using Op = QuestCondition::Op;

constexpr const char* kSilver = fanren::rules::kPathCurrencyItemId;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "data" / "pathactions") && fs::exists(root / "maps" / "ch04_getang.tmj")) {
            return candidate;
        }
    }
    return ".";
}

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
}

QuestCondition flagAtLeast(std::string flag, int value) {
    return QuestCondition{Op::FlagAtLeast, std::move(flag), value};
}
QuestCondition flagEquals(std::string flag, int value) {
    return QuestCondition{Op::FlagEquals, std::move(flag), value};
}

// 三条手搭的条目：都挂在 m / npc 上，ch04.a 置上才出现、ch04.b 置上就收起。
PathAction inquiry(const std::string& id) {
    PathAction a;
    a.id = id;
    a.chapter = 4;
    a.kind = PathActionKind::Inquire;
    a.mapId = "m";
    a.npc = "npc";
    a.when = {flagAtLeast("ch04.a", 1)};
    a.until = {flagAtLeast("ch04.b", 1)};
    a.doneFlag = "ch04.path." + id;
    a.textKey = "t." + id;
    return a;
}

PathAction purchase(const std::string& id, int price) {
    PathAction a = inquiry(id);
    a.kind = PathActionKind::Purchase;
    a.goods = BagEntry{"pill_x", 2, 0};
    a.price = price;
    a.dealKey = "t.deal";
    a.poorKey = "t.poor";
    return a;
}

PathAction challenge(const std::string& id) {
    PathAction a = inquiry(id);
    a.kind = PathActionKind::Challenge;
    a.battleId = "b_x";
    a.winKey = "t.win";
    a.loseKey = "t.lose";
    a.rewardCultivation = 12;
    a.rewardItems = {BagEntry{"herb_y", 1, 5}};
    return a;
}

std::vector<Effect> kindsOf(const std::vector<PathEffect>& effects) {
    std::vector<Effect> kinds;
    for (const PathEffect& e : effects) kinds.push_back(e.kind);
    return kinds;
}

// 游戏层照单施加的样子（契约第 5 节）。测试里只用它来验「清单施加下去，存档是契约说的那样」。
void applyEffects(const std::vector<PathEffect>& effects, GameState& state) {
    for (const PathEffect& e : effects) {
        switch (e.kind) {
            case Effect::GiveItem: state.addItem(e.item.itemId, e.item.count, e.item.herbAge); break;
            case Effect::TakeItem: ASSERT_TRUE(state.removeItem(e.item.itemId, e.item.count)); break;
            case Effect::SetFlag: state.setFlag(e.flag); break;
            case Effect::GainCultivation: state.cultivation += e.amount; break;
            case Effect::Say:
            case Effect::RevealWeakness:
            case Effect::StartBattle: break;
        }
    }
}

// ===========================================================================
// 一、规则层
// ===========================================================================

TEST(PathActionRule, TheWindowNeedsEveryWhenAndNoUntil) {
    PathAction a = inquiry("x_dating");
    a.when.push_back(flagEquals("ch04.c", 2));
    a.until.push_back(flagAtLeast("ch04.d", 1));
    GameState s;
    EXPECT_FALSE(fanren::rules::pathWindowOpen(a, s)) << "什么都没置";
    s.setFlag("ch04.a");
    EXPECT_FALSE(fanren::rules::pathWindowOpen(a, s)) << "when 只成立了一半：它是 AND";
    s.setFlag("ch04.c", 2);
    EXPECT_TRUE(fanren::rules::pathWindowOpen(a, s)) << "when 全成立、until 一条也没成立";
    s.setFlag("ch04.d");
    EXPECT_FALSE(fanren::rules::pathWindowOpen(a, s)) << "until 的第二条成立就收起：它是 OR";
    s.setFlag("ch04.d", 0);
    EXPECT_TRUE(fanren::rules::pathWindowOpen(a, s));
    s.setFlag("ch04.b");
    EXPECT_FALSE(fanren::rules::pathWindowOpen(a, s)) << "until 的第一条成立也收起";
}

TEST(PathActionRule, AnEmptyWhenNeverOpensButAnEmptyUntilNeverCloses) {
    PathAction a = inquiry("x_dating");
    a.until.clear();
    GameState s;
    s.setFlag("ch04.a");
    EXPECT_TRUE(fanren::rules::pathWindowOpen(a, s)) << "空的 until 就是不收起";
    a.when.clear();
    EXPECT_FALSE(fanren::rules::pathWindowOpen(a, s))
        << "空的 when 按 AND 恒真，等于一开局就挂着——那是漏写，规则层不信它";
}

TEST(PathActionRule, AnInquiryStaysAfterItIsDoneButPurchasesAndWonChallengesFold) {
    GameState s;
    s.setFlag("ch04.a");
    const PathAction ask = inquiry("x_dating");
    const PathAction buy = purchase("x_qiugou", 5);
    const PathAction duel = challenge("x_qiecuo");
    for (const PathAction* a : {&ask, &buy, &duel}) {
        EXPECT_TRUE(fanren::rules::pathActionShown(*a, s)) << a->id << " 还没做过，该挂着";
        s.setFlag(a->doneFlag);
    }
    EXPECT_TRUE(fanren::rules::pathActionShown(ask, s)) << "打探做过仍挂着：再按一次是重看那段情报";
    EXPECT_FALSE(fanren::rules::pathActionShown(buy, s)) << "求购每条只买一次";
    EXPECT_FALSE(fanren::rules::pathActionShown(duel, s)) << "赢过的切磋不再挂";
}

TEST(PathActionRule, APendingChallengeIsNeverShown) {
    GameState s;
    s.setFlag("ch04.a");
    PathAction duel = challenge("x_qiecuo");
    EXPECT_TRUE(fanren::rules::pathActionShown(duel, s));
    duel.pending = true;
    EXPECT_FALSE(fanren::rules::pathActionShown(duel, s))
        << "编成还没建好：挂出来就是一个按下去什么也打不起来的选项";
}

TEST(PathActionRule, OneNpcListsInquiriesThenPurchasesThenChallengesAndNobodyElses) {
    std::vector<PathAction> all = {challenge("a_qiecuo"), purchase("b_qiugou", 1), inquiry("c_dating"),
                                   inquiry("b_dating")};
    PathAction elsewhere = inquiry("d_dating");
    elsewhere.npc = "npc_other";
    PathAction otherMap = inquiry("e_dating");
    otherMap.mapId = "m2";
    PathAction closed = inquiry("f_dating");
    closed.when = {flagAtLeast("ch04.never", 1)};
    all.push_back(elsewhere);
    all.push_back(otherMap);
    all.push_back(closed);

    GameState s;
    s.setFlag("ch04.a");
    std::vector<std::string> ids;
    for (const PathAction* a : fanren::rules::pathActionsAt(all, s, "m", "npc")) ids.push_back(a->id);
    EXPECT_EQ(ids, (std::vector<std::string>{"b_dating", "c_dating", "b_qiugou", "a_qiecuo"}))
        << "打探 → 求购 → 切磋，同类按 id；别人身上的、别的图上的、时段没开的都不在";
}

TEST(PathActionRule, TheRealmGateRefusesBelowTheThresholdAndOpensAtIt) {
    PathAction ask = inquiry("x_dating");
    ask.minRealm = Realm::QiRefining7;
    ask.refuseKey = "t.refuse";
    GameState s;
    s.realm = Realm::QiRefining6;
    fanren::rules::PathVerdict verdict = fanren::rules::pathActionVerdict(ask, s);
    EXPECT_EQ(verdict.block, PathBlock::RealmTooLow);
    EXPECT_EQ(verdict.reasonKey, "t.refuse") << "阅历不足时说的就是条目自己那一句";
    EXPECT_FALSE(verdict.allowed());

    s.realm = Realm::QiRefining7;
    verdict = fanren::rules::pathActionVerdict(ask, s);
    EXPECT_TRUE(verdict.allowed()) << "门槛是 ≥：正好到就谈得成";
    EXPECT_TRUE(verdict.reasonKey.empty());
    s.realm = Realm::FoundationEarly;
    EXPECT_TRUE(fanren::rules::pathActionVerdict(ask, s).allowed()) << "筑基（21）高过炼气七层（7）";
}

TEST(PathActionRule, MoneyIsCheckedAfterTheRealmAndOnlyForPurchases) {
    PathAction buy = purchase("x_qiugou", 12);
    GameState s;
    s.addItem(kSilver, 11);
    fanren::rules::PathVerdict verdict = fanren::rules::pathActionVerdict(buy, s);
    EXPECT_EQ(verdict.block, PathBlock::NotEnoughMoney) << "差一块也不行";
    EXPECT_EQ(verdict.reasonKey, "t.poor");
    s.addItem(kSilver, 1);
    EXPECT_TRUE(fanren::rules::pathActionVerdict(buy, s).allowed()) << "正好十二块，买得起";

    buy.minRealm = Realm::QiRefining1;
    buy.refuseKey = "t.refuse";
    ASSERT_TRUE(s.removeItem(kSilver, 12));
    EXPECT_EQ(fanren::rules::pathActionVerdict(buy, s).block, PathBlock::RealmTooLow)
        << "阅历不到，对方连价都不开，谈不上钱够不够";

    const PathAction ask = inquiry("x_dating");
    EXPECT_TRUE(fanren::rules::pathActionVerdict(ask, s).allowed()) << "打探不要钱：身上一块没有也问得";
}

TEST(PathActionRule, TheFirstInquiryCarriesEveryEffectAndMarksItselfDoneLast) {
    PathAction ask = inquiry("x_dating");
    ask.reveals = {PathReveal{"role_a", "火"}, PathReveal{"role_b", "剑"}};
    ask.gives = {BagEntry{"herb_y", 2, 3}};
    ask.setFlags = {"ch04.path.zhi"};
    GameState s;
    s.setFlag("ch04.a");
    const std::vector<PathEffect> effects = fanren::rules::pathActionEffects(ask, s);
    EXPECT_EQ(kindsOf(effects), (std::vector<Effect>{Effect::Say, Effect::RevealWeakness, Effect::RevealWeakness,
                                                     Effect::GiveItem, Effect::SetFlag, Effect::SetFlag}));
    ASSERT_EQ(effects.size(), 6u);
    EXPECT_EQ(effects[0].textKey, "t.x_dating");
    EXPECT_EQ(effects[1].reveal.roleId, "role_a");
    EXPECT_EQ(effects[1].reveal.category, "火");
    EXPECT_EQ(effects[2].reveal.category, "剑");
    EXPECT_EQ(effects[3].item.itemId, "herb_y");
    EXPECT_EQ(effects[3].item.count, 2);
    EXPECT_EQ(effects[3].item.herbAge, 3);
    EXPECT_EQ(effects[4].flag, "ch04.path.zhi");
    EXPECT_EQ(effects[5].flag, ask.doneFlag) << "记做过永远在最后：前面哪一步没施加成，这一条就不算做过";
}

TEST(PathActionRule, ADoneInquiryOnlyRepeatsItsWords) {
    PathAction ask = inquiry("x_dating");
    ask.reveals = {PathReveal{"role_a", "火"}};
    ask.gives = {BagEntry{"herb_y", 2, 3}};
    ask.setFlags = {"ch04.path.zhi"};
    GameState s;
    s.setFlag("ch04.a");
    s.setFlag(ask.doneFlag);
    const std::vector<PathEffect> effects = fanren::rules::pathActionEffects(ask, s);
    ASSERT_EQ(effects.size(), 1u) << "按两下 E 不许多拿一份物件、多揭一回破绽";
    EXPECT_EQ(effects[0].kind, Effect::Say);
    EXPECT_EQ(effects[0].textKey, ask.textKey);
}

TEST(PathActionRule, APurchasePaysExactlyThePriceInSilverThenHandsOverTheGoods) {
    const PathAction buy = purchase("x_qiugou", 24);
    const std::vector<PathEffect> effects = fanren::rules::pathActionEffects(buy, GameState{});
    ASSERT_EQ(kindsOf(effects),
              (std::vector<Effect>{Effect::Say, Effect::TakeItem, Effect::GiveItem, Effect::SetFlag}));
    EXPECT_EQ(effects[0].textKey, "t.deal") << "成交说的是 deal_key，不是开价那一句";
    EXPECT_EQ(effects[1].item.itemId, kSilver);
    EXPECT_EQ(effects[1].item.count, 24);
    EXPECT_EQ(effects[2].item.itemId, "pill_x");
    EXPECT_EQ(effects[2].item.count, 2);
    EXPECT_EQ(effects[3].flag, buy.doneFlag);
}

TEST(PathActionRule, AChallengeOpensWithWordsThenTheBattle) {
    const PathAction duel = challenge("x_qiecuo");
    const std::vector<PathEffect> effects = fanren::rules::pathActionEffects(duel, GameState{});
    ASSERT_EQ(kindsOf(effects), (std::vector<Effect>{Effect::Say, Effect::StartBattle}));
    EXPECT_EQ(effects[0].textKey, duel.textKey);
    EXPECT_EQ(effects[1].battleId, "b_x");
}

TEST(PathActionRule, WinningPaysOnceAndLosingLeavesTheChallengeOpen) {
    PathAction duel = challenge("x_qiecuo");
    const std::vector<PathEffect> won = fanren::rules::challengeResultEffects(duel, true);
    ASSERT_EQ(kindsOf(won),
              (std::vector<Effect>{Effect::Say, Effect::GainCultivation, Effect::GiveItem, Effect::SetFlag}));
    EXPECT_EQ(won[0].textKey, "t.win");
    EXPECT_EQ(won[1].amount, 12);
    EXPECT_EQ(won[2].item.itemId, "herb_y");
    EXPECT_EQ(won[3].flag, duel.doneFlag);

    const std::vector<PathEffect> lost = fanren::rules::challengeResultEffects(duel, false);
    ASSERT_EQ(kindsOf(lost), (std::vector<Effect>{Effect::Say})) << "输了不记、不发：歇好了可以再来";
    EXPECT_EQ(lost[0].textKey, "t.lose");

    duel.rewardCultivation = 0;
    EXPECT_EQ(kindsOf(fanren::rules::challengeResultEffects(duel, true)),
              (std::vector<Effect>{Effect::Say, Effect::GiveItem, Effect::SetFlag}))
        << "修为为 0 就不出一条加 0 的效果";
    EXPECT_TRUE(fanren::rules::challengeResultEffects(inquiry("x_dating"), true).empty())
        << "不是切磋，谈不上收场";
}

TEST(PathActionRule, RevealedWeaknessesComeOnlyFromDoneInquiriesSortedAndOnce) {
    PathAction first = inquiry("a_dating");
    first.reveals = {PathReveal{"wolf", "拳"}, PathReveal{"bandit", "剑"}};
    PathAction second = inquiry("b_dating");
    second.reveals = {PathReveal{"wolf", "拳"}};
    PathAction untouched = inquiry("c_dating");
    untouched.reveals = {PathReveal{"boss", "火"}};
    const std::vector<PathAction> all = {first, second, untouched};
    GameState s;
    EXPECT_TRUE(fanren::rules::pathRevealedWeaknesses(all, s).empty()) << "一条也没问过";
    s.setFlag(first.doneFlag);
    s.setFlag(second.doneFlag);
    const std::vector<PathReveal> known = fanren::rules::pathRevealedWeaknesses(all, s);
    ASSERT_EQ(known.size(), 2u) << "同一处破绽两条打探都揭了，只算一次；没问过的那条不算";
    EXPECT_EQ(known[0].roleId, "bandit");
    EXPECT_EQ(known[1].roleId, "wolf");
}

TEST(PathActionRule, PendingChallengesAreListedByChapterThenId) {
    PathAction late = challenge("b_qiecuo");
    late.chapter = 5;
    late.pending = true;
    PathAction early = challenge("z_qiecuo");
    early.pending = true;
    PathAction built = challenge("a_qiecuo");
    const std::vector<PathAction> all = {late, built, early, inquiry("a_dating")};
    std::vector<std::string> ids;
    for (const PathAction* a : fanren::rules::pendingChallenges(all)) ids.push_back(a->id);
    EXPECT_EQ(ids, (std::vector<std::string>{"z_qiecuo", "b_qiecuo"}));
}

TEST(PathActionRule, ApplyingTheListsLeavesTheStateTheContractDescribes) {
    PathAction buy = purchase("x_qiugou", 12);
    PathAction ask = inquiry("x_dating");
    ask.gives = {BagEntry{"herb_y", 1, 3}};
    ask.setFlags = {"ch04.path.zhi"};
    GameState s;
    s.setFlag("ch04.a");
    s.addItem(kSilver, 20);

    // 算效果不改存档：签名是 const，这里再看一眼数没变。
    const GameState before = s;
    (void)fanren::rules::pathActionEffects(buy, s);
    (void)fanren::rules::pathActionVerdict(buy, s);
    (void)fanren::rules::pathActionsAt({buy, ask}, s, "m", "npc");
    EXPECT_EQ(s.flags, before.flags);
    EXPECT_EQ(s.itemCount(kSilver), 20);

    applyEffects(fanren::rules::pathActionEffects(buy, s), s);
    EXPECT_EQ(s.itemCount(kSilver), 8) << "二十块付了十二块";
    EXPECT_EQ(s.itemCount("pill_x"), 2);
    EXPECT_FALSE(fanren::rules::pathActionShown(buy, s)) << "买过就收起";

    applyEffects(fanren::rules::pathActionEffects(ask, s), s);
    applyEffects(fanren::rules::pathActionEffects(ask, s), s);
    EXPECT_EQ(s.itemCount("herb_y"), 1) << "问两遍只给一株";
    EXPECT_EQ(s.flag("ch04.path.zhi"), 1);
}

TEST(PathActionRule, TheCurrencyIsTheSameItemTheShopsCount) {
    EXPECT_STREQ(fanren::rules::kPathCurrencyItemId, fanren::game::kSpiritStoneItemId)
        << "core 引不到 game，这个 id 写了两份；两份必须是同一件东西";
}

// ===========================================================================
// 二、加载器
// ===========================================================================
//
// 三条写全了的条目（第 4 章）。负例在它们上面各改一处，改的原文要在里面恰好出现一次。
const char* const kInquiry =
    R"({"id": "probe_dating", "kind": "inquire", "map": "ch04_getang", "npc": "npc_li_feiyu", )"
    R"("when": [{"flag": "ch04.a", "op": ">=", "value": 1}], )"
    R"("until": [{"flag": "ch04.b", "op": ">=", "value": 1}, {"flag": "ch04.done", "op": ">=", "value": 1}], )"
    R"("realm": 5, "refuse_key": "t.refuse", "done_flag": "ch04.path.probe_dating", "text_key": "t.text", )"
    R"("reveal": [{"role": "r1", "category": "火"}], "give": [{"item_id": "i1", "count": 2, "herb_age": 3}], )"
    R"("set_flags": ["ch04.path.zhi"], "origin": "o", "note": "n"})";
const char* const kPurchase =
    R"({"id": "probe_qiugou", "kind": "purchase", "map": "ch04_getang", "npc": "npc_li_feiyu", )"
    R"("when": [{"flag": "ch03.done", "op": ">=", "value": 1}], )"
    R"("until": [{"flag": "ch04.done", "op": ">=", "value": 1}], )"
    R"("done_flag": "ch04.path.probe_qiugou", "text_key": "t.offer", )"
    R"("item_id": "i2", "count": 3, "herb_age": 20, "price": 24, "deal_key": "t.deal", "poor_key": "t.poor"})";
const char* const kChallenge =
    R"({"id": "probe_qiecuo", "kind": "challenge", "map": "ch04_getang", "npc": "npc_wang_juechu", )"
    R"("when": [{"flag": "ch04.a", "op": "==", "value": 2}], )"
    R"("until": [{"flag": "ch04.done", "op": ">=", "value": 1}], )"
    R"("done_flag": "ch04.path.probe_qiecuo", "text_key": "t.dare", "battle": "b_probe", "pending": true, )"
    R"("win_key": "t.win", "lose_key": "t.lose", )"
    R"("reward": {"cultivation": 25, "items": [{"item_id": "i3", "count": 1}]}})";

std::string chapterFile(const std::string& entries, int chapter = 4,
                        const std::string& id = "pathactions_ch04") {
    return R"({"id": ")" + id + R"(", "name": "探针", "chapter": )" + std::to_string(chapter) +
           R"(, "entries": [)" + entries + "]}";
}

std::string replacedOnce(const std::string& text, const std::string& from, const std::string& to) {
    const std::size_t at = text.find(from);
    EXPECT_NE(at, std::string::npos) << "夹具里找不到要改的原文：" << from;
    EXPECT_EQ(text.find(from, at + 1), std::string::npos) << "要改的原文出现了不止一次：" << from;
    if (at == std::string::npos) return text;
    return text.substr(0, at) + to + text.substr(at + from.size());
}

fanren::core::Result<std::vector<PathAction>> loadOne(const TempDir& tmp, const std::string& content,
                                                     const std::string& stem = "ch04") {
    const fs::path path = tmp.path() / (stem + ".json");
    writeFile(path, content);
    return fanren::io::loadPathActionFile(path.string());
}

TEST(PathActionLoader, AWellFormedChapterLoadsEveryField) {
    TempDir tmp("fanren_pathaction_good");
    const auto loaded = loadOne(tmp, chapterFile(std::string(kInquiry) + ", " + kPurchase + ", " + kChallenge));
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_EQ(loaded.value.size(), 3u);

    const PathAction& ask = loaded.value[0];
    EXPECT_EQ(ask.id, "probe_dating");
    EXPECT_EQ(ask.chapter, 4);
    EXPECT_EQ(ask.kind, PathActionKind::Inquire);
    EXPECT_EQ(ask.mapId, "ch04_getang");
    EXPECT_EQ(ask.npc, "npc_li_feiyu");
    ASSERT_EQ(ask.when.size(), 1u);
    EXPECT_EQ(ask.when[0].op, Op::FlagAtLeast);
    ASSERT_EQ(ask.until.size(), 2u);
    EXPECT_EQ(ask.until[1].subject, "ch04.done");
    EXPECT_EQ(ask.minRealm, Realm::QiRefining5);
    EXPECT_EQ(ask.refuseKey, "t.refuse");
    EXPECT_EQ(ask.doneFlag, "ch04.path.probe_dating");
    ASSERT_EQ(ask.reveals.size(), 1u);
    EXPECT_EQ(ask.reveals[0].roleId, "r1");
    EXPECT_EQ(ask.reveals[0].category, "火");
    ASSERT_EQ(ask.gives.size(), 1u);
    EXPECT_EQ(ask.gives[0].herbAge, 3);
    EXPECT_EQ(ask.setFlags, (std::vector<std::string>{"ch04.path.zhi"}));
    EXPECT_EQ(ask.origin, "o");

    const PathAction& buy = loaded.value[1];
    EXPECT_EQ(buy.kind, PathActionKind::Purchase);
    EXPECT_EQ(buy.minRealm, Realm::Mortal) << "没写 realm = 没有门槛";
    EXPECT_EQ(buy.goods.itemId, "i2");
    EXPECT_EQ(buy.goods.count, 3);
    EXPECT_EQ(buy.goods.herbAge, 20);
    EXPECT_EQ(buy.price, 24);
    EXPECT_EQ(buy.dealKey, "t.deal");
    EXPECT_EQ(buy.poorKey, "t.poor");
    EXPECT_EQ(buy.when[0].subject, "ch03.done") << "上一章收尾也算锚在本章";

    const PathAction& duel = loaded.value[2];
    EXPECT_EQ(duel.kind, PathActionKind::Challenge);
    EXPECT_EQ(duel.when[0].op, Op::FlagEquals);
    EXPECT_EQ(duel.battleId, "b_probe");
    EXPECT_TRUE(duel.pending);
    EXPECT_EQ(duel.rewardCultivation, 25);
    ASSERT_EQ(duel.rewardItems.size(), 1u);
    EXPECT_EQ(duel.rewardItems[0].itemId, "i3");
}

struct ShapeCase {
    const char* name;
    const char* entry;
    const char* from;
    const char* to;
    const char* needle;
};

TEST(PathActionLoader, EveryShapeTheContractRejectsIsRejectedWithItsReason) {
    const std::vector<ShapeCase> cases = {
        {"条目字段拼错", kInquiry, R"("origin": "o")", R"("origin": "o", "colour": 1)", "不认识的字段 colour"},
        {"kind 写成表外的词", kInquiry, R"("kind": "inquire")", R"("kind": "ask")", "kind 只认"},
        {"id 的尾巴与种类对不上", kInquiry, R"("id": "probe_dating")", R"("id": "probe_qiugou")", "id 的形状"},
        {"id 带大写", kInquiry, R"("id": "probe_dating")", R"("id": "Probe_dating")", "id 的形状"},
        {"缺 npc", kInquiry, R"("npc": "npc_li_feiyu", )", "", "map 与 npc"},
        {"when 为空", kInquiry, R"("when": [{"flag": "ch04.a", "op": ">=", "value": 1}])", R"("when": [])",
         "when 必须是非空数组"},
        {"until 漏了本章收尾", kInquiry, R"(, {"flag": "ch04.done", "op": ">=", "value": 1}])", "]",
         "until 里必须有"},
        {"when 没锚在本章", kInquiry, R"("when": [{"flag": "ch04.a")", R"("when": [{"flag": "ch03.a")",
         "没有锚在本章"},
        {"when 只要一个 == 0（没置过也成立）", kInquiry, R"("when": [{"flag": "ch04.a", "op": ">=", "value": 1}])",
         R"("when": [{"flag": "ch04.a", "op": "==", "value": 0}])", "没有锚在本章"},
        {"realm 不是境界编号", kInquiry, R"("realm": 5)", R"("realm": 15)", "realm 必须是合法的境界编号"},
        {"有门槛没回绝", kInquiry, R"("refuse_key": "t.refuse", )", "", "就必须写 refuse_key"},
        {"没门槛却写回绝", kInquiry, R"("realm": 5, )", "", "永远说不出口"},
        {"done_flag 与 id 对不上", kInquiry, R"("done_flag": "ch04.path.probe_dating")",
         R"("done_flag": "ch04.probe_dating")", "done_flag 必须是 ch04.path.probe_dating"},
        {"缺 text_key", kInquiry, R"("text_key": "t.text", )", "", "缺少 text_key"},
        {"破绽类别是表外的词", kInquiry, R"("category": "火")", R"("category": "laser")", "不是十种攻击类别之一"},
        {"reveal 多一个字段", kInquiry, R"("category": "火")", R"("category": "火", "extra": 1)",
         "只认 {role, category}"},
        {"give 的 count 为 0", kInquiry, R"("count": 2)", R"("count": 0)", "count 必须是 ≥ 1 的整数"},
        {"give 的年份为负", kInquiry, R"("herb_age": 3)", R"("herb_age": -1)", "herb_age 必须是 ≥ 0 的整数"},
        {"打探去置剧情旗标", kInquiry, R"(["ch04.path.zhi"])", R"(["ch04.kaizhan"])", "只许写本章的路径行动旗标"},
        {"打探去置别章的路径旗标", kInquiry, R"(["ch04.path.zhi"])", R"(["ch05.path.zhi"])",
         "只许写本章的路径行动旗标"},
        {"置的旗标就是自己的 done_flag", kInquiry, R"(["ch04.path.zhi"])", R"(["ch04.path.probe_dating"])",
         "与 done_flag 或前一项重复"},
        {"打探写了求购的字段", kInquiry, R"("origin": "o")", R"("origin": "o", "price": 5)", "不认识的字段 price"},
        {"求购白送", kPurchase, R"("price": 24)", R"("price": 0)", "price 必须是 ≥ 1"},
        {"求购缺成交那一句", kPurchase, R"("deal_key": "t.deal", )", "", "缺少 deal_key"},
        {"求购缺钱不够那一句", kPurchase, R"(, "poor_key": "t.poor")", "", "缺少 poor_key"},
        {"求购的件数为 0", kPurchase, R"("count": 3)", R"("count": 0)", "求购的 count 必须是"},
        {"求购缺物品", kPurchase, R"("item_id": "i2", )", "", "求购缺少 item_id"},
        {"求购写了打探的附带效果", kPurchase, R"("price": 24)", R"("price": 24, "give": [])", "不认识的字段 give"},
        {"切磋缺编成", kChallenge, R"("battle": "b_probe", )", "", "缺少 battle"},
        {"pending 不是布尔", kChallenge, R"("pending": true)", R"("pending": "yes")", "pending 必须是 true / false"},
        {"切磋缺奖励", kChallenge, R"(, "reward": {"cultivation": 25, "items": [{"item_id": "i3", "count": 1}]})",
         "", "缺少 reward"},
        {"切磋赢了什么也没有", kChallenge, R"({"cultivation": 25, "items": [{"item_id": "i3", "count": 1}]})", "{}",
         "至少给一样"},
        {"奖励写了表外字段", kChallenge, R"("cultivation": 25)", R"("cultivation": 25, "silver": 3)",
         "只认 cultivation / items"},
        {"奖励修为为负", kChallenge, R"("cultivation": 25)", R"("cultivation": -1)", "cultivation 必须是 ≥ 0"},
    };
    TempDir tmp("fanren_pathaction_shape");
    // 先验：没改过的三条本身读得进。不然下面每一条「报了」都可能只是底子就坏了。
    ASSERT_TRUE(loadOne(tmp, chapterFile(std::string(kInquiry) + ", " + kPurchase + ", " + kChallenge)).ok);
    for (const ShapeCase& c : cases) {
        const auto loaded = loadOne(tmp, chapterFile(replacedOnce(c.entry, c.from, c.to)));
        EXPECT_FALSE(loaded.ok) << c.name << "：该拒收却读进来了";
        EXPECT_NE(loaded.error.find(c.needle), std::string::npos)
            << c.name << "：拒收的理由不对，实际是 " << loaded.error;
    }
}

TEST(PathActionLoader, FileLevelShapesAreRejected) {
    TempDir tmp("fanren_pathaction_file");
    const std::string good = chapterFile(kInquiry);
    const auto baseline = loadOne(tmp, good);
    ASSERT_TRUE(baseline.ok) << baseline.error;

    auto loaded = loadOne(tmp, good, "ch05");
    EXPECT_FALSE(loaded.ok);
    EXPECT_NE(loaded.error.find("文件名必须是 ch04.json"), std::string::npos) << loaded.error;

    loaded = loadOne(tmp, chapterFile(kInquiry, 4, "pathactions_ch4"));
    EXPECT_NE(loaded.error.find("顶层 id 必须是 pathactions_ch04"), std::string::npos) << loaded.error;

    loaded = loadOne(tmp, chapterFile(kInquiry, 0));
    EXPECT_NE(loaded.error.find("chapter 必须是 1–14"), std::string::npos) << loaded.error;

    loaded = loadOne(tmp, chapterFile(""));
    EXPECT_NE(loaded.error.find("entries 必须是非空数组"), std::string::npos) << loaded.error;

    loaded = loadOne(tmp, replacedOnce(good, R"("name": "探针")", R"("name": "探针", "extra": 1)"));
    EXPECT_NE(loaded.error.find("有不认识的字段 extra"), std::string::npos) << loaded.error;

    loaded = loadOne(tmp, chapterFile(std::string(kInquiry) + ", " + kInquiry));
    EXPECT_NE(loaded.error.find("条目 id 在本章内重复：probe_dating"), std::string::npos) << loaded.error;
}

// 只查形状的那一层（loadGameData 走它）：临时根目录里只有 data/pathactions、没有 maps/ 与 flags.json，
// 照样读得进；查引用的那一层在同一棵树上就该报「读不到」。两层对同一章两个文件都要拒收。
TEST(PathActionLoader, TheShapeOnlyDirectoryReaderNeedsNeitherMapsNorFlags) {
    TempDir tmp("fanren_pathaction_dir");
    const fs::path dir = tmp.path() / "data" / "pathactions";
    writeFile(dir / "ch04.json", chapterFile(std::string(kInquiry) + ", " + kPurchase));
    const auto shapes = fanren::io::loadPathActionDir(dir.string());
    ASSERT_TRUE(shapes.ok) << shapes.error;
    ASSERT_EQ(shapes.value.size(), 2u);
    EXPECT_EQ(shapes.value[0].id, "probe_dating");
    EXPECT_EQ(shapes.value[1].id, "probe_qiugou");

    const auto withReferences = fanren::io::loadPathActions((tmp.path() / "data").string(), GameData{});
    EXPECT_FALSE(withReferences.ok) << "查引用的那一层没有 flags.json 也读进来了";
    EXPECT_NE(withReferences.error.find("旗标登记表读不到"), std::string::npos) << withReferences.error;

    const auto missing = fanren::io::loadPathActionDir((tmp.path() / "nowhere").string());
    ASSERT_TRUE(missing.ok) << missing.error;
    EXPECT_TRUE(missing.value.empty()) << "目录不存在 = 0 条";

    writeFile(dir / "more" / "ch04.json", chapterFile(kChallenge));
    const auto twice = fanren::io::loadPathActionDir(dir.string());
    EXPECT_FALSE(twice.ok);
    EXPECT_NE(twice.error.find("同属第 4 章"), std::string::npos) << twice.error;
}

// 谓词：契约写的是「照任务系统那套」，所以同一个写法两个加载器必须同进同退。
// 那边改了一条规则而这边没跟，这一条当场红。
TEST(PathActionLoader, PredicatesAreAcceptedExactlyWhenTheQuestLoaderAcceptsThem) {
    const std::vector<std::pair<std::string, bool>> predicates = {
        {R"({"flag": "ch04.x", "op": ">=", "value": 1})", true},
        {R"({"flag": "ch04.x", "op": "==", "value": 0})", true},
        {R"({"flag": "ch04.x", "op": "==", "value": 2})", true},
        {R"({"item": "i1", "op": ">=", "value": 2})", true},
        {R"({"flag": "ch04.x", "item": "i1", "op": ">=", "value": 1})", false},
        {R"({"op": ">=", "value": 1})", false},
        {R"({"flag": "ch04.x", "op": ">", "value": 1})", false},
        {R"({"flag": "ch04.x", "op": ">=", "value": true})", false},
        {R"({"flag": "ch04.x", "op": ">=", "value": 1.0})", false},
        {R"({"flag": "ch04.x", "op": ">=", "value": "1"})", false},
        {R"({"flag": "ch04.x", "op": ">=", "value": 0})", false},
        {R"({"flag": "ch04.x", "op": "==", "value": -1})", false},
        {R"({"item": "i1", "op": "==", "value": 1})", false},
        {R"({"item": "i1", "op": ">=", "value": 0})", false},
        {R"({"flag": "ch04.x", "op": ">=", "val": 1})", false},
        {R"({"flag": "", "op": ">=", "value": 1})", false},
    };
    TempDir tmp("fanren_pathaction_pred");
    for (const auto& [predicate, accepted] : predicates) {
        const std::string quest = R"({"id": "q_probe", "name": "探针", "chapter": 4, "kind": "side", )"
                                  R"("title_key": "t", "summary_key": "t", "accept": [)" + predicate +
                                  R"(], "steps": [{"id": "s1", "text_key": "t", "done": [{"flag": "f", "op": ">=", "value": 1}]}], )"
                                  R"("complete": [{"flag": "f", "op": ">=", "value": 1}]})";
        writeFile(tmp.path() / "q_probe.json", quest);
        const bool questOk = fanren::io::loadQuestFile((tmp.path() / "q_probe.json").string()).ok;
        const std::string entry = replacedOnce(kInquiry, R"("when": [{"flag": "ch04.a", "op": ">=", "value": 1}])",
                                               R"("when": [{"flag": "ch04.a", "op": ">=", "value": 1}, )" +
                                                   predicate + "]");
        const bool pathOk = loadOne(tmp, chapterFile(entry)).ok;
        EXPECT_EQ(questOk, accepted) << "任务加载器对 " << predicate << " 的判断变了，本表要跟着改";
        EXPECT_EQ(pathOk, questOk) << "同一个谓词两个加载器一进一退：" << predicate;
    }
}

// 引用：用一棵临时的资产根（data/ 与 maps/ 同级），地图抄真的第 4 章各堂。
// 图块集一并整目录抄过去：loadTileMap 要核 tileset 引用的 .tsj 真的在。
class PathActionReferences : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = tmp_.path();
        const fs::path maps = fs::path(assetRoot()) / "maps";
        std::error_code ec;
        fs::create_directories(root_ / "maps", ec);
        fs::copy_file(maps / "ch04_getang.tmj", root_ / "maps" / "ch04_getang.tmj",
                      fs::copy_options::overwrite_existing, ec);
        ASSERT_FALSE(ec) << "抄地图失败：" << ec.message();
        fs::copy(maps / "tilesets", root_ / "maps" / "tilesets", fs::copy_options::recursive, ec);
        ASSERT_FALSE(ec) << "抄图块集失败：" << ec.message();
        for (const char* key : {"t.refuse", "t.text", "t.offer", "t.deal", "t.poor", "t.dare", "t.win", "t.lose"}) {
            data_.text[key] = "有字";
        }
        for (const char* item : {"i1", "i2", "i3"}) data_.items[item].id = item;
        data_.roles["r1"].id = "r1";
        flags_ = {"ch03.done", "ch04.a", "ch04.b", "ch04.done", "ch04.path.probe_dating", "ch04.path.probe_qiugou",
                  "ch04.path.probe_qiecuo", "ch04.path.zhi"};
        entries_ = {kInquiry, kPurchase, kChallenge};
    }

    fanren::core::Result<std::vector<PathAction>> load() {
        std::string flags = "{";
        for (const std::string& flag : flags_) flags += (flags.size() > 1 ? ", \"" : "\"") + flag + "\": \"x\"";
        writeFile(root_ / "data" / "flags.json", flags + "}");
        std::string joined;
        for (const std::string& entry : entries_) joined += (joined.empty() ? "" : ", ") + entry;
        writeFile(root_ / "data" / "pathactions" / "ch04.json", chapterFile(joined));
        return fanren::io::loadPathActions((root_ / "data").string(), data_);
    }

    void writeBattle(bool fatal, int cultivation) {
        writeFile(root_ / "data" / "battles" / "b_probe.json",
                  R"({"id": "b_probe", "name": "探针", "chapter": 4, "can_escape": true, "defeat_is_fatal": )" +
                      std::string(fatal ? "true" : "false") +
                      R"(, "units": [{"role_id": "r1", "x": 3, "y": 3, "faction": "enemy"}], )"
                      R"("rewards": {"cultivation": )" +
                      std::to_string(cultivation) + R"(, "spirit_stones": 0, "drops": []}})");
    }

    void expectRejected(const std::string& why, const std::string& needle) {
        const auto loaded = load();
        EXPECT_FALSE(loaded.ok) << why << "：该拒收却读进来了";
        EXPECT_NE(loaded.error.find(needle), std::string::npos) << why << "：实际报的是 " << loaded.error;
    }

    TempDir tmp_{"fanren_pathaction_refs"};
    fs::path root_;
    GameData data_;
    std::set<std::string> flags_;
    std::vector<std::string> entries_;
};

TEST_F(PathActionReferences, AConsistentTreeLoadsAndAMissingDirectoryIsZeroEntries) {
    const auto loaded = load();
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.size(), 3u);

    TempDir empty("fanren_pathaction_empty");
    fs::create_directories(empty.path() / "data");
    const auto none = fanren::io::loadPathActions((empty.path() / "data").string(), data_);
    ASSERT_TRUE(none.ok) << none.error;
    EXPECT_TRUE(none.value.empty()) << "没有 pathactions 目录 = 0 条，不算错";
}

TEST_F(PathActionReferences, AMissingTextKeyIsRejected) {
    data_.text.erase("t.text");
    expectRejected("文案 key 不存在", "文案 key 不存在：t.text");
}

TEST_F(PathActionReferences, AMissingItemIsRejected) {
    data_.items.erase("i1");
    expectRejected("给的物品不存在", "物品 id 不存在：i1");
}

TEST_F(PathActionReferences, AnUnregisteredFlagIsRejected) {
    flags_.erase("ch04.b");
    expectRejected("until 里的旗标没登记", "旗标未在 data/flags.json 登记：ch04.b");
}

TEST_F(PathActionReferences, AMissingRevealRoleIsRejected) {
    data_.roles.erase("r1");
    expectRejected("揭破绽的角色不存在", "reveal 的角色不存在：r1");
}

TEST_F(PathActionReferences, AMissingMapOrNpcIsRejected) {
    entries_[0] = replacedOnce(kInquiry, R"("map": "ch04_getang")", R"("map": "ch04_nowhere")");
    expectRejected("地图不存在", "地图不存在");
    entries_[0] = replacedOnce(kInquiry, R"("npc": "npc_li_feiyu")", R"("npc": "npc_nobody")");
    expectRejected("那张图上没有这个 npc", "上没有名为 npc_nobody 的 npc 对象");
}

TEST_F(PathActionReferences, ChallengeBattlesAreCheckedUnlessPendingAndPendingOnesMustNotExistYet) {
    ASSERT_TRUE(load().ok) << "先验：pending 且编成不存在，照样读得进";
    writeBattle(false, 0);
    expectRejected("编成建好了还挂着 pending", "去掉 pending");

    entries_[2] = replacedOnce(kChallenge, R"(, "pending": true)", "");
    const auto built = load();
    ASSERT_TRUE(built.ok) << built.error << "（先验：建好的编成、输了不死、自己不发奖励，读得进）";
    writeBattle(true, 0);
    expectRejected("切磋输了会 game over", "defeat_is_fatal 必须为 false");
    writeBattle(false, 30);
    expectRejected("切磋的编成自己发奖励", "编成的必须全 0");
    fs::remove(root_ / "data" / "battles" / "b_probe.json");
    expectRejected("不是 pending、编成却不存在", "编成不存在：b_probe");
}

TEST_F(PathActionReferences, AnInquiryMayNotMarkAnotherEntryDone) {
    entries_[0] = replacedOnce(kInquiry, R"(["ch04.path.zhi"])", R"(["ch04.path.probe_qiugou"])");
    expectRejected("打探置别的条目的 done_flag", "是别的条目的 done_flag");
}

TEST_F(PathActionReferences, TwoFilesForOneChapterAreRejected) {
    writeFile(root_ / "data" / "pathactions" / "more" / "ch04.json", chapterFile(kPurchase));
    expectRejected("两个文件同属一章", "同属第 4 章");
}

// ===========================================================================
// 三、真数据：第 1–5 章的设计意图
// ===========================================================================

struct RealData {
    bool ok = false;
    std::string error;
    GameData data;
    std::vector<PathAction> actions;
    std::map<std::string, fanren::core::BattleSetup> battles;
};

const RealData& realData() {
    static const RealData kData = [] {
        RealData r;
        auto data = fanren::io::loadGameData(assetRoot() + "/data");
        if (!data) {
            r.error = data.error;
            return r;
        }
        r.data = std::move(data.value);
        auto actions = fanren::io::loadPathActions(assetRoot() + "/data", r.data);
        if (!actions) {
            r.error = actions.error;
            return r;
        }
        r.actions = std::move(actions.value);
        auto battles = fanren::io::loadBattles(assetRoot() + "/data/battles");
        if (!battles) {
            r.error = battles.error;
            return r;
        }
        r.battles = std::move(battles.value);
        r.ok = true;
        return r;
    }();
    return kData;
}

// 一条条目上屏的全部文案 key。
std::vector<std::string> keysOf(const PathAction& a) {
    std::vector<std::string> keys;
    for (const std::string* key : {&a.textKey, &a.refuseKey, &a.dealKey, &a.poorKey, &a.winKey, &a.loseKey}) {
        if (!key->empty()) keys.push_back(*key);
    }
    return keys;
}

// UTF-8 里的汉字个数（基本区与扩展 A、兼容区）。标点、字母、数字不算。
int hanCount(const std::string& text) {
    int count = 0;
    for (std::size_t i = 0; i < text.size();) {
        const auto c = static_cast<unsigned char>(text[i]);
        if (c >= 0xE0 && c < 0xF0 && i + 2 < text.size()) {
            const unsigned cp = ((c & 0x0Fu) << 12) | ((static_cast<unsigned char>(text[i + 1]) & 0x3Fu) << 6) |
                                (static_cast<unsigned char>(text[i + 2]) & 0x3Fu);
            if ((cp >= 0x3400 && cp <= 0x9FFF) || (cp >= 0xF900 && cp <= 0xFAFF)) ++count;
            i += 3;
        } else {
            i += c < 0x80 ? 1u : (c < 0xE0 ? 2u : 4u);
        }
    }
    return count;
}

TEST(PathActionData, AllFiveChaptersLoadAndEveryLineHasWords) {
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    std::map<int, int> perChapter;
    int keys = 0;
    for (const PathAction& a : real.actions) {
        ++perChapter[a.chapter];
        for (const std::string& key : keysOf(a)) {
            ++keys;
            EXPECT_NE(real.data.lookupText(key), key) << a.id << " 的 " << key << " 查不到";
            EXPECT_GT(hanCount(real.data.lookupText(key)), 0) << key << " 是空话";
        }
    }
    for (int chapter = 1; chapter <= 5; ++chapter) {
        EXPECT_GT(perChapter[chapter], 0) << "第 " << chapter << " 章一条路径行动也没读到，下面那些「每一条都……」在空转";
    }
    EXPECT_GE(keys, static_cast<int>(real.actions.size())) << "每条至少一句话";

    // 只查形状的那一层（loadGameData 走它）读出来的必须是同一批、同一个次序。
    const auto shapes = fanren::io::loadPathActionDir(assetRoot() + "/data/pathactions");
    ASSERT_TRUE(shapes.ok) << shapes.error;
    ASSERT_EQ(shapes.value.size(), real.actions.size());
    for (std::size_t i = 0; i < shapes.value.size(); ++i) {
        EXPECT_EQ(shapes.value[i].id, real.actions[i].id) << "第 " << i << " 条";
        EXPECT_EQ(shapes.value[i].chapter, real.actions[i].chapter) << "第 " << i << " 条";
    }
}

// 施工图 5.1：打探第 1 章起、求购第 2 章起、切磋第 3 章起（战斗开放之后）。
TEST(PathActionData, EachKindOpensInTheChapterTheDesignSays) {
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    for (const PathAction& a : real.actions) {
        if (a.kind == PathActionKind::Purchase) EXPECT_GE(a.chapter, 2) << a.id << "：求购第 2 章起";
        if (a.kind == PathActionKind::Challenge) EXPECT_GE(a.chapter, 3) << a.id << "：切磋第 3 章起";
    }
}

// 阅历门槛够得着：韩立在那一章里剧情给得到的最高境界（上限）。
//   第 1 章：凡人。口诀节点 9 才授、上限到那时才抬一层，而同一个脚本当场置 ch01.done，
//            本章条目全在那一刻收起——所以第 1 章的打探一条也不设门槛（派工原话）。
//   第 2 章：三层（scripts/ch02/ceng3.lua 的 realm.cap）。第 3 章：三层（本章脚本不抬）。
//   第 4、5 章：八层（scripts/ch04/yufeng.lua 的 realm.advance；第 5 章不升境，ch05-design 1.3）。
// 这张表的来处由下面的 TheRealmCapTableStillMatchesTheScripts 从脚本里再推一遍。
const std::map<int, Realm>& realmCapOfChapter() {
    static const std::map<int, Realm> kCap = {{1, Realm::Mortal},      {2, Realm::QiRefining3}, {3, Realm::QiRefining3},
                                              {4, Realm::QiRefining8}, {5, Realm::QiRefining8},
                                              // 第 6 章：九层（scripts/ch06/kuxiu.lua 节点 9b 的 realm.advance）。
                                              {6, Realm::QiRefining9}};
    return kCap;
}

TEST(PathActionData, EveryGateIsWithinWhatHanLiCanReachInThatChapter) {
    const std::map<int, Realm>& cap = realmCapOfChapter();
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    int gated = 0;
    for (const PathAction& a : real.actions) {
        ASSERT_TRUE(cap.count(a.chapter) != 0) << "第 " << a.chapter << " 章还没定门槛的口径，先补这张表";
        EXPECT_LE(fanren::rules::toValue(a.minRealm), fanren::rules::toValue(cap.at(a.chapter)))
            << a.id << " 的门槛是" << fanren::rules::nameOf(a.minRealm) << "，第 " << a.chapter << " 章到不了";
        if (a.minRealm != Realm::Mortal) ++gated;
    }
    EXPECT_GT(gated, 0) << "先验：至少有一条设了门槛，否则上面那条比较只在凡人对凡人";
}

// 门槛上限表的来处：剧情脚本里抬境界的那几句（realm.cap / realm.advance，玩家打坐突破也越不过它）。
// 第 N 章够得着的 = 第 1..N 章脚本里抬到过的最高一层。第 1 章例外：那一层与 ch01.done 在同一个脚本里给，
// 本章条目在那一刻一起收起，所以按凡人算——这一点也核：第 1 章抬境界的脚本必须同时置 ch01.done。
TEST(PathActionData, TheRealmCapTableStillMatchesTheScripts) {
    static const std::regex kRaise(R"re(realm\.(?:cap|advance)\(realm\.QI_REFINING_(\d+)\))re");
    std::map<int, int> raisedIn;
    int scripts = 0;
    for (int chapter = 1; chapter <= 6; ++chapter) {
        const fs::path dir = fs::path(assetRoot()) / "scripts" / ("ch0" + std::to_string(chapter));
        ASSERT_TRUE(fs::is_directory(dir)) << dir.string();
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.path().extension() != ".lua") continue;
            ++scripts;
            const std::string body = readFile(entry.path());
            for (auto it = std::sregex_iterator(body.begin(), body.end(), kRaise); it != std::sregex_iterator(); ++it) {
                raisedIn[chapter] = std::max(raisedIn[chapter], std::stoi((*it)[1].str()));
                if (chapter == 1) {
                    EXPECT_NE(body.find(R"(flag.set("ch01.done"))"), std::string::npos)
                        << entry.path().string() << " 在第 1 章抬了境界，却不在同一个脚本里收尾：第 1 章的门槛不能再按凡人算";
                }
            }
        }
    }
    ASSERT_GT(scripts, 20) << "先验：脚本读到了";
    ASSERT_GT(raisedIn[1], 0) << "先验：正则抓得到脚本里抬境界的那一句";
    int reachable = raisedIn[1];
    for (int chapter = 2; chapter <= 6; ++chapter) {
        reachable = std::max(reachable, raisedIn[chapter]);
        EXPECT_EQ(fanren::rules::toValue(realmCapOfChapter().at(chapter)), reachable)
            << "第 " << chapter << " 章：脚本抬到炼气 " << reachable << " 层，门槛上限表写的是"
            << fanren::rules::nameOf(realmCapOfChapter().at(chapter));
    }
}

// 价钱付得起：本章最省钱的主线路上、这一条挂出来之后手里至少有的碎银。
//   第 2 章：6 块——头一回卖药（maiyao.lua，置 ch02.duan2_start）给六块，下一笔进账在章末算账，
//            而算账那个脚本当场置 ch02.done。所以本章的求购还得挂在 duan2_start 之后。
//   第 4 章：28 块——第 3 章交过来多少就是多少（两侧夹具都是 28），坊门那一仗（节点 7）之前没有进账，
//            之后只增不减。
//   第 5 章：87 块——第 4 章终局首侧 112（第二侧 148）；本章能花钱的只有三处可选项（3a 一袋 20、4a 一小袋 10、
//            7b 见面礼 20），中间③④两仗各进 10、15；全花掉的那条路上最低点是 7b 之后：112−20+10−10+15−20 = 87。
struct Purse {
    int silver;
    const char* afterFlag;   // 空 = 本章一开局就有
};

TEST(PathActionData, EveryPriceFitsTheLeanestPurseOfItsChapter) {
    // 第 6 章：0 块——节点 1a 碎银留给了村民，节点 12 搜身之前一块灵石也没有（docs/ch06-design.md 第 9 节）。
    // 本章那一条求购（丹砂）是**有意**买不起的：条目挂着、poor_key 说没有灵石，让玩家看见自己的穷
    //（施工图 16.2）。例外只此一条、按 id 点名，别的求购照旧得付得起。
    const std::map<int, Purse> purse = {{2, {6, "ch02.duan2_start"}}, {4, {28, ""}}, {5, {87, ""}},
                                        {6, {0, ""}}};
    const std::set<std::string> kDeliberatelyUnaffordable = {"dansha_qiugou"};
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    int purchases = 0;
    for (const PathAction& a : real.actions) {
        if (a.kind != PathActionKind::Purchase) continue;
        ++purchases;
        ASSERT_TRUE(purse.count(a.chapter) != 0) << "第 " << a.chapter << " 章还没算过钱袋，先补这张表";
        const Purse& p = purse.at(a.chapter);
        if (kDeliberatelyUnaffordable.count(a.id) != 0) {
            EXPECT_GT(a.price, p.silver) << a.id << "：点名为有意买不起，却付得起了——例外表该删这一条";
            continue;
        }
        EXPECT_LE(a.price, p.silver) << a.id << " 卖 " << a.price << " 块，本章最省的那条路上手里只有 " << p.silver;
        if (p.afterFlag[0] != '\0') {
            const bool after = std::any_of(a.when.begin(), a.when.end(), [&p](const QuestCondition& c) {
                return c.op == Op::FlagAtLeast && c.subject == p.afterFlag && c.value >= 1;
            });
            EXPECT_TRUE(after) << a.id << " 得在 " << p.afterFlag << " 之后才挂：那之前身上一块钱也没有";
        }
    }
    EXPECT_GT(purchases, 0) << "先验：读到了求购";
}

// 上面那张钱袋表的来处还在不在：卖药给的钱、夹具里的钱少了，表就不再是「至少有的」。
TEST(PathActionData, ThePurseTableStillMatchesTheChapterFixtures) {
    const std::string maiyao = readFile(fs::path(assetRoot()) / "scripts" / "ch02" / "maiyao.lua");
    static const std::regex kGive(R"re(give\("material_lingshi",\s*(\d+)\))re");
    int paid = 0;
    for (auto it = std::sregex_iterator(maiyao.begin(), maiyao.end(), kGive); it != std::sregex_iterator(); ++it) {
        paid += std::stoi((*it)[1].str());
    }
    EXPECT_GE(paid, 6) << "scripts/ch02/maiyao.lua：第 2 章的钱袋按头一回卖药给的六块算的";
    EXPECT_NE(maiyao.find(R"(flag.set("ch02.duan2_start"))"), std::string::npos)
        << "那六块与 ch02.duan2_start 得在同一个脚本里给：求购挂在这面旗之后才算付得起";

    const fs::path fixtures = fs::path(assetRoot()) / "tests" / "fixtures";
    for (const char* file : {"ch03-end-first.sav", "ch03-end-second.sav"}) {
        const auto state = fanren::io::loadGame((fixtures / file).string());
        ASSERT_TRUE(state.ok) << state.error;
        EXPECT_GE(state.value.itemCount(kSilver), 28) << file << "：第 4 章的钱袋按 28 块算的";
    }
    for (const char* file : {"ch04-end-first.sav", "ch04-end-second.sav"}) {
        const auto state = fanren::io::loadGame((fixtures / file).string());
        ASSERT_TRUE(state.ok) << state.error;
        EXPECT_GE(state.value.itemCount(kSilver), 112) << file << "：第 5 章的钱袋从 112 块推出来";
    }
}

// 切磋是添头：赢一场给的修为不超过本章剧情里那一场切磋（第 4 章演武台 40、第 5 章燕歌讨教 30）。
TEST(PathActionData, NoSparPaysMoreThanItsChaptersStorySpar) {
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    for (const PathAction& a : real.actions) {
        if (a.kind != PathActionKind::Challenge) continue;
        int storySpar = -1;
        for (const auto& [id, setup] : real.battles) {
            if (setup.chapter == a.chapter && !setup.defeatIsFatal && id.find("qiecuo") != std::string::npos) {
                storySpar = std::max(storySpar, setup.reward.cultivation);
            }
        }
        ASSERT_GE(storySpar, 0) << "第 " << a.chapter << " 章没有剧情切磋可作参照，" << a.id << " 的奖励先定口径";
        EXPECT_LE(a.rewardCultivation, storySpar) << a.id;
    }
}

// 派工原话：打探每条 40–150 字。
TEST(PathActionData, EveryInquiryRunsFortyToOneHundredFiftyCharacters) {
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    int inquiries = 0;
    for (const PathAction& a : real.actions) {
        if (a.kind != PathActionKind::Inquire) continue;
        ++inquiries;
        const int n = hanCount(real.data.lookupText(a.textKey));
        EXPECT_GE(n, 40) << a.id << " 只有 " << n << " 字";
        EXPECT_LE(n, 150) << a.id << " 有 " << n << " 字";
    }
    EXPECT_GT(inquiries, 0);
}

// 回绝不许点名境界（派工原话：用「阅历不足」式的说法）：只说对方不肯，不说「你还不到第几层」。
// 词表 = 存档里每一个境界的名字（rules::nameOf）＋ 大境界的名字 ＋ 功法名 ＋「第 N 层」。
TEST(PathActionData, RefusalsNeverNameARealm) {
    std::vector<std::string> names = {"炼气", "筑基", "结丹", "长春功"};
    for (int v : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 21, 22, 23, 31, 32, 33}) {
        names.emplace_back(fanren::rules::nameOf(fanren::rules::fromValue(v)));
    }
    for (const char* n : {"一", "二", "三", "四", "五", "六", "七", "八", "九", "十", "十一", "十二", "十三"}) {
        names.push_back(std::string("第") + n + "层");
    }
    const auto caught = [&names](const std::string& text) {
        return std::any_of(names.begin(), names.end(),
                           [&text](const std::string& name) { return text.find(name) != std::string::npos; });
    };
    ASSERT_TRUE(caught("他还不到" + std::string(fanren::rules::nameOf(Realm::QiRefining7)))) << "先验：抓得住境界名";
    ASSERT_TRUE(caught("口诀才练到第三层")) << "先验：抓得住「第 N 层」";
    ASSERT_FALSE(caught("师父说他功课还没做扎实")) << "先验：不点名的回绝不许误报";
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    int refusals = 0;
    for (const PathAction& a : real.actions) {
        if (a.refuseKey.empty()) continue;
        ++refusals;
        const std::string text = real.data.lookupText(a.refuseKey);
        for (const std::string& name : names) {
            EXPECT_EQ(text.find(name), std::string::npos) << a.id << " 的回绝里有「" << name << "」：" << text;
        }
    }
    EXPECT_GT(refusals, 0) << "先验：读到了回绝";
}

// 价钱只活在 price 里：文案写死「十二块碎银」，改价的人只改数据，嘴上说的就成了假话。
bool numeralBefore(const std::string& text, std::size_t at) {
    static const std::vector<std::string> kNumerals = {"零", "一", "二", "两", "三", "四", "五", "六", "七",
                                                       "八", "九", "十", "百", "千", "几", "半"};
    if (at > 0 && text[at - 1] >= '0' && text[at - 1] <= '9') return true;
    return std::any_of(kNumerals.begin(), kNumerals.end(), [&](const std::string& n) {
        return at >= n.size() && text.compare(at - n.size(), n.size(), n) == 0;
    });
}

bool spellsOutSilver(const std::string& text) {
    const std::string block = "块";
    const std::string silver = "碎银";
    for (std::size_t at = text.find(block + silver); at != std::string::npos; at = text.find(block + silver, at + 1)) {
        if (numeralBefore(text, at)) return true;
    }
    for (std::size_t at = text.find(silver); at != std::string::npos; at = text.find(silver, at + 1)) {
        const std::size_t next = text.find(block, at + silver.size());
        if (next != std::string::npos && next > at + silver.size() && next <= at + silver.size() + 9 &&
            numeralBefore(text, next)) {
            return true;
        }
    }
    return false;
}

TEST(PathActionData, PricesAreNeverSpelledOutInTheWords) {
    ASSERT_TRUE(spellsOutSilver("他数出十二块碎银推过去")) << "先验：扫描器抓得住";
    ASSERT_TRUE(spellsOutSilver("开价碎银二十块"));
    ASSERT_FALSE(spellsOutSilver("收钱时一块一块数得很仔细")) << "先验：不是钱的「一块」不许误报";
    ASSERT_FALSE(spellsOutSilver("从怀里掏出一包碎银"));
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    for (const PathAction& a : real.actions) {
        for (const std::string& key : keysOf(a)) {
            EXPECT_FALSE(spellsOutSilver(real.data.lookupText(key))) << key << " 把钱数写进了嘴里";
        }
    }
}

// 第 1、2 章的硬约束（ch01-design 第 1 节表第 4 条、ch02-design 第 1 节表第 8 条）：七玄门是凡人武馆，
// 本章任何人都不提修仙、灵根、法术。tests/LexiconTests.cpp 的词表不收这三个词，这里专门看路径行动的文案。
TEST(PathActionData, ChaptersOneAndTwoNeverSpeakOfImmortals) {
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    int scanned = 0;
    for (const PathAction& a : real.actions) {
        if (a.chapter > 2) continue;
        for (const std::string& key : keysOf(a)) {
            ++scanned;
            const std::string text = real.data.lookupText(key);
            for (const char* word : {"修仙", "灵根", "法术"}) {
                EXPECT_EQ(text.find(word), std::string::npos) << key << " 里有「" << word << "」";
            }
        }
    }
    EXPECT_GT(scanned, 10) << "先验：第 1、2 章的文案真的扫到了";
}

// 第 5 章的章内先后（ch05-design 12.2）按条目自己的 when 判：这条挂出来的那一节 = when 里剧情旗标所属节点的最大值
//（节点号照 ch05-design 3.1 每个挂点的 set_flag）。形状与 Ch05LexiconTests 不同——那一份按 key 的前缀查节点表，
// 这一份按数据推——两份都得登记对，这里还顺手核那份节点表里 ch05.path.* 那几行与数据推出来的一致。
// 「升仙」「仙令」「天眼」本章一个字也不许：那一条归 Ch05LexiconTests，它扫 data/text/ch05*.json，本路的
// ch05_path.json 在里面。
const std::map<std::string, int>& chapterFiveNodeOfFlag() {
    static const std::map<std::string, int> kNodes = {
        {"ch05.kaipian", 1},  {"ch05.shangchuan", 1}, {"ch05.matou", 2},     {"ch05.shoufu", 3},
        {"ch05.zhuishao", 3}, {"ch05.qingbao", 4},    {"ch05.jieren", 4},    {"ch05.jiulou", 5},
        {"ch05.yeru", 6},     {"ch05.toutin", 6},     {"ch05.dengmen", 7},   {"ch05.jianmianli", 7},
        {"ch05.huayuan", 8},  {"ch05.duizhi", 9},     {"ch05.dingji", 10},   {"ch05.xiaoxiang", 10},
        {"ch05.duobang", 10}, {"ch05.jiaoyi", 11},    {"ch05.yange", 11},    {"ch05.anpai", 11},
        {"ch05.shigui", 12},  {"ch05.chuzheng", 12},  {"ch05.tancha", 12},   {"ch05.cisha", 12},
        {"ch05.done", 12},
    };
    return kNodes;
}

int chapterFiveNodeOf(const PathAction& a) {
    int node = -1;
    for (const QuestCondition& c : a.when) {
        if (c.subject.rfind("ch05.path.", 0) == 0) continue;   // 别的条目置的：那一条自己挂出来得更早
        const auto found = chapterFiveNodeOfFlag().find(c.subject);
        if (found != chapterFiveNodeOfFlag().end()) node = std::max(node, found->second);
    }
    return node;
}

TEST(PathActionData, ChapterFiveSaysNoWordBeforeTheNodeItsWhenOpensAt) {
    struct Ordering {
        const char* word;
        int firstNode;
    };
    const std::vector<Ordering> orderings = {{"寒毒", 9},   {"惊蛟会", 4}, {"五色门", 9},
                                             {"独霸山庄", 9}, {"太南谷", 11}, {"太南山", 11}};
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;

    std::map<std::string, int> registered;
    const std::string lexicon = readFile(fs::path(assetRoot()) / "tests" / "Ch05LexiconTests.cpp");
    static const std::regex kRow(R"re(\{"ch05\.path\.([a-z0-9_]+)\.",\s*(\d+)\})re");
    for (auto it = std::sregex_iterator(lexicon.begin(), lexicon.end(), kRow); it != std::sregex_iterator(); ++it) {
        registered[(*it)[1]] = std::stoi((*it)[2]);
    }

    int entries = 0;
    for (const PathAction& a : real.actions) {
        if (a.chapter != 5) continue;
        ++entries;
        const int node = chapterFiveNodeOf(a);
        ASSERT_GE(node, 1) << a.id << " 的 when 里没有一个认得出节点的剧情旗标";
        const auto row = registered.find(a.id);
        EXPECT_TRUE(row != registered.end() && row->second == node)
            << a.id << " 在 tests/Ch05LexiconTests.cpp 的节点表里该登记成第 " << node << " 节";
        for (const std::string& key : keysOf(a)) {
            const std::string text = real.data.lookupText(key);
            for (const Ordering& o : orderings) {
                if (node < o.firstNode) {
                    EXPECT_EQ(text.find(o.word), std::string::npos)
                        << key << " 第 " << node << " 节就挂出来了，可「" << o.word << "」第 " << o.firstNode << " 节才说得出";
                }
            }
        }
    }
    EXPECT_GT(entries, 5) << "先验：第 5 章的条目读到了";
    EXPECT_EQ(registered.size(), static_cast<std::size_t>(entries))
        << "节点表里 ch05.path.* 的行数与第 5 章的条目数对不上：多出来的是删掉的条目没撤登记，少的是新条目没登记";
}

// 条目挂着的那个 NPC 对象是谁（它的 role_id）。地图读不到或图上没有这个对象时返回空串。
std::string npcRoleOf(const PathAction& a) {
    const auto map = fanren::io::loadTileMap(assetRoot() + "/maps/" + a.mapId + ".tmj");
    if (!map.ok) return {};
    for (const fanren::core::MapObject& object : map.value.objects) {
        if (object.type == "npc" && object.name == a.npc) return object.property("role_id");
    }
    return {};
}

// 协调者拍板（第 5 章独立校对 docs/ch05-review.md 8.2、8.4）：欧阳飞天只能以剑符取首级，打探不揭他的破绽、
// 不提前说刀剑难伤与霸王甲；燕歌、吴剑鸣不挂切磋；本章求购不卖清灵散、养精丹；独霸山庄以外不写庄丁。
TEST(PathActionData, ChapterFiveLeavesTheManorAndItsMasterToTheStory) {
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    int challenges = 0;
    for (const PathAction& a : real.actions) {
        if (a.chapter != 5) continue;
        for (const PathReveal& r : a.reveals) EXPECT_NE(r.roleId, "ouyang_feitian") << a.id;
        if (a.kind == PathActionKind::Challenge) {
            ++challenges;
            const std::string role = npcRoleOf(a);
            ASSERT_FALSE(role.empty()) << a.id << "：" << a.mapId << " 上找不到 " << a.npc << " 的 role_id";
            EXPECT_NE(role, "yan_ge") << a.id << "：燕歌只有剧情里那一场讨教（b05_yange_qiecuo）";
            EXPECT_NE(role, "wu_jianming") << a.id;
        }
        if (a.kind == PathActionKind::Purchase) {
            EXPECT_NE(a.goods.itemId, "pill_qingling_san") << a.id;
            EXPECT_NE(a.goods.itemId, "pill_yangjing_dan") << a.id;
        }
        for (const std::string& key : keysOf(a)) {
            const std::string text = real.data.lookupText(key);
            for (const char* word : {"刀剑难伤", "霸王甲", "刀枪不入", "砍不进", "庄丁"}) {
                EXPECT_EQ(text.find(word), std::string::npos) << key << " 里有「" << word << "」";
            }
        }
    }
    EXPECT_GT(challenges, 0) << "先验：第 5 章的切磋读到了，上面那两条 role 比较不是在空转";
}

// 药账（ch04-design 3.3）：养精丹只有第 4 章峰上那座丹炉一个来源，「第 1–4 章…都不卖养精丹」，本章发几批料
// 也记在那张账上——所以第 1–4 章的路径行动不给、不卖、不赏养精丹，也不碰炼它的黄精、紫参。
// 那块牌子（ch04-design 1.1，本章没有名字）哪一章都不许有人给、卖、赏。
TEST(PathActionData, ThePillLedgerAndThePlaqueAreLeftAlone) {
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    int handedOver = 0;
    for (const PathAction& a : real.actions) {
        std::vector<std::string> items;
        if (a.kind == PathActionKind::Purchase) items.push_back(a.goods.itemId);
        for (const BagEntry& e : a.gives) items.push_back(e.itemId);
        for (const BagEntry& e : a.rewardItems) items.push_back(e.itemId);
        for (const std::string& item : items) {
            ++handedOver;
            EXPECT_NE(item, "material_heipai") << a.id;
            if (a.chapter <= 4) {
                for (const char* banned : {"pill_yangjing_dan", "herb_huangjing_cao", "herb_zishen_cao"}) {
                    EXPECT_NE(item, banned) << a.id << "：第 1–4 章的药账不动";
                }
            }
        }
    }
    EXPECT_GT(handedOver, 3) << "先验：读到了给出去的物件";
}

// 揭破绽的角色得真的会上阵：在某场编成里，或是某条 pending 切磋的那个 NPC（编成补建时就是他）。
TEST(PathActionData, EveryRevealTargetsARoleThatActuallyFights) {
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    std::set<std::string> fighting;
    for (const auto& [id, setup] : real.battles) {
        for (const fanren::core::BattleUnitSpec& unit : setup.units) fighting.insert(unit.roleId);
    }
    for (const PathAction* a : fanren::rules::pendingChallenges(real.actions)) {
        const std::string role = npcRoleOf(*a);
        ASSERT_FALSE(role.empty()) << a->id << "：" << a->mapId << " 上找不到 " << a->npc << " 的 role_id";
        fighting.insert(role);
    }
    int reveals = 0;
    for (const PathAction& a : real.actions) {
        for (const PathReveal& r : a.reveals) {
            ++reveals;
            EXPECT_TRUE(fighting.count(r.roleId) != 0) << a.id << " 揭的 " << r.roleId << " 不在任何一场编成里";
        }
    }
    EXPECT_GT(reveals, 5) << "先验：读到了揭破绽的打探";
}

// ---------------------------------------------------------------------------
// 待办闸门：pending 的切磋在第 5 章收尾前必须清零
// ---------------------------------------------------------------------------
// 切磋的编成要按战斗路的横版格式建（施工图第 2 节；派工写明这一棒不新建 data/battles 与敌方 data/roles），
// 所以现在挂着 pending。这条用例每跑一次都把它们一条条念出来；**一旦横版格式合回来了**
// （data/battles 或 data/roles 里出现 weaknesses / toughness 字段），还挂着 pending 就转红——
// 那一刻起它们就该补建，而不是继续躺在待办里。规格见 docs/interfaces-octo-pathactions.md 第 6 节。
bool sideScrollFormatLanded() {
    for (const char* dir : {"battles", "roles"}) {
        const fs::path root = fs::path(assetRoot()) / "data" / dir;
        if (!fs::is_directory(root)) continue;
        for (const auto& entry : fs::recursive_directory_iterator(root)) {
            if (entry.path().extension() != ".json") continue;
            const std::string body = readFile(entry.path());
            if (body.find("\"weaknesses\"") != std::string::npos || body.find("\"toughness\"") != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

TEST(PathActionData, PendingChallengesMustBeBuiltOnceTheSideScrollBattlesLandBack) {
    const RealData& real = realData();
    ASSERT_TRUE(real.ok) << real.error;
    const std::vector<const PathAction*> pending = fanren::rules::pendingChallenges(real.actions);
    std::ostringstream list;
    for (const PathAction* a : pending) {
        list << "  第 " << a->chapter << " 章 " << a->id << " → 编成 " << a->battleId << "（" << a->mapId << " / "
             << a->npc << "）\n";
        EXPECT_EQ(real.battles.count(a->battleId), 0u) << a->battleId << " 已经建好了，" << a->id << " 该去掉 pending";
    }
    if (!pending.empty()) {
        std::cout << "[路径行动·待办] 还有 " << pending.size() << " 条切磋等编成：\n" << list.str();
    }
    if (sideScrollFormatLanded()) {
        EXPECT_TRUE(pending.empty()) << "横版战斗格式已经合回来了，这几条切磋的编成该补建了"
                                        "（规格：docs/interfaces-octo-pathactions.md 第 6 节）：\n"
                                     << list.str();
    }
}

}  // namespace
