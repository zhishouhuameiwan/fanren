// 任务系统最低版（契约 docs/interfaces-p3-ch05.md 第 1 节，设计 docs/ch05-design.md 10.4 Q1–Q12）。
//
// 三段各有一组，外加一组「第 1–4 章零变化」：
//   · 规则层   —— 谓词三种、状态四种与判定次序、当前步骤「走得最远」、告示板排序；
//   · 加载器   —— 好文件读得进；契约 1.6 那张拒收表逐项一条负例；主线由目标链原地生成；
//   · 游戏层   —— 告示板三种支线行的样子；HUD 与指路一概不看支线；开告示板不发奖励；
//   · 零变化   —— 沿第 1–4 章真链的每一个前缀，告示板的行与改动前的算法逐行相同。
//
// 判据的形状（docs/README.md「判据自己会说谎」那张表逐条对过）：
//   · 每一条「不成立」都配一条同一处的「成立」，反之亦然——只验一边的话，一个恒真或恒假的
//     谓词能让那一半全绿；
//   · 「已过期」那一行先验原因**确实有内容**，不写成「找不到某个词就算过」；
//   · 零变化那一组先验走过的前缀数 > 0、第 1–4 章的步骤数 > 0，分母塌成 0 时不许空转。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Objectives.h"
#include "core/rules/Quests.h"
#include "game/Application.h"
#include "game/BoardScene.h"
#include "game/WorldScene.h"
#include "io/DataLoader.h"
#include "io/QuestLoader.h"
#include "io/SaveFile.h"
#include "ui/Widgets.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameData;
using fanren::core::GameState;
using fanren::core::Objective;
using fanren::core::Quest;
using fanren::core::QuestCondition;
using fanren::core::QuestKind;
using fanren::core::QuestStep;
using fanren::game::BoardScene;
using fanren::rules::QuestStatus;
using fanren::rules::allConditionsHold;
using fanren::rules::conditionHolds;
using fanren::rules::currentQuestStep;
using fanren::rules::questStatus;
using fanren::rules::sideQuestJournal;
using fanren::test::TempDir;
using Op = fanren::core::QuestCondition::Op;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "data" / "objectives" / "ch01.json") &&
            fs::exists(root / "maps" / "ch01_hanjiacun.tmj")) {
            return candidate;
        }
    }
    return ".";
}

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open()) << "无法创建夹具文件: " << path.string();
    out << content;
}

QuestCondition flagAtLeast(std::string flag, int value) {
    return QuestCondition{Op::FlagAtLeast, std::move(flag), value};
}
QuestCondition flagEquals(std::string flag, int value) {
    return QuestCondition{Op::FlagEquals, std::move(flag), value};
}
QuestCondition itemAtLeast(std::string item, int value) {
    return QuestCondition{Op::ItemAtLeast, std::move(item), value};
}

QuestStep stepOf(std::string id, std::vector<QuestCondition> done) {
    QuestStep step;
    step.id = std::move(id);
    step.textKey = "t." + step.id;
    step.done = std::move(done);
    return step;
}

// 一条三步的支线：备一份药（物品，会回落）→ 走一趟 → 交差。
Quest threeStepQuest() {
    Quest quest;
    quest.id = "q_probe";
    quest.name = "探针";
    quest.chapter = 5;
    quest.kind = QuestKind::Side;
    quest.titleKey = "t.title";
    quest.summaryKey = "t.summary";
    quest.accept = {flagAtLeast("f.accept", 1)};
    quest.steps = {stepOf("s1", {itemAtLeast("pill_x", 1)}),
                   stepOf("s2", {flagAtLeast("f.s2", 1)}),
                   stepOf("s3", {flagAtLeast("f.s3", 1)})};
    quest.complete = {flagAtLeast("f.done", 1)};
    quest.fail = {flagAtLeast("f.late", 1)};
    quest.failTextKey = "t.fail";
    return quest;
}

const std::vector<Objective> kNoChain;

// ===========================================================================
// 一、规则层：谓词
// ===========================================================================

TEST(QuestRule, FlagAtLeastHoldsFromTheValueUpAndNotBelow) {
    GameState state;
    const QuestCondition cond = flagAtLeast("f.x", 2);
    EXPECT_FALSE(conditionHolds(cond, state)) << "没置过（0）不该算 >= 2";
    state.setFlag("f.x", 1);
    EXPECT_FALSE(conditionHolds(cond, state)) << "1 不该算 >= 2";
    state.setFlag("f.x", 2);
    EXPECT_TRUE(conditionHolds(cond, state)) << "2 该算 >= 2";
    state.setFlag("f.x", 3);
    EXPECT_TRUE(conditionHolds(cond, state)) << "3 该算 >= 2";
}

TEST(QuestRule, FlagEqualsHoldsOnlyOnTheValue) {
    GameState state;
    const QuestCondition two = flagEquals("f.x", 2);
    state.setFlag("f.x", 1);
    EXPECT_FALSE(conditionHolds(two, state));
    state.setFlag("f.x", 2);
    EXPECT_TRUE(conditionHolds(two, state));
    state.setFlag("f.x", 3);
    EXPECT_FALSE(conditionHolds(two, state)) << "== 2 在 3 上不该成立：那是 >= 的口径";

    // == 0 即「没置过」（契约 1.2）。
    const QuestCondition unset = flagEquals("f.y", 0);
    EXPECT_TRUE(conditionHolds(unset, state));
    state.setFlag("f.y", 1);
    EXPECT_FALSE(conditionHolds(unset, state));
}

TEST(QuestRule, ItemAtLeastCountsEveryAgeTogether) {
    GameState state;
    const QuestCondition cond = itemAtLeast("herb_a", 2);
    state.addItem("herb_a", 1, 0);
    EXPECT_FALSE(conditionHolds(cond, state)) << "只有 1 株不该算 >= 2";
    state.addItem("herb_a", 1, 40);
    EXPECT_TRUE(conditionHolds(cond, state)) << "零年一株加四十年一株，合计 2 株，该算 >= 2";
    ASSERT_TRUE(state.removeItem("herb_a", 1));
    EXPECT_FALSE(conditionHolds(cond, state)) << "吃掉一株之后又不够了";
}

TEST(QuestRule, AConditionWithNoSubjectNeverHolds) {
    // 旗标名为空串是数据漏写（加载器不收）。按 flag("") == 0 去比，`== 0` 会恒真，
    // 一条漏写的谓词就成了一句空话。
    GameState state;
    EXPECT_FALSE(conditionHolds(flagEquals("", 0), state));
    EXPECT_FALSE(conditionHolds(flagAtLeast("", 1), state));
    EXPECT_FALSE(conditionHolds(itemAtLeast("", 1), state));
    // 对照：主语在的时候 == 0 确实成立，上面那条不是因为 == 0 本身坏了。
    EXPECT_TRUE(conditionHolds(flagEquals("f.present", 0), state));
}

