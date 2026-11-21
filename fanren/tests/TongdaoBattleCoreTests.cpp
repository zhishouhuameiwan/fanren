// 同道录 2a·路 K：战斗规则层（core/battle）新加的几样，直接驱动 BattleState，不开 Application。
//   * 开场的劲：先手、她的被动、同修不叠加取大；在册加成的偷袭劲叠在先手上；封顶 5；
//   * 法术的基础段数（Magic::hits）；四种辅助效果（回血、护持、剑幕、定身）；剑幕的时效（返修第 2 条：管到施术者下一次轮到）；
//   * 打赢回血、符的威力加成、主角某属法术多削架势、她的追击加成；
//   * 切磋：不追击、不合招。
// 走 Application 真入口的那一半（建场、切磋编成、同修）在 TongdaoWiringTests.cpp。
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/AttackCategory.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"

namespace {

using fanren::core::Item;
using fanren::core::Magic;
using fanren::core::MagicEffect;
using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattleEvent;
using fanren::core::battle::BattleEventKind;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::BattleState;
using fanren::core::battle::SetupOptions;
using fanren::core::battle::Unit;
using fanren::rules::Realm;

Unit hero(int speed = 50) {
    Unit u;
    u.id = "hanli";
    u.name = "韩立";
    u.hero = true;
    u.ally = true;
    u.hp = u.maxHp = 200;
    u.mp = u.maxMp = 200;
    u.attack = 10;
    u.defence = 5;
    u.speed = speed;
    u.realm = Realm::QiRefining10;
    u.weapons = fanren::core::kCategorySword;
    u.magicsExhaustive = true;
    return u;
}

Unit companion(const std::string& id, int bond, int speed = 40) {
    Unit u = hero(speed);
    u.id = id;
    u.name = id;
    u.hero = false;
    u.companion = id;
    u.bond = bond;
    u.canChase = true;
    return u;
}

Unit foe(const std::string& id = "dummy", int speed = 1, int toughness = 3, int weaknesses = 0) {
    Unit u;
    u.id = id;
    u.name = id;
    u.ally = false;
    u.hp = u.maxHp = 5000;
    u.attack = 30;
    u.defence = 0;
    u.speed = speed;
    u.realm = Realm::QiRefining10;
    u.maxToughness = toughness;
    u.weaknesses = weaknesses;
    u.weapons = fanren::core::kCategoryFist;
    u.magicsExhaustive = true;
    return u;
}

Magic magic(const std::string& id, int power, int element = 0) {
    Magic m;
    m.id = id;
    m.name = id;
    m.power = power;
    m.needMp = 0;
    m.element = element;
    m.needRealm = Realm::QiRefining1;
    return m;
}

Magic support(const std::string& id, MagicEffect effect, int amount = 0) {
    Magic m = magic(id, 0);
    m.effect = effect;
    m.amount = amount;
    return m;
}

Action act(ActionKind kind, int actor, int target = -1, const std::string& magicId = {}) {
    Action a;
    a.kind = kind;
    a.actorIndex = actor;
    a.targetIndex = target;
    a.magicId = magicId;
    return a;
}

std::vector<BattleEvent> since(const BattleState& b, std::size_t from) {
    return {b.events().begin() + static_cast<std::ptrdiff_t>(from), b.events().end()};
}
int count(const std::vector<BattleEvent>& events, BattleEventKind kind) {
    return static_cast<int>(std::count_if(events.begin(), events.end(), [kind](const BattleEvent& e) { return e.kind == kind; }));
}

// 推到 who 轮到为止：别的我方空过这一手（不防御——防御会把后面量的伤害减半），敌方照 AI。
void turnOf(BattleState& b, int who) {
    for (int guard = 0; guard < 200 && b.phase() == BattlePhase::Ongoing; ++guard) {
        const int actor = b.currentActor();
        if (actor == who) return;
        if (actor >= 0 && !b.units()[static_cast<std::size_t>(actor)].ally) {
            ASSERT_TRUE(b.apply(b.decideAi(actor)).ok);
        }
        b.endTurn();
    }
}

// ---------------------------------------------------------------------------
// 开场的劲
// ---------------------------------------------------------------------------

TEST(TongdaoBattleOpening, DefaultsAreTheOldOpening) {
    BattleState b;
    b.setup({hero(), foe()}, 1);
    EXPECT_EQ(b.units()[0].bp, fanren::core::battle::kBpStart);
    BattleState p;
    SetupOptions o;
    o.preemptive = true;
    p.setup({hero(), foe()}, 1, o);
    EXPECT_EQ(p.units()[0].bp, fanren::core::battle::kBpStart + fanren::core::battle::kPreemptiveBonusBp);
}

TEST(TongdaoBattleOpening, PassiveTongxiuAndPreemptiveTakeTheLargestAmbushAddsOnTop) {
    SetupOptions o;
    o.openingBp = 1;
    BattleState a;
    a.setup({hero(), foe()}, 1, o);
    EXPECT_EQ(a.units()[0].bp, 2);
    EXPECT_EQ(a.units()[1].bp, 0) << "敌方没有劲";

    o.preemptive = true;
    BattleState b;
    b.setup({hero(), foe()}, 1, o);
    EXPECT_EQ(b.units()[0].bp, 2) << "先手的 1 点与被动的 1 点不叠加";

    o.ambushBp = 1;
    BattleState c;
    c.setup({hero(), foe()}, 1, o);
    EXPECT_EQ(c.units()[0].bp, 3) << "偷袭劲叠在先手上";

    o.preemptive = false;
    BattleState d;
    d.setup({hero(), foe()}, 1, o);
    EXPECT_EQ(d.units()[0].bp, 2) << "不是先手：偷袭劲不读";

    o.preemptive = true;
    o.openingBp = 2;
    o.ambushBp = 9;
    BattleState e;
    e.setup({hero(), foe()}, 1, o);
    EXPECT_EQ(e.units()[0].bp, fanren::core::battle::kBpMax) << "封顶 5";
}

// ---------------------------------------------------------------------------
// 段数与辅助效果
// ---------------------------------------------------------------------------

TEST(TongdaoBattleMagic, ABaseHitCountStrikesThatManyTimesAndHitsBoostAddsOnTop) {
    BattleState b;
    Unit h = hero();
    h.magics = {"twin"};
    b.setup({h, foe()}, 1);
    Magic twin = magic("twin", 10, fanren::core::kElementMetal);
    twin.hits = 2;
    b.addMagic(twin);
    turnOf(b, 0);
    const std::size_t from = b.events().size();
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 0, 1, "twin")).ok);
    EXPECT_EQ(count(since(b, from), BattleEventKind::Hit), 2);
    EXPECT_EQ(b.hitCount(act(ActionKind::Cast, 0, 1, "twin")), 2);

    Magic rain = twin;
    rain.id = "rain";
    rain.boost = fanren::core::MagicBoost::Hits;
    b.addMagic(rain);
    Action boosted = act(ActionKind::Cast, 0, 1, "rain");
    boosted.boost = 2;
    EXPECT_EQ(b.hitCount(boosted), 4) << "基础 2 段 + 蓄劲 2 段";
}