TEST(QuestRule, AListIsAnAndAndTheEmptyListIsTrue) {
    GameState state;
    state.setFlag("f.a", 1);
    EXPECT_TRUE(allConditionsHold({flagAtLeast("f.a", 1)}, state));
    EXPECT_FALSE(allConditionsHold({flagAtLeast("f.a", 1), flagAtLeast("f.b", 1)}, state))
        << "AND：少一条就不成立";
    state.setFlag("f.b", 1);
    EXPECT_TRUE(allConditionsHold({flagAtLeast("f.a", 1), flagAtLeast("f.b", 1)}, state));
    EXPECT_TRUE(allConditionsHold({}, state)) << "空表按数学口径为真（契约 1.3）";
}

// ===========================================================================
// 二、规则层：状态与判定次序
// ===========================================================================

TEST(QuestStatusRule, NotAcceptedHidesEvenAFinishedOrExpiredQuest) {
    Quest quest = threeStepQuest();
    GameState state;
    state.setFlag("f.done", 1);
    state.setFlag("f.late", 1);
    EXPECT_EQ(questStatus(quest, kNoChain, state), QuestStatus::NotAccepted)
        << "没接的任务，complete / fail 成立了也不显示：玩家从没听说过的事谈不上消失";
    EXPECT_EQ(currentQuestStep(quest, kNoChain, state), nullptr);

    state.setFlag("f.accept", 1);
    EXPECT_NE(questStatus(quest, kNoChain, state), QuestStatus::NotAccepted) << "对照：接了就显示";
}

TEST(QuestStatusRule, AcceptedIsActiveThenCompleted) {
    Quest quest = threeStepQuest();
    GameState state;
    state.setFlag("f.accept", 1);
    EXPECT_EQ(questStatus(quest, kNoChain, state), QuestStatus::Active);
    state.setFlag("f.done", 1);
    EXPECT_EQ(questStatus(quest, kNoChain, state), QuestStatus::Completed);
}

TEST(QuestStatusRule, AcceptedAndPastTheDeadlineIsFailed) {
    Quest quest = threeStepQuest();
    GameState state;
    state.setFlag("f.accept", 1);
    state.setFlag("f.late", 1);
    EXPECT_EQ(questStatus(quest, kNoChain, state), QuestStatus::Failed);
    EXPECT_EQ(currentQuestStep(quest, kNoChain, state), nullptr) << "过期了就没有「下一步」";
}

TEST(QuestStatusRule, CompletingBeatsExpiring) {
    // 第 5 章 Z1 的 fail 是 ch05.done，交过医书的人章末照样置它（契约 1.3）。
    Quest quest = threeStepQuest();
    GameState state;
    state.setFlag("f.accept", 1);
    state.setFlag("f.done", 1);
    state.setFlag("f.late", 1);
    EXPECT_EQ(questStatus(quest, kNoChain, state), QuestStatus::Completed)
        << "做完了的任务，章末那一下不许把它改判成「已过期」";
}

TEST(QuestStatusRule, AnEmptyFailListMeansTheQuestNeverExpires) {
    // 空表的 AND 为真。若 fail 走了 AND，每一条不会过期的任务一接下就是「已过期」。
    Quest quest = threeStepQuest();
    quest.fail.clear();
    quest.failTextKey.clear();
    GameState state;
    state.setFlag("f.accept", 1);
    state.setFlag("f.late", 1);
    EXPECT_EQ(questStatus(quest, kNoChain, state), QuestStatus::Active)
        << "fail 为空的任务不会过期（契约 1.3 那一处例外）";
}

TEST(QuestStatusRule, AnEmptyAcceptListIsNeverTakenAsAccepted) {
    // 加载器不收空 accept；这里兜的是「数据绕过了加载器」：按 AND 它恒真，
    // 等于一开局就接了。
    Quest quest = threeStepQuest();
    quest.accept.clear();
    GameState state;
    EXPECT_EQ(questStatus(quest, kNoChain, state), QuestStatus::NotAccepted);
}

// ===========================================================================
// 三、规则层：当前步骤走得最远
// ===========================================================================

TEST(QuestStepRule, TheCurrentStepFollowsTheFurthestDoneStep) {
    Quest quest = threeStepQuest();
    GameState state;
    state.setFlag("f.accept", 1);
    ASSERT_NE(currentQuestStep(quest, kNoChain, state), nullptr);
    EXPECT_EQ(currentQuestStep(quest, kNoChain, state)->id, "s1");

    state.addItem("pill_x", 1);
    ASSERT_NE(currentQuestStep(quest, kNoChain, state), nullptr);
    EXPECT_EQ(currentQuestStep(quest, kNoChain, state)->id, "s2");

    state.setFlag("f.s2", 1);
    ASSERT_NE(currentQuestStep(quest, kNoChain, state), nullptr);
    EXPECT_EQ(currentQuestStep(quest, kNoChain, state)->id, "s3");

    // 药用掉了：第一步的谓词回落。按「第一条没做完的」算，当前步骤会倒退回 s1，
    // 而玩家早就走过那一步了。
    ASSERT_TRUE(state.removeItem("pill_x", 1));
    ASSERT_FALSE(conditionHolds(quest.steps[0].done[0], state)) << "先验：第一步确实回落了";
    ASSERT_NE(currentQuestStep(quest, kNoChain, state), nullptr);
    EXPECT_EQ(currentQuestStep(quest, kNoChain, state)->id, "s3") << "药吃掉了，步骤却倒退了";
}

TEST(QuestStepRule, ASkippedStepDoesNotHoldTheQuestBack) {
    Quest quest = threeStepQuest();
    GameState state;
    state.setFlag("f.accept", 1);
    state.setFlag("f.s2", 1);   // 第一步那份药从没备过
    ASSERT_NE(currentQuestStep(quest, kNoChain, state), nullptr);
    EXPECT_EQ(currentQuestStep(quest, kNoChain, state)->id, "s3");
}

TEST(QuestStepRule, AllStepsDoneButNotCompleteHasNoCurrentStep) {
    Quest quest = threeStepQuest();
    GameState state;
    state.setFlag("f.accept", 1);
    state.addItem("pill_x", 1);
    state.setFlag("f.s2", 1);
    state.setFlag("f.s3", 1);
    ASSERT_EQ(questStatus(quest, kNoChain, state), QuestStatus::Active) << "先验：还没了结";
    EXPECT_EQ(currentQuestStep(quest, kNoChain, state), nullptr)
        << "全部做完而没了结：没有下一步可说，告示板改说 summary";
}

// ===========================================================================
// 四、规则层：告示板的排序
// ===========================================================================

TEST(QuestJournal, ActiveThenFailedThenCompletedNewestChapterFirstAndUntakenLeftOut) {
    auto sideQuest = [](std::string id, int chapter, std::string acceptFlag) {
        Quest quest = threeStepQuest();
        quest.id = std::move(id);
        quest.chapter = chapter;
        quest.accept = {flagAtLeast(acceptFlag, 1)};
        quest.complete = {flagAtLeast("f.done." + quest.id, 1)};
        quest.fail = {flagAtLeast("f.late." + quest.id, 1)};
        return quest;
    };
    std::vector<Quest> quests;
    Quest mainQuest;
    mainQuest.id = "objectives_ch05";
    mainQuest.chapter = 5;
    mainQuest.kind = QuestKind::Main;
    quests.push_back(mainQuest);
    quests.push_back(sideQuest("q_c_done", 5, "f.c"));
    quests.push_back(sideQuest("q_b_late", 5, "f.b"));
    quests.push_back(sideQuest("q_a_now", 5, "f.a"));
    quests.push_back(sideQuest("q_d_untaken", 5, "f.d"));
    quests.push_back(sideQuest("q_e_now6", 6, "f.e"));
    quests.push_back(sideQuest("q_f_now5", 5, "f.f"));

    GameState state;
    for (const char* flag : {"f.a", "f.b", "f.c", "f.e", "f.f"}) state.setFlag(flag, 1);
    state.setFlag("f.late.q_b_late", 1);
    state.setFlag("f.done.q_c_done", 1);

    const auto journal = sideQuestJournal(quests, state);
    std::vector<std::string> ids;
    for (const auto& entry : journal) ids.push_back(entry.quest->id);
    const std::vector<std::string> expected{"q_e_now6", "q_a_now", "q_f_now5", "q_b_late",
                                            "q_c_done"};
    EXPECT_EQ(ids, expected) << "进行中（新章在前、同章按 id）→ 已过期 → 已了结；没接的与主线不列";
    ASSERT_EQ(journal.size(), expected.size());
    EXPECT_EQ(journal[3].status, QuestStatus::Failed);
    EXPECT_EQ(journal[4].status, QuestStatus::Completed);
    ASSERT_NE(journal[0].step, nullptr) << "进行中的要带当前步骤";
    EXPECT_EQ(journal[3].step, nullptr) << "过期的没有步骤";
}

// ===========================================================================
// 五、主线：由目标链原地收编，状态照目标链判据
// ===========================================================================

TEST(QuestMain, AMainQuestFollowsTheChainIncludingACheckpointThatSkippedSteps) {
    auto objective = [](std::string id, std::string flag, int chapter) {
        Objective o;
        o.id = std::move(id);
        o.textKey = "objective." + o.id;
        o.doneFlag = std::move(flag);
        o.targetMap = "m";
        o.targetObject = "o";
        o.chapter = chapter;
        return o;
    };
    const std::vector<Objective> chain{objective("a1", "f.a1", 1), objective("a2", "f.a2", 1),
                                       objective("b1", "f.b1", 2), objective("b2", "f.b2", 2)};
    auto mainOf = [&chain](int chapter) {
        Quest quest;
        quest.id = "objectives_ch0" + std::to_string(chapter);
        quest.chapter = chapter;
        quest.kind = QuestKind::Main;
        for (const Objective& o : chain) {
            if (o.chapter == chapter) quest.steps.push_back(stepOf(o.id, {}));
        }
        return quest;
    };
    const Quest one = mainOf(1);
    const Quest two = mainOf(2);

    GameState state;
    EXPECT_EQ(questStatus(one, chain, state), QuestStatus::Active);
    ASSERT_NE(currentQuestStep(one, chain, state), nullptr);
    EXPECT_EQ(currentQuestStep(one, chain, state)->id, "a1");
    EXPECT_EQ(questStatus(two, chain, state), QuestStatus::NotAccepted);

    state.setFlag("f.a1");
    state.setFlag("f.a2");
    EXPECT_EQ(questStatus(one, chain, state), QuestStatus::Completed);
    EXPECT_EQ(questStatus(two, chain, state), QuestStatus::Active);
    EXPECT_EQ(currentQuestStep(two, chain, state)->id, "b1");

    // 检查点存档只带了后面的旗标（与 ObjectiveTests 那条同一个观感）：
    // 第 1 章一个旗标都没有，第 2 章走到了 b1——第 1 章该算了结，不该算「进行中」。
    GameState checkpoint;
    checkpoint.setFlag("f.b1");
    EXPECT_EQ(questStatus(one, chain, checkpoint), QuestStatus::Completed);
    EXPECT_EQ(questStatus(two, chain, checkpoint), QuestStatus::Active);
    ASSERT_NE(currentQuestStep(two, chain, checkpoint), nullptr);
    EXPECT_EQ(currentQuestStep(two, chain, checkpoint)->id, "b2");

    checkpoint.setFlag("f.b2");
    EXPECT_EQ(questStatus(two, chain, checkpoint), QuestStatus::Completed) << "整条链走完";
}

// ===========================================================================
// 六、加载器
// ===========================================================================

// 一份什么都写全了的支线。下面每一条负例只改其中一处。
const std::string kGoodQuest = R"json({
  "id": "q_probe", "name": "探针", "chapter": 5, "kind": "side",
  "title_key": "t.title", "summary_key": "t.summary",
  "accept": [ {"flag": "f.accept", "op": ">=", "value": 1} ],
  "steps": [
    {"id": "s1", "text_key": "t.s1", "done": [ {"item": "pill_x", "op": ">=", "value": 1} ],
     "target_map": "m1", "target_object": "o1"},
    {"id": "s2", "text_key": "t.s2", "done": [ {"flag": "f.s2", "op": "==", "value": 2} ]}
  ],
  "complete": [ {"flag": "f.done", "op": ">=", "value": 1} ],
  "fail": [ {"flag": "f.late", "op": ">=", "value": 1} ],
  "fail_text_key": "t.fail",
  "rewards": [ {"item_id": "herb_a", "count": 6, "herb_age": 40}, {"item_id": "coin", "count": 2} ],
  "origin": "原著", "note": "说明"
})json";

// 换掉 base 里恰好一处 from。from 找不到就当场失败——否则这条「负例」读的是一份
// 没被改过的好文件，而它照样会报失败（期望失败），那是一条空转的负例。
std::string replaced(const std::string& base, const std::string& from, const std::string& to) {
    const std::size_t at = base.find(from);
    EXPECT_NE(at, std::string::npos) << "夹具里找不到要替换的片段：" << from;
    if (at == std::string::npos) return base;
    EXPECT_EQ(base.find(from, at + 1), std::string::npos) << "要替换的片段不唯一：" << from;
    std::string out = base;
    out.replace(at, from.size(), to);
    return out;
}