TEST(TongdaoBattleMagic, HealGoesToAWoundedAllyOnly) {
    BattleState b;
    Unit h = hero();
    h.magics = {"heal"};
    h.hp = 50;
    Unit mate = companion("mate", 0);
    b.setup({h, mate, foe()}, 1);
    b.addMagic(support("heal", MagicEffect::Heal, 80));
    turnOf(b, 0);
    std::string why;
    EXPECT_FALSE(b.isLegal(act(ActionKind::Cast, 0, 2, "heal"), &why)) << "不能给敌人回血";
    EXPECT_FALSE(b.isLegal(act(ActionKind::Cast, 0, 1, "heal"), &why)) << "满血的回不了";
    EXPECT_NE(why.find("满"), std::string::npos) << why;
    const std::size_t from = b.events().size();
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 0, 0, "heal")).ok);
    EXPECT_EQ(b.units()[0].hp, 130);
    const auto events = since(b, from);
    EXPECT_EQ(count(events, BattleEventKind::Heal), 1);
    EXPECT_EQ(count(events, BattleEventKind::Hit), 0) << "不是一击";
}

TEST(TongdaoBattleMagic, AWardSwallowsTheNextHitAndOnlyThatOne) {
    BattleState b;
    Unit h = hero(50);
    h.magics = {"ward"};
    b.setup({h, foe("brute", 10, 0)}, 1);
    b.addMagic(support("ward", MagicEffect::Ward));
    turnOf(b, 0);
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 0, 0, "ward")).ok);
    EXPECT_TRUE(b.units()[0].warded(b.round()));
    EXPECT_FALSE(b.isLegal(act(ActionKind::Cast, 0, 0, "ward"))) << "已经有护持了";
    b.endTurn();
    const int before = b.units()[0].hp;
    std::size_t from = b.events().size();
    ASSERT_TRUE(b.apply(act(ActionKind::Attack, 1, 0)).ok);
    EXPECT_EQ(b.units()[0].hp, before) << "护持挡下这一击";
    EXPECT_FALSE(b.units()[0].warded(b.round()));
    EXPECT_EQ(count(since(b, from), BattleEventKind::WardBlock), 1);
    b.endTurn();
    turnOf(b, 1);
    from = b.events().size();
    ASSERT_TRUE(b.apply(act(ActionKind::Attack, 1, 0)).ok);
    EXPECT_LT(b.units()[0].hp, before) << "第二击照常打进来";
    EXPECT_EQ(count(since(b, from), BattleEventKind::WardBlock), 0);
}

// 一记不带剑幕的普攻打多少（hero(50) 对 brute：恶狼攻 30、韩立防 5）。
int plainBruteHit() {
    BattleState plain;
    plain.setup({hero(50), foe("brute", 10, 0)}, 1);
    plain.endTurn();
    EXPECT_TRUE(plain.apply(act(ActionKind::Attack, 1, 0)).ok);
    return 200 - plain.units()[0].hp;
}

// 事件 kind 在 events 里第一次出现的下标；没有返回 -1。
int firstOf(const std::vector<BattleEvent>& events, BattleEventKind kind) {
    for (std::size_t i = 0; i < events.size(); ++i) {
        if (events[i].kind == kind) return static_cast<int>(i);
    }
    return -1;
}

// 返修（docs/ch07-tongdaolu-fix1.md 第 2 条）：原名 ABulwarkCutsDamageForTheWholeSideUntilTheRoundEnds，钉的是「只管这一回合」。
// 剑幕现在管到施术者下一次轮到为止；这里韩立（身法 50）在下一回合排在恶狼前头，轮到他时剑幕就散，恶狼那一下照常。
TEST(TongdaoBattleMagic, ABulwarkCutsDamageForTheWholeSideUntilTheCasterActsAgain) {
    const int full = plainBruteHit();
    ASSERT_GT(full, 10);

    BattleState b;
    Unit h = hero(50);
    h.magics = {"wall"};
    b.setup({h, companion("mate", 0, 1), foe("brute", 10, 0)}, 1);
    b.addMagic(support("wall", MagicEffect::Bulwark, 50));
    turnOf(b, 0);
    std::size_t from = b.events().size();
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 0, -1, "wall")).ok);
    EXPECT_EQ(count(since(b, from), BattleEventKind::Bulwark), 1);
    EXPECT_EQ(b.units()[1].status.bulwarkPercent, 50) << "整边都护着";
    EXPECT_EQ(b.units()[1].status.bulwarkCaster, 0) << "记着是谁张的";
    b.endTurn();
    turnOf(b, 2);
    ASSERT_TRUE(b.apply(act(ActionKind::Attack, 2, 0)).ok);
    const int cut = 200 - b.units()[0].hp;
    EXPECT_EQ(cut, (full + 1) / 2) << "减五成（四舍五入）";
    from = b.events().size();
    b.endTurn();
    turnOf(b, 2);   // 下一回合：韩立先轮到（剑幕散），再到恶狼
    EXPECT_EQ(count(since(b, from), BattleEventKind::BulwarkEnd), 2) << "罩着两位，各散一条";
    EXPECT_EQ(b.units()[0].status.bulwarkPercent, 0);
    EXPECT_EQ(b.units()[1].status.bulwarkPercent, 0);
    const int before = b.units()[0].hp;
    ASSERT_TRUE(b.apply(act(ActionKind::Attack, 2, 0)).ok);
    EXPECT_EQ(before - b.units()[0].hp, full) << "轮到施术者剑幕就散了";
}