fanren::core::Result<Quest> loadQuestText(const std::string& json) {
    TempDir tmp{"fanren_quest_file"};
    const fs::path file = tmp.path() / "q_probe.json";
    writeFile(file, json);
    return fanren::io::loadQuestFile(file.string());
}

TEST(QuestLoading, AFullyWrittenQuestLoadsEveryField) {
    const auto loaded = loadQuestText(kGoodQuest);
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const Quest& q = loaded.value;
    EXPECT_EQ(q.id, "q_probe");
    EXPECT_EQ(q.name, "探针");
    EXPECT_EQ(q.chapter, 5);
    EXPECT_EQ(q.kind, QuestKind::Side);
    EXPECT_EQ(q.titleKey, "t.title");
    EXPECT_EQ(q.summaryKey, "t.summary");
    ASSERT_EQ(q.accept.size(), 1u);
    EXPECT_EQ(q.accept[0].op, Op::FlagAtLeast);
    EXPECT_EQ(q.accept[0].subject, "f.accept");
    EXPECT_EQ(q.accept[0].value, 1);
    ASSERT_EQ(q.steps.size(), 2u);
    EXPECT_EQ(q.steps[0].id, "s1");
    EXPECT_EQ(q.steps[0].textKey, "t.s1");
    ASSERT_EQ(q.steps[0].done.size(), 1u);
    EXPECT_EQ(q.steps[0].done[0].op, Op::ItemAtLeast);
    EXPECT_EQ(q.steps[0].done[0].subject, "pill_x");
    EXPECT_EQ(q.steps[0].targetMap, "m1");
    EXPECT_EQ(q.steps[0].targetObject, "o1");
    ASSERT_EQ(q.steps[1].done.size(), 1u);
    EXPECT_EQ(q.steps[1].done[0].op, Op::FlagEquals);
    EXPECT_EQ(q.steps[1].done[0].value, 2);
    EXPECT_TRUE(q.steps[1].targetMap.empty());
    ASSERT_EQ(q.complete.size(), 1u);
    ASSERT_EQ(q.fail.size(), 1u);
    EXPECT_EQ(q.fail[0].subject, "f.late");
    EXPECT_EQ(q.failTextKey, "t.fail");
    ASSERT_EQ(q.rewards.size(), 2u);
    EXPECT_EQ(q.rewards[0].itemId, "herb_a");
    EXPECT_EQ(q.rewards[0].count, 6);
    EXPECT_EQ(q.rewards[0].herbAge, 40);
    EXPECT_EQ(q.rewards[1].herbAge, 0) << "herb_age 缺省为 0";
    EXPECT_EQ(q.origin, "原著");
    EXPECT_EQ(q.note, "说明");
}

TEST(QuestLoading, FailAndItsReasonMayBothBeLeftOut) {
    std::string json = replaced(kGoodQuest,
                                R"json("fail": [ {"flag": "f.late", "op": ">=", "value": 1} ],)json", "");
    json = replaced(json, R"json("fail_text_key": "t.fail",)json", "");
    const auto loaded = loadQuestText(json);
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_TRUE(loaded.value.fail.empty());
    EXPECT_TRUE(loaded.value.failTextKey.empty());
}

struct BadQuest {
    const char* what;
    std::string json;
    const char* mustMention;   // 报错里必须点得出的那个字段 / 词
};

std::vector<BadQuest> badQuests() {
    const std::string& g = kGoodQuest;
    const std::string accept = R"json({"flag": "f.accept", "op": ">=", "value": 1})json";
    return {
        {"顶层不是对象", "[1, 2]", "顶层"},
        {"缺 id", replaced(g, R"json("id": "q_probe", )json", ""), "id"},
        {"缺 name", replaced(g, R"json("name": "探针", )json", ""), "name"},
        {"章号 0", replaced(g, R"json("chapter": 5)json", R"json("chapter": 0)json"), "chapter"},
        {"章号 15", replaced(g, R"json("chapter": 5)json", R"json("chapter": 15)json"), "chapter"},
        {"章号写成字符串", replaced(g, R"json("chapter": 5)json", R"json("chapter": "5")json"), "chapter"},
        {"kind 写成 main", replaced(g, R"json("kind": "side")json", R"json("kind": "main")json"), "kind"},
        {"缺 title_key", replaced(g, R"json("title_key": "t.title", )json", ""), "title_key"},
        {"缺 summary_key", replaced(g, R"json("summary_key": "t.summary",)json", ""), "summary_key"},
        {"accept 为空", replaced(g, "\"accept\": [ " + accept + " ]", R"json("accept": [])json"), "accept"},
        {"缺 accept", replaced(g, "\"accept\": [ " + accept + " ],", ""), "accept"},
        {"steps 为空",
         replaced(g, g.substr(g.find("\"steps\""), g.find("\"complete\"") - g.find("\"steps\"")),
                  R"json("steps": [], )json"),
         "steps"},
        {"步骤缺 text_key", replaced(g, R"json("text_key": "t.s2", )json", ""), "text_key"},
        {"步骤 done 为空",
         replaced(g, R"json("done": [ {"flag": "f.s2", "op": "==", "value": 2} ])json", R"json("done": [])json"),
         "done"},
        {"步骤 id 重复", replaced(g, R"json({"id": "s2")json", R"json({"id": "s1")json"), "重复"},
        {"target_map 落单", replaced(g, R"json(, "target_object": "o1")json", ""), "成对"},
        {"complete 为空",
         replaced(g, R"json("complete": [ {"flag": "f.done", "op": ">=", "value": 1} ])json",
                  R"json("complete": [])json"),
         "complete"},
        {"有 fail 没原因", replaced(g, R"json("fail_text_key": "t.fail",)json", ""), "fail_text_key"},
        {"没 fail 有原因",
         replaced(g, R"json("fail": [ {"flag": "f.late", "op": ">=", "value": 1} ],)json", ""),
         "fail_text_key"},
        {"谓词 flag 与 item 都写",
         replaced(g, R"json({"flag": "f.done", "op")json", R"json({"flag": "f.done", "item": "x", "op")json"),
         "恰好"},
        {"谓词 flag 与 item 都没写",
         replaced(g, R"json({"flag": "f.done", "op")json", R"json({"op")json"), "恰好"},
        {"op 写成 >", replaced(g, R"json({"flag": "f.done", "op": ">=")json", R"json({"flag": "f.done", "op": ">")json"),
         "op"},
        {"物品谓词用 ==",
         replaced(g, R"json({"item": "pill_x", "op": ">=")json", R"json({"item": "pill_x", "op": "==")json"), "物品"},
        {"旗标 >= 0",
         replaced(g, R"json({"flag": "f.done", "op": ">=", "value": 1})json",
                  R"json({"flag": "f.done", "op": ">=", "value": 0})json"),
         "至少为 1"},
        {"物品 >= 0",
         replaced(g, R"json({"item": "pill_x", "op": ">=", "value": 1})json",
                  R"json({"item": "pill_x", "op": ">=", "value": 0})json"),
         "至少为 1"},
        {"旗标 == -1",
         replaced(g, R"json({"flag": "f.s2", "op": "==", "value": 2})json",
                  R"json({"flag": "f.s2", "op": "==", "value": -1})json"),
         "负"},
        {"value 写成 true",
         replaced(g, R"json({"flag": "f.done", "op": ">=", "value": 1})json",
                  R"json({"flag": "f.done", "op": ">=", "value": true})json"),
         "value"},
        {"value 写成 1.5",
         replaced(g, R"json({"flag": "f.done", "op": ">=", "value": 1})json",
                  R"json({"flag": "f.done", "op": ">=", "value": 1.5})json"),
         "value"},
        {"value 写成 \"1\"",
         replaced(g, R"json({"flag": "f.done", "op": ">=", "value": 1})json",
                  R"json({"flag": "f.done", "op": ">=", "value": "1"})json"),
         "value"},
        {"顶层字段拼错", replaced(g, R"json("fail_text_key")json", R"json("fail_text")json"), "不认识的字段 fail_text"},
        {"步骤字段拼错", replaced(g, R"json("target_object": "o1")json", R"json("target_obj": "o1")json"),
         "不认识的字段 target_obj"},
        {"谓词字段拼错",
         replaced(g, R"json({"flag": "f.done", "op": ">=", "value": 1})json",
                  R"json({"flag": "f.done", "op": ">=", "val": 1})json"),
         "不认识的字段 val"},
        {"奖励 count 为 0", replaced(g, R"json("count": 6)json", R"json("count": 0)json"), "count"},
        {"奖励 herb_age 为负", replaced(g, R"json("herb_age": 40)json", R"json("herb_age": -1)json"), "herb_age"},
        {"奖励字段拼错", replaced(g, R"json({"item_id": "coin", "count": 2})json", R"json({"item_id": "coin", "n": 2})json"),
         "字段 n"},
    };
}