// 返修第 2 条：她排在回合末张开，剑幕翻过回合接着管，下一回合抢在她前头出手的敌人照样吃减伤；轮到她时才散。
TEST(TongdaoBattleMagic, ABulwarkCastLastInTheRoundStillCoversTheNextRoundUntilHerTurn) {
    const int full = plainBruteHit();
    BattleState b;
    Unit chen = companion("chen", 0, 5);   // 最慢：回合末才轮到她
    chen.magics = {"wall"};
    b.setup({hero(50), chen, foe("brute", 30, 0)}, 1);   // 次序：韩立、恶狼、她
    b.addMagic(support("wall", MagicEffect::Bulwark, 50));
    turnOf(b, 1);
    ASSERT_EQ(b.round(), 1);
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 1, -1, "wall")).ok);
    EXPECT_EQ(b.units()[0].status.bulwarkPercent, 50);
    b.endTurn();   // 她是这一回合最后一位：行动者推成 -1
    b.endTurn();   // 第 2 回合，韩立先手
    ASSERT_EQ(b.round(), 2);
    ASSERT_EQ(b.currentActor(), 0);
    EXPECT_EQ(b.units()[0].status.bulwarkPercent, 50) << "翻过回合还罩着";
    b.endTurn();
    ASSERT_EQ(b.currentActor(), 2);
    const int before = b.units()[0].hp;
    ASSERT_TRUE(b.apply(act(ActionKind::Attack, 2, 0)).ok);
    EXPECT_EQ(before - b.units()[0].hp, (full + 1) / 2) << "第 2 回合抢在她前头的这一下照样减五成";
    const std::size_t from = b.events().size();
    b.endTurn();   // 轮到她
    ASSERT_EQ(b.currentActor(), 1);
    const auto events = since(b, from);
    EXPECT_EQ(count(events, BattleEventKind::BulwarkEnd), 2);
    for (const BattleEvent& e : events) {
        if (e.kind == BattleEventKind::BulwarkEnd) EXPECT_EQ(e.actor, 1) << "散的是她张的那一道";
    }
    EXPECT_EQ(b.units()[0].status.bulwarkPercent, 0);
    EXPECT_EQ(b.units()[1].status.bulwarkPercent, 0);
    EXPECT_EQ(b.units()[0].status.bulwarkCaster, -1);
}

// 返修第 2 条：她那一手被定身跳过也算轮到——剑幕照样散，而且散在「动弹不得」之前。
// 我方没有会定身的敌人，这里让敌方那一边的人张剑幕、韩立去定它（规则层不分敌我）。
TEST(TongdaoBattleMagic, ABoundCasterLosesHerTurnAndHerBulwarkWithIt) {
    BattleState b;
    Unit h = hero(50);
    h.magics = {"bind"};
    Unit caster = foe("caster", 30, 0);
    caster.magics = {"wall"};
    b.setup({h, caster, foe("other", 10, 0)}, 1);
    b.addMagic(support("wall", MagicEffect::Bulwark, 50));
    b.addMagic(support("bind", MagicEffect::Bind));
    turnOf(b, 1);
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 1, -1, "wall")).ok);
    EXPECT_EQ(b.units()[2].status.bulwarkPercent, 50) << "罩着她那一边";
    EXPECT_EQ(b.units()[0].status.bulwarkPercent, 0) << "不罩对面";
    b.endTurn();
    turnOf(b, 0);   // 第 2 回合，韩立
    ASSERT_EQ(b.round(), 2);
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 0, 1, "bind")).ok);
    const std::size_t from = b.events().size();
    b.endTurn();   // 轮到她：被定住跳过
    EXPECT_EQ(b.currentActor(), 2);
    const auto events = since(b, from);
    ASSERT_EQ(count(events, BattleEventKind::BoundSkip), 1);
    EXPECT_EQ(count(events, BattleEventKind::BulwarkEnd), 2);
    EXPECT_LT(firstOf(events, BattleEventKind::BulwarkEnd), firstOf(events, BattleEventKind::BoundSkip))
        << "先散剑幕，再动弹不得";
    EXPECT_EQ(b.units()[1].status.bulwarkPercent, 0);
    EXPECT_EQ(b.units()[2].status.bulwarkPercent, 0);
}