TEST(QuestLoading, EveryShapeTheContractRefusesIsRefusedAndSaysWhere) {
    // 先验：那份好文件本身读得进。否则下面每一条「被拒」都可能只是因为底子就坏了。
    ASSERT_TRUE(loadQuestText(kGoodQuest).ok);
    const std::vector<BadQuest> cases = badQuests();
    ASSERT_GE(cases.size(), 30u) << "契约 1.6 那张拒收表逐项都要有一条";
    for (const BadQuest& bad : cases) {
        ASSERT_NE(bad.json, kGoodQuest) << bad.what << "：夹具没改动，这条负例是空的";
        const auto loaded = loadQuestText(bad.json);
        EXPECT_FALSE(loaded.ok) << bad.what << " 却被收下了";
        if (loaded.ok) continue;
        EXPECT_NE(loaded.error.find(bad.mustMention), std::string::npos)
            << bad.what << " 的报错没点出「" << bad.mustMention << "」：" << loaded.error;
        EXPECT_NE(loaded.error.find("q_probe.json"), std::string::npos)
            << bad.what << " 的报错没写明是哪个文件：" << loaded.error;
    }
}

// ---- loadGameData：目标链原地收编成主线，外加支线目录 ----

const std::string kChainOne = R"json({"id":"objectives_ch01","name":"第一章","chapter":1,"steps":[
  {"id":"a1","text_key":"ta1","done_flag":"f.a1","target_map":"m","target_object":"o"},
  {"id":"a2","text_key":"ta2","done_flag":"f.a2","target_map":"m2","target_object":"o2"}]})json";
const std::string kChainTwo = R"json({"id":"objectives_ch02","name":"第二章","chapter":2,"steps":[
  {"id":"b1","text_key":"tb1","done_flag":"f.b1","target_map":"m","target_object":"o"}]})json";

TEST(QuestLoading, TheChainIsReadInPlaceAsOneMainQuestPerChapterAndTheChainItselfIsUntouched) {
    TempDir tmp{"fanren_quest_data"};
    const fs::path data = tmp.path() / "data";
    // 文件名倒序，确认主线按章号排而不是按文件名。
    writeFile(data / "objectives" / "zz.json", kChainOne);
    writeFile(data / "objectives" / "aa.json", kChainTwo);
    writeFile(data / "quests" / "q_probe.json", kGoodQuest);

    const auto loaded = fanren::io::loadGameData(data.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const GameData& d = loaded.value;

    // 目标链本身与从前一样：按章、按步。
    ASSERT_EQ(d.objectives.size(), 3u);
    EXPECT_EQ(d.objectives[0].id, "a1");
    EXPECT_EQ(d.objectives[1].id, "a2");
    EXPECT_EQ(d.objectives[2].id, "b1");

    ASSERT_EQ(d.quests.size(), 3u) << "两条主线 + 一条支线";
    EXPECT_EQ(d.quests[0].kind, QuestKind::Main);
    EXPECT_EQ(d.quests[0].id, "objectives_ch01");
    EXPECT_EQ(d.quests[0].name, "第一章");
    EXPECT_EQ(d.quests[0].chapter, 1);
    ASSERT_EQ(d.quests[0].steps.size(), 2u);
    EXPECT_EQ(d.quests[0].steps[1].id, "a2");
    EXPECT_EQ(d.quests[0].steps[1].textKey, "ta2");
    EXPECT_EQ(d.quests[0].steps[1].targetMap, "m2");
    EXPECT_EQ(d.quests[0].steps[1].targetObject, "o2");
    EXPECT_TRUE(d.quests[0].steps[1].done.empty()) << "主线的完成只认目标链判据，不另写谓词";
    EXPECT_TRUE(d.quests[0].accept.empty());
    EXPECT_EQ(d.quests[1].id, "objectives_ch02");
    EXPECT_EQ(d.quests[2].kind, QuestKind::Side);
    EXPECT_EQ(d.quests[2].id, "q_probe");
}

TEST(QuestLoading, TwoChainsForOneChapterAreRefused) {
    TempDir tmp{"fanren_quest_data"};
    const fs::path data = tmp.path() / "data";
    writeFile(data / "objectives" / "a.json", kChainOne);
    writeFile(data / "objectives" / "b.json",
              replaced(kChainTwo, R"json("chapter":2)json", R"json("chapter":1)json"));
    const auto loaded = fanren::io::loadGameData(data.string());
    EXPECT_FALSE(loaded.ok) << "同一章两条主线被收下了";
    EXPECT_NE(loaded.error.find("第 1 章"), std::string::npos) << loaded.error;

    // 对照：章号不撞就读得进。
    writeFile(data / "objectives" / "b.json", kChainTwo);
    EXPECT_TRUE(fanren::io::loadGameData(data.string()).ok);
}

TEST(QuestLoading, AChainWithoutAnIdIsRefusedNowThatItNamesAQuest) {
    TempDir tmp{"fanren_quest_data"};
    const fs::path data = tmp.path() / "data";
    writeFile(data / "objectives" / "a.json",
              replaced(kChainOne, R"json("id":"objectives_ch01",)json", ""));
    const auto loaded = fanren::io::loadGameData(data.string());
    EXPECT_FALSE(loaded.ok);
    EXPECT_NE(loaded.error.find("id"), std::string::npos) << loaded.error;
}

TEST(QuestLoading, ASideQuestMayNotReuseAnIdEitherOfAnotherSideQuestOrOfAChain) {
    TempDir tmp{"fanren_quest_data"};
    const fs::path data = tmp.path() / "data";
    writeFile(data / "objectives" / "a.json", kChainOne);
    writeFile(data / "quests" / "q_probe.json", kGoodQuest);
    writeFile(data / "quests" / "sub" / "q_probe_again.json", kGoodQuest);
    auto loaded = fanren::io::loadGameData(data.string());
    EXPECT_FALSE(loaded.ok) << "两份支线同一个 id";
    EXPECT_NE(loaded.error.find("重复"), std::string::npos) << loaded.error;

    fs::remove(data / "quests" / "sub" / "q_probe_again.json");
    writeFile(data / "quests" / "clash.json",
              replaced(kGoodQuest, R"json("id": "q_probe")json", R"json("id": "objectives_ch01")json"));
    loaded = fanren::io::loadGameData(data.string());
    EXPECT_FALSE(loaded.ok) << "支线与目标链同一个 id";
    EXPECT_NE(loaded.error.find("objectives_ch01"), std::string::npos) << loaded.error;
}

TEST(QuestLoading, NoQuestsFolderMeansNoSideQuestsNotAFailure) {
    TempDir tmp{"fanren_quest_data"};
    const fs::path data = tmp.path() / "data";
    writeFile(data / "objectives" / "a.json", kChainOne);
    const auto loaded = fanren::io::loadGameData(data.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_EQ(loaded.value.quests.size(), 1u);
    EXPECT_EQ(loaded.value.quests[0].kind, QuestKind::Main);
}

// ===========================================================================
// 七、真数据：主线逐步对应目标链；第 1–4 章零变化
// ===========================================================================

class ShippedQuests : public ::testing::Test {
protected:
    void SetUp() override {
        auto loaded = fanren::io::loadGameData((fs::path(assetRoot()) / "data").string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        data_ = std::move(loaded.value);
        ASSERT_FALSE(data_.objectives.empty()) << "一条目标都没加载到";
    }

    // 改动之前告示板下半截的算法，逐字抄自第 5 章之前的 BoardScene::onEnter：
    // 做过的主线步骤，倒序，只有正文。
    [[nodiscard]] std::vector<fanren::ui::ListItem> rowsBeforeChapterFive(const GameState& state) const {
        std::vector<fanren::ui::ListItem> items;
        for (const Objective* step : fanren::rules::completedObjectives(data_.objectives, state)) {
            fanren::ui::ListItem row;
            row.label = data_.lookupText(step->textKey);
            items.push_back(std::move(row));
        }
        std::reverse(items.begin(), items.end());
        return items;
    }

    GameData data_;
};

TEST_F(ShippedQuests, EveryChainFileBecomesOneMainQuestThatMirrorsItStepByStep) {
    std::vector<int> chaptersWithSteps;
    for (const Objective& o : data_.objectives) {
        if (chaptersWithSteps.empty() || chaptersWithSteps.back() != o.chapter) {
            chaptersWithSteps.push_back(o.chapter);
        }
    }
    ASSERT_GE(chaptersWithSteps.size(), 4u) << "先验：第 1–4 章都有目标链";

    for (int chapter : chaptersWithSteps) {
        std::vector<const Quest*> mains;
        for (const Quest& q : data_.quests) {
            if (q.kind == QuestKind::Main && q.chapter == chapter) mains.push_back(&q);
        }
        ASSERT_EQ(mains.size(), 1u) << "第 " << chapter << " 章该有且只有一条主线任务";
        std::vector<const Objective*> chain;
        for (const Objective& o : data_.objectives) {
            if (o.chapter == chapter) chain.push_back(&o);
        }
        ASSERT_EQ(mains[0]->steps.size(), chain.size()) << "第 " << chapter << " 章步数对不上";
        for (std::size_t i = 0; i < chain.size(); ++i) {
            const QuestStep& step = mains[0]->steps[i];
            EXPECT_EQ(step.id, chain[i]->id);
            EXPECT_EQ(step.textKey, chain[i]->textKey);
            EXPECT_EQ(step.targetMap, chain[i]->targetMap);
            EXPECT_EQ(step.targetObject, chain[i]->targetObject);
        }
    }
}

TEST_F(ShippedQuests, WalkingTheRealChainKeepsExactlyTheHudChapterActive) {
    // 按真链逐个置位：每一个前缀上，进行中的主线任务至多一条，且正是 HUD 那一步所在的章，
    // 当前步骤正是 HUD 那一步。之前的章已了结，之后的章未接。
    GameState state;
    std::size_t walked = 0;
    for (std::size_t i = 0; i <= data_.objectives.size(); ++i) {
        const Objective* hud = fanren::rules::currentObjective(data_.objectives, state);
        int active = 0;
        for (const Quest& q : data_.quests) {
            if (q.kind != QuestKind::Main) continue;
            const QuestStatus status = questStatus(q, data_.objectives, state);
            if (status == QuestStatus::Active) {
                ++active;
                ASSERT_NE(hud, nullptr) << "目标链走完了，却还有一条主线在进行中：" << q.id;
                EXPECT_EQ(q.chapter, hud->chapter) << "进行中的主线不是 HUD 那一章";
                const QuestStep* step = currentQuestStep(q, data_.objectives, state);
                ASSERT_NE(step, nullptr) << q.id << " 进行中却说不出当前步骤";
                EXPECT_EQ(step->id, hud->id) << "告示板与 HUD 说着两件不同的事";
            } else if (hud != nullptr) {
                EXPECT_EQ(status, q.chapter < hud->chapter ? QuestStatus::Completed
                                                           : QuestStatus::NotAccepted)
                    << q.id << " 在第 " << i << " 步时状态不对";
            } else {
                EXPECT_EQ(status, QuestStatus::Completed) << "全部走完，" << q.id << " 该已了结";
            }
        }
        EXPECT_EQ(active, hud == nullptr ? 0 : 1) << "第 " << i << " 步时进行中的主线有 " << active << " 条";
        if (i < data_.objectives.size()) state.setFlag(data_.objectives[i].doneFlag);
        ++walked;
    }
    EXPECT_GT(walked, data_.objectives.size()) << "先验：真的走完了整条链";
}

TEST_F(ShippedQuests, ChapterOneToFourBoardRowsAreExactlyWhatTheyWereBeforeQuestsExisted) {
    // 第 1–4 章零变化（契约 1.9、设计 Q8）。沿第 1–4 章真链的每一个前缀：
    // 告示板下半截与改动前的算法逐行相同——一行支线也不许冒出来，主线那几行一个字不变。
    GameState state;
    std::size_t prefixes = 0;
    std::size_t rowsSeen = 0;
    for (const Objective& step : data_.objectives) {
        if (step.chapter > 4) break;
        const auto now = BoardScene::buildRows(data_, state);
        const auto before = rowsBeforeChapterFive(state);
        ASSERT_EQ(now.size(), before.size()) << "走到 " << step.id << " 时告示板多了或少了行";
        for (std::size_t r = 0; r < now.size(); ++r) {
            EXPECT_EQ(now[r].label, before[r].label) << step.id << " 第 " << r << " 行变了";
            EXPECT_EQ(now[r].detail, before[r].detail);
            EXPECT_EQ(now[r].enabled, before[r].enabled);
        }
        rowsSeen += now.size();
        ++prefixes;
        state.setFlag(step.doneFlag);
    }
    // 先验分母：真的走过了第 1–4 章的步骤、真的比过了行。
    EXPECT_GE(prefixes, 40u) << "第 1–4 章的步骤数不对，这条用例可能在空转";
    EXPECT_GT(rowsSeen, 100u) << "一路上比过的行太少";
}

// ===========================================================================
// 八、告示板：三种支线行
// ===========================================================================

TEST(QuestBoard, ActiveFailedAndCompletedSideQuestsEachShowTheirOwnLineAboveTheMainSteps) {
    GameData data;
    data.text = {{"t.title.a", "医书"},       {"t.title.b", "剑符"},   {"t.title.c", "清灵散"},
                 {"t.title.d", "没接的"},     {"t.s1", "誊抄手札"},   {"t.s2", "交给墨凤舞"},
                 {"t.s3", "练两天"},          {"t.summary", "那件事"}, {"t.fail.b", "欧阳飞天已死"},
                 {"ch05.map.kezhan.name", "汇源客栈"},                  {"objective.a1", "主线第一步"},
                 {"objective.a2", "主线第二步"}};
    Objective a1;
    a1.id = "a1";
    a1.textKey = "objective.a1";
    a1.doneFlag = "f.a1";
    a1.chapter = 5;
    Objective a2 = a1;
    a2.id = "a2";
    a2.textKey = "objective.a2";
    a2.doneFlag = "f.a2";
    data.objectives = {a1, a2};

    Quest active = threeStepQuest();
    active.id = "q_a";
    active.titleKey = "t.title.a";
    active.accept = {flagAtLeast("f.accept.a", 1)};
    active.steps = {stepOf("s1", {flagAtLeast("f.s1", 1)}), stepOf("s2", {flagAtLeast("f.s2", 1)})};
    active.steps[0].targetMap = "ch05_kezhan";
    active.steps[0].targetObject = "trigger_chaoxie";
    Quest failed = threeStepQuest();
    failed.id = "q_b";
    failed.titleKey = "t.title.b";
    failed.accept = {flagAtLeast("f.accept.b", 1)};
    failed.fail = {flagAtLeast("f.late.b", 1)};
    failed.failTextKey = "t.fail.b";
    Quest completed = threeStepQuest();
    completed.id = "q_c";
    completed.titleKey = "t.title.c";
    completed.accept = {flagAtLeast("f.accept.c", 1)};
    completed.complete = {flagAtLeast("f.done.c", 1)};
    Quest untaken = threeStepQuest();
    untaken.id = "q_d";
    untaken.titleKey = "t.title.d";
    untaken.accept = {flagAtLeast("f.accept.d", 1)};
    data.quests = {untaken, completed, failed, active};

    GameState state;
    for (const char* flag : {"f.accept.a", "f.accept.b", "f.accept.c", "f.late.b", "f.done.c", "f.a1"}) {
        state.setFlag(flag, 1);
    }

    const auto rows = BoardScene::buildRows(data, state);
    ASSERT_EQ(rows.size(), 4u) << "三条支线（没接的不列）+ 一步做过的主线";

    EXPECT_EQ(rows[0].label, "医书：誊抄手札（汇源客栈）") << "进行中：任务名、当前步骤、地名";
    EXPECT_EQ(rows[0].detail, BoardScene::kActiveTag);

    // 过期那一行：先验原因确实有内容，再验它真的写在了那一行上。
    const std::string reason = data.lookupText("t.fail.b");
    ASSERT_FALSE(reason.empty());
    ASSERT_NE(reason, "t.fail.b") << "先验：原因那句文案查得到";
    EXPECT_EQ(rows[1].label, "剑符：" + reason) << "已过期要写原因（Q7）";
    EXPECT_EQ(rows[1].detail, BoardScene::kFailedTag);

    EXPECT_EQ(rows[2].label, "清灵散");
    EXPECT_EQ(rows[2].detail, BoardScene::kCompletedTag);

    EXPECT_EQ(rows[3].label, "主线第一步") << "支线之后是做过的主线步骤";
    EXPECT_TRUE(rows[3].detail.empty()) << "主线那几行与从前一样没有右栏";

    for (const auto& row : rows) {
        EXPECT_EQ(row.label.find("没接的"), std::string::npos) << "没接的任务出现在了告示板上";
    }

    // 步骤都做完而没了结：改说 summary。
    state.setFlag("f.s1", 1);
    state.setFlag("f.s2", 1);
    const auto later = BoardScene::buildRows(data, state);
    ASSERT_FALSE(later.empty());
    EXPECT_EQ(later[0].label, "医书：那件事");
    EXPECT_EQ(later[0].detail, BoardScene::kActiveTag);
}

// ===========================================================================
// 九、游戏层：HUD 与指路不看支线；开告示板不发奖励
// ===========================================================================
//
// 夹具资源根：data/、scripts/、maps/ 原样复制一份，再在 data/quests/ 里放一条探针支线
// （正式的 data/quests/ 归内容路，不往里写）。探针的步骤指向另一张图——若 HUD 或指路
// 跟着支线走，亮的门就会变。

fs::path& probeRoot() {
    static fs::path path;
    return path;
}

const std::string kProbeQuestId = "q_test_board_probe";

class QuestInGame : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        const fs::path source(assetRoot());
        fs::path& root = probeRoot();
        root = fanren::test::uniqueTempPath("fanren_quest_root");
        std::error_code ec;
        fs::create_directories(root, ec);
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(source / dir, root / dir, fs::copy_options::recursive, ec);
            ASSERT_FALSE(ec) << "复制 " << dir << "/ 失败: " << ec.message();
        }
        std::string probe = replaced(kGoodQuest, R"json("id": "q_probe")json",
                                     "\"id\": \"" + kProbeQuestId + "\"");
        // 接取条件用第 1 章的一个旗标（ch01.done），步骤指向第 2 章的药铺。
        probe = replaced(probe, R"json({"flag": "f.accept", "op": ">=", "value": 1})json",
                         R"json({"flag": "ch01.done", "op": ">=", "value": 1})json");
        probe = replaced(probe, R"json({"item": "pill_x", "op": ">=", "value": 1})json",
                         R"json({"flag": "f.never", "op": ">=", "value": 1})json");
        probe = replaced(probe, R"json("target_map": "m1", "target_object": "o1")json",
                         R"json("target_map": "ch02_yaopu", "target_object": "npc_probe")json");
        probe = replaced(probe, R"json({"flag": "f.done", "op": ">=", "value": 1})json",
                         R"json({"flag": "f.probe_done", "op": ">=", "value": 1})json");
        writeFile(root / "data" / "quests" / (kProbeQuestId + ".json"), probe);
    }
    static void TearDownTestSuite() {
        std::error_code ec;
        fs::remove_all(probeRoot(), ec);
    }

    void SetUp() override {
        auto ready = app_.init(probeRoot().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    // 第 3 章终局站在谷外（与 ObjectiveTests 的 ObjectiveRoute 同一个站位）。
    void standAtChapterThreeEnding() {
        GameState& s = app_.state();
        s.setFlag("ch01.done");
        s.setFlag("ch01.muqin_bie");
        s.setFlag("ch02.done");
        s.setFlag("ch03.done");
        auto loaded = app_.loadMap("ch03_guwai", "");
        ASSERT_TRUE(loaded.ok) << loaded.error;
        s.position = fanren::core::Point{19, 25};
    }

    const Quest* probe() {
        for (const Quest& q : app_.data().quests) {
            if (q.id == kProbeQuestId) return &q;
        }
        return nullptr;
    }

    fanren::game::Application app_;
};

TEST_F(QuestInGame, AnActiveSideQuestDoesNotMoveTheHudOrTheLitDoor) {
    standAtChapterThreeEnding();
    ASSERT_FALSE(HasFatalFailure());
    const Quest* side = probe();
    ASSERT_NE(side, nullptr) << "先验：探针支线真的被加载器读进来了";
    ASSERT_EQ(questStatus(*side, app_.data().objectives, app_.state()), QuestStatus::Active)
        << "先验：探针支线此刻在进行中";
    const QuestStep* step = currentQuestStep(*side, app_.data().objectives, app_.state());
    ASSERT_NE(step, nullptr);
    ASSERT_EQ(step->targetMap, "ch02_yaopu") << "先验：支线那一步指着另一张图";

    // HUD 那一步仍是主线的；亮的门仍是去主线目标的那一道（Q5、Q9）。
    const Objective* hud = fanren::rules::currentObjective(app_.data().objectives, app_.state());
    ASSERT_NE(hud, nullptr);
    EXPECT_EQ(hud->targetMap, "ch04_getang") << "HUD 该指第 4 章第一步";
    const fanren::rules::PortalLink* hop = fanren::game::WorldScene::guidePortal(app_);
    ASSERT_NE(hop, nullptr);
    EXPECT_EQ(hop->targetMap, "ch01_shenshougu") << "亮的门被支线带走了";
    const std::string where = fanren::game::WorldScene::objectiveWhereText(app_);
    const std::string yaopu = app_.text(fanren::core::mapDisplayNameKey("ch02_yaopu"));
    ASSERT_NE(yaopu, fanren::core::mapDisplayNameKey("ch02_yaopu")) << "先验：药铺的地名有文案";
    EXPECT_EQ(where.find(yaopu), std::string::npos) << "HUD 第二行报了支线的地名：" << where;

    // 告示板上它确实在（对照：探针不是因为没接才不影响 HUD）。
    const auto rows = BoardScene::buildRows(app_.data(), app_.state());
    ASSERT_FALSE(rows.empty());
    EXPECT_EQ(rows[0].detail, BoardScene::kActiveTag);
}

TEST_F(QuestInGame, OpeningTheBoardOnACompletedQuestGivesNothing) {
    // 任务系统不发奖励（Q4）：探针写着六株四十年的 herb_a 与两个 coin，
    // 了结之后开一次告示板，存档一个字节也不许变。
    standAtChapterThreeEnding();
    ASSERT_FALSE(HasFatalFailure());
    app_.state().setFlag("f.probe_done");
    const Quest* side = probe();
    ASSERT_NE(side, nullptr);
    ASSERT_EQ(questStatus(*side, app_.data().objectives, app_.state()), QuestStatus::Completed);
    ASSERT_FALSE(side->rewards.empty()) << "先验：这条任务确实写着奖励";

    const fs::path before = fanren::test::uniqueTempPath("fanren_quest_before", ".json");
    const fs::path after = fanren::test::uniqueTempPath("fanren_quest_after", ".json");
    ASSERT_TRUE(fanren::io::saveGame(app_.state(), before.string()).ok);

    BoardScene board;
    board.onEnter(app_);
    static_cast<void>(fanren::rules::sideQuestJournal(app_.data().quests, app_.state()));

    ASSERT_TRUE(fanren::io::saveGame(app_.state(), after.string()).ok);
    std::ifstream a(before, std::ios::binary);
    std::ifstream b(after, std::ios::binary);
    const std::string textBefore((std::istreambuf_iterator<char>(a)), std::istreambuf_iterator<char>());
    const std::string textAfter((std::istreambuf_iterator<char>(b)), std::istreambuf_iterator<char>());
    a.close();
    b.close();
    std::error_code ec;
    fs::remove(before, ec);
    fs::remove(after, ec);
    ASSERT_GT(textBefore.size(), 100u) << "先验：真的写出了存档";
    EXPECT_EQ(textBefore, textAfter) << "开了一次告示板，存档变了：任务系统不许发奖励";
    EXPECT_EQ(app_.state().itemCount("herb_a"), 0);
}

}  // namespace