// 返修第 2 条：施术者倒下，剑幕当场散（紧跟在她的 Fall 之后），不等轮到她。
TEST(TongdaoBattleMagic, ABulwarkFallsWithItsCaster) {
    BattleState b;
    Unit chen = companion("chen", 0, 50);
    chen.magics = {"wall"};
    chen.hp = 1;
    b.setup({hero(5), chen, foe("brute", 30, 0)}, 1);   // 次序：她、恶狼、韩立
    b.addMagic(support("wall", MagicEffect::Bulwark, 50));
    turnOf(b, 1);
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 1, -1, "wall")).ok);
    b.endTurn();
    ASSERT_EQ(b.currentActor(), 2);
    const std::size_t from = b.events().size();
    ASSERT_TRUE(b.apply(act(ActionKind::Attack, 2, 1)).ok);
    const auto events = since(b, from);
    ASSERT_EQ(count(events, BattleEventKind::Fall), 1);
    EXPECT_EQ(count(events, BattleEventKind::BulwarkEnd), 2);
    EXPECT_LT(firstOf(events, BattleEventKind::Fall), firstOf(events, BattleEventKind::BulwarkEnd));
    EXPECT_EQ(b.units()[0].status.bulwarkPercent, 0) << "韩立身上的也散了";
    EXPECT_EQ(b.phase(), BattlePhase::Ongoing);
}

// 返修第 2 条：重放就是刷新——轮到她时旧的先散，再张开的那一道照新的数、管到她再下一次轮到。
TEST(TongdaoBattleMagic, RecastingABulwarkRefreshesIt) {
    const int full = plainBruteHit();
    BattleState b;
    Unit chen = companion("chen", 0, 50);
    chen.magics = {"wall", "thin"};
    b.setup({hero(5), chen, foe("brute", 30, 0)}, 1);   // 次序：她、恶狼、韩立
    b.addMagic(support("wall", MagicEffect::Bulwark, 50));
    b.addMagic(support("thin", MagicEffect::Bulwark, 30));
    turnOf(b, 1);
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 1, -1, "wall")).ok);
    b.endTurn();
    turnOf(b, 1);   // 第 2 回合轮到她
    ASSERT_EQ(b.round(), 2);
    EXPECT_EQ(b.units()[0].status.bulwarkPercent, 0) << "轮到她时旧的先散";
    const std::size_t from = b.events().size();
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 1, -1, "thin")).ok);
    EXPECT_EQ(count(since(b, from), BattleEventKind::Bulwark), 1);
    EXPECT_EQ(b.units()[0].status.bulwarkPercent, 30) << "照新的数";
    EXPECT_EQ(b.units()[0].status.bulwarkCaster, 1);
    b.endTurn();
    ASSERT_EQ(b.currentActor(), 2);
    const int before = b.units()[0].hp;
    ASSERT_TRUE(b.apply(act(ActionKind::Attack, 2, 0)).ok);
    EXPECT_EQ(before - b.units()[0].hp, static_cast<int>(std::lround(full * 70 / 100.0))) << "减三成";
}

// 返修第 2 条：她这一回合的行动被合招用掉（陪着出了合招）也算轮到过，剑幕在合招那一刻散（与守势同一个口径）。
TEST(TongdaoBattleMagic, BeingCalledIntoADuoCountsAsHerTurnForTheBulwark) {
    BattleState b;
    Unit chen = companion("chen", 100, 40);
    chen.duo = "duo";
    chen.magics = {"wall"};
    b.setup({hero(50), chen, foe("brute", 10, 0)}, 1);   // 次序：韩立、她、恶狼
    b.addMagic(support("wall", MagicEffect::Bulwark, 50));
    b.addMagic(magic("duo", 10));
    turnOf(b, 1);
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 1, -1, "wall")).ok);
    b.endTurn();
    turnOf(b, 0);   // 第 2 回合，韩立先手
    ASSERT_EQ(b.round(), 2);
    EXPECT_EQ(b.units()[0].status.bulwarkPercent, 50);
    Action duo = act(ActionKind::Duo, 0, 2, "duo");
    duo.partnerIndex = 1;
    std::string why;
    ASSERT_TRUE(b.isLegal(duo, &why)) << why;
    const std::size_t from = b.events().size();
    ASSERT_TRUE(b.apply(duo).ok);
    EXPECT_EQ(count(since(b, from), BattleEventKind::BulwarkEnd), 2);
    EXPECT_EQ(b.units()[0].status.bulwarkPercent, 0);
}

TEST(TongdaoBattleMagic, ABoundFoeLosesItsNextTurnButBossesAndUnescapableFightsResist) {
    BattleState b;
    Unit h = hero(50);
    h.magics = {"bind"};
    Unit boss = foe("boss", 5, 3);
    boss.actions = 2;
    b.setup({h, foe("grunt", 10, 0), boss}, 1);
    b.addMagic(support("bind", MagicEffect::Bind));
    turnOf(b, 0);
    std::string why;
    EXPECT_FALSE(b.isLegal(act(ActionKind::Cast, 0, 2, "bind"), &why)) << "首领定不住";
    EXPECT_FALSE(b.isLegal(act(ActionKind::Cast, 0, 0, "bind"))) << "不能定自己人";
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 0, 1, "bind")).ok);
    EXPECT_EQ(b.units()[1].status.bindSkips, 1);
    const std::size_t from = b.events().size();
    const int hp = b.units()[0].hp;
    b.endTurn();
    // 轮到 grunt 时它被跳过：下一位是首领。
    EXPECT_NE(b.currentActor(), 1);
    const auto events = since(b, from);
    ASSERT_EQ(count(events, BattleEventKind::BoundSkip), 1);
    EXPECT_EQ(b.units()[1].status.bindSkips, 0) << "定身只管一次";
    EXPECT_EQ(b.units()[0].hp, hp);

    BattleState hard;
    hard.setup({h, foe("grunt", 10, 0)}, 1);
    hard.setCanEscape(false);
    hard.addMagic(support("bind", MagicEffect::Bind));
    turnOf(hard, 0);
    EXPECT_TRUE(hard.bindImmune(1)) << "不能逃的仗（精英、剧情硬仗）里定不住";
    EXPECT_FALSE(hard.isLegal(act(ActionKind::Cast, 0, 1, "bind")));
}

TEST(TongdaoBattleMagic, SupportSpellsCostManaButIgnoreBoost) {
    BattleState b;
    Unit h = hero();
    h.magics = {"ward"};
    b.setup({h, foe()}, 1);
    Magic ward = support("ward", MagicEffect::Ward);
    ward.needMp = 7;
    b.addMagic(ward);
    turnOf(b, 0);
    Action a = act(ActionKind::Cast, 0, 0, "ward");
    a.boost = 1;
    const int bp = b.units()[0].bp;
    ASSERT_TRUE(b.apply(a).ok);
    EXPECT_EQ(b.units()[0].mp, 193);
    EXPECT_EQ(b.units()[0].bp, bp) << "蓄了也只当没蓄";
}

// ---------------------------------------------------------------------------
// 被动与在册加成落在战斗里的样子
// ---------------------------------------------------------------------------

TEST(TongdaoBattlePerks, VictoryHealsStandingAlliesOnAWinOnly) {
    SetupOptions o;
    o.victoryHealPercent = 10;
    BattleState b;
    Unit h = hero();
    h.hp = 100;
    Unit weak = foe("paper", 1, 0);
    weak.hp = weak.maxHp = 1;
    b.setup({h, weak}, 1, o);
    turnOf(b, 0);
    ASSERT_TRUE(b.apply(act(ActionKind::Attack, 0, 1)).ok);
    ASSERT_EQ(b.phase(), BattlePhase::Won);
    EXPECT_EQ(b.units()[0].hp, 120) << "maxHp 200 的一成";

    BattleState none;
    none.setup({h, weak}, 1);
    turnOf(none, 0);
    ASSERT_TRUE(none.apply(act(ActionKind::Attack, 0, 1)).ok);
    EXPECT_EQ(none.units()[0].hp, 100) << "缺省不回";
}

TEST(TongdaoBattlePerks, TalismanPowerRaisesOnlyItemCasts) {
    Item charm;
    charm.id = "charm";
    charm.name = "符";
    charm.castMagic = "bolt";
    const Magic bolt = magic("bolt", 40, fanren::core::kElementFire);
    const auto damageWith = [&](int percent, ActionKind kind) {
        SetupOptions o;
        o.talismanPowerPercent = percent;
        BattleState b;
        Unit h = hero();
        h.magics = {"bolt"};
        b.setup({h, foe()}, 1, o);
        b.addMagic(bolt);
        b.addItem(charm);
        turnOf(b, 0);
        Action a = act(kind, 0, 1, kind == ActionKind::Item ? "charm" : "bolt");
        const int estimate = b.estimateHitDamage(a, 1);
        const int before = b.units()[1].hp;
        EXPECT_TRUE(b.apply(a).ok);
        EXPECT_EQ(before - b.units()[1].hp, estimate) << "预估与真打同一个数";
        return before - b.units()[1].hp;
    };
    const int plain = damageWith(0, ActionKind::Item);
    EXPECT_GT(damageWith(50, ActionKind::Item), plain);
    EXPECT_EQ(damageWith(50, ActionKind::Cast), damageWith(0, ActionKind::Cast)) << "自己施法不吃符的加成";
}

TEST(TongdaoBattlePerks, ExtraStaggerOnlyForTheMatchingElementOnAWeakness) {
    const auto toughnessAfter = [](int element, bool bonus) {
        BattleState b;
        Unit h = hero();
        h.magics = {"spell"};
        if (bonus) h.extraStagger.push_back(Unit::ExtraStagger{fanren::core::kCategoryFire, 1});
        b.setup({h, foe("target", 1, 5, fanren::core::kCategoryFire | fanren::core::kCategoryWater)}, 1);
        b.addMagic(magic("spell", 1, element));
        turnOf(b, 0);
        EXPECT_TRUE(b.apply(act(ActionKind::Cast, 0, 1, "spell")).ok);
        return b.units()[1].toughness;
    };
    EXPECT_EQ(toughnessAfter(fanren::core::kElementFire, false), 4);
    EXPECT_EQ(toughnessAfter(fanren::core::kElementFire, true), 3) << "火属打中破绽：多削 1 点";
    EXPECT_EQ(toughnessAfter(fanren::core::kElementWater, true), 4) << "别的属不加";
}

TEST(TongdaoBattlePerks, ChasePowerRaisesHerChaseDamage) {
    const auto chaseDamage = [](int percent) {
        BattleState b;
        Unit mate = companion("mate", 30, 40);   // 二档：会追击
        mate.chasePowerPercent = percent;
        b.setup({hero(50), mate, foe("target", 1, 1, fanren::core::kCategorySword)}, 1);
        turnOf(b, 0);
        const std::size_t from = b.events().size();
        Action hit = act(ActionKind::Attack, 0, 2);
        hit.category = fanren::core::kCategorySword;
        EXPECT_TRUE(b.apply(hit).ok);
        const auto events = since(b, from);
        for (std::size_t i = 0; i + 1 < events.size(); ++i) {
            if (events[i].kind == BattleEventKind::Chase && events[i + 1].kind == BattleEventKind::Hit) {
                return events[i + 1].value;
            }
        }
        ADD_FAILURE() << "没有追击";
        return 0;
    };
    const int plain = chaseDamage(0);
    const int raised = chaseDamage(50);
    EXPECT_GT(raised, plain);
}

TEST(TongdaoBattleSpar, NoChaseAndNoDuoInASpar) {
    SetupOptions o;
    o.spar = true;
    BattleState b;
    Unit mate = companion("mate", 100, 40);
    mate.duo = "duo";
    b.setup({hero(50), mate, foe("target", 1, 1, fanren::core::kCategorySword)}, 1, o);
    b.addMagic(magic("duo", 10));
    EXPECT_TRUE(b.spar());
    turnOf(b, 0);
    Action duo = act(ActionKind::Duo, 0, 2, "duo");
    duo.partnerIndex = 1;
    std::string why;
    EXPECT_FALSE(b.isLegal(duo, &why));
    const std::size_t from = b.events().size();
    Action hit = act(ActionKind::Attack, 0, 2);
    hit.category = fanren::core::kCategorySword;
    ASSERT_TRUE(b.apply(hit).ok);
    const auto events = since(b, from);
    EXPECT_EQ(count(events, BattleEventKind::Break), 1);
    EXPECT_EQ(count(events, BattleEventKind::Chase), 0) << "切磋不追击";
}

TEST(TongdaoBattleMagic, AnUnusedWardFadesAtTheEndOfTheNextRound) {
    BattleState b;
    Unit h = hero(50);
    h.magics = {"ward"};
    b.setup({h, foe("idle", 1, 0)}, 1);
    b.addMagic(support("ward", MagicEffect::Ward));
    turnOf(b, 0);
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 0, 0, "ward")).ok);
    const int castRound = b.round();
    EXPECT_EQ(b.units()[0].status.wardUntilRound, castRound + 1);
    // 敌方这几手空过（不打他）：只看护持自己会不会散。
    const auto skipTo = [&b](int round) {
        while (b.round() < round || b.currentActor() != 0) b.endTurn();
    };
    skipTo(castRound + 1);   // 下一回合：还在
    EXPECT_TRUE(b.units()[0].warded(b.round()));
    skipTo(castRound + 2);   // 再下一回合：散了
    EXPECT_FALSE(b.units()[0].warded(b.round()));
    EXPECT_TRUE(b.isLegal(act(ActionKind::Cast, 0, 0, "ward"))) << "散了就能再挂";
}

TEST(TongdaoBattleMagic, AFoeThatWasBoundShrugsOffBindForTheNextTwoRounds) {
    BattleState b;
    Unit h = hero(50);
    h.magics = {"bind"};
    b.setup({h, foe("grunt", 10, 0)}, 1);
    b.addMagic(support("bind", MagicEffect::Bind));
    turnOf(b, 0);
    ASSERT_TRUE(b.apply(act(ActionKind::Cast, 0, 1, "bind")).ok);
    const int boundRound = b.round();
    b.endTurn();   // grunt 被跳过
    EXPECT_EQ(b.units()[1].status.bindImmuneUntilRound, boundRound + 2);
    std::string why;
    turnOf(b, 0);
    EXPECT_FALSE(b.isLegal(act(ActionKind::Cast, 0, 1, "bind"), &why)) << "随后一回合";
    EXPECT_NE(why.find("刚挣脱"), std::string::npos) << why;
    b.endTurn();
    turnOf(b, 0);
    EXPECT_FALSE(b.isLegal(act(ActionKind::Cast, 0, 1, "bind"))) << "随后第二回合";
    b.endTurn();
    turnOf(b, 0);
    ASSERT_EQ(b.round(), boundRound + 3);
    EXPECT_TRUE(b.isLegal(act(ActionKind::Cast, 0, 1, "bind"))) << "两回合过去又能定";
}

TEST(TongdaoBattleAi, AnAllyWithAHealSpellSavesTheMostWoundedAllyAndOtherwiseFights) {
    BattleState b;
    Unit h = hero(10);
    h.hp = 60;   // 三成
    Unit healer = companion("healer", 0, 50);
    healer.magics = {"heal"};
    b.setup({h, healer, foe()}, 1);
    b.addMagic(support("heal", MagicEffect::Heal, 50));
    turnOf(b, 1);
    const Action a = b.decideAi(1);
    EXPECT_EQ(a.kind, ActionKind::Cast);
    EXPECT_EQ(a.magicId, "heal");
    EXPECT_EQ(a.targetIndex, 0);

    BattleState fine;
    Unit healthy = hero(10);
    fine.setup({healthy, healer, foe()}, 1);
    fine.addMagic(support("heal", MagicEffect::Heal, 50));
    turnOf(fine, 1);
    EXPECT_NE(fine.decideAi(1).kind, ActionKind::Cast) << "都不危急：照旧出手";
}

}  // namespace
