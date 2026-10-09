#include <gtest/gtest.h>
#include "Ch10Driver.h"

namespace {
using namespace fanren;
using namespace fanren::test::ch10;

void writeUnitJson(const fs::path& path,const Json& value) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path,std::ios::binary);
    ASSERT_TRUE(output.good()) << path;
    output << value.dump(2) << '\n';
}

void prepareHandUnit(const test::TempDir& temporary,bool item=false,bool single=false) {
    const fs::path base=temporary.path();
    writeUnitJson(base/"data/magics/all.json",{{"id","hand_unit_all"},{"name","Unit spell"},
        {"element",8},{"needMp",7},{"needRealm","QiRefining1"},{"power",200},
        {"boost","power"},{"target",single ? "single" : "all"}});
    writeUnitJson(base/"data/items/scroll.json",{{"id","hand_unit_scroll"},{"name","Unit scroll"},
        {"kind","artifact"},{"castMagic","hand_unit_all"}});
    for (const char* id : {"caster","high_first","kill_second"}) {
        Json role{{"id",id},{"name",id},{"realm","QiRefining3"},{"maxHp",5000},
            {"maxMp",100},{"attack",1},{"defence",0},{"speed",1},
            {"toughness",1},{"weaknesses",Json::array({"\u706b"})}};
        if (std::string(id)=="caster") {
            role["maxHp"]=500;
            role["speed"]=100;
            role["magics"]=Json::array({"hand_unit_all"});
        }
        if (std::string(id)=="kill_second") role["maxHp"]=80;
        writeUnitJson(base/"data/roles"/(std::string(id)+".json"),role);
    }
    Json units=Json::array();
    if (!item) units.push_back({{"role_id","caster"},{"faction","ally"}});
    units.push_back({{"role_id","high_first"},{"faction","enemy"}});
    units.push_back({{"role_id","kill_second"},{"faction","enemy"}});
    writeUnitJson(base/"data/battles/unit.json",{{"id","hand_unit"},{"hero_absent",!item},
        {"can_escape",false},{"units",units}});
    fs::create_directories(base/"scripts/common");
    std::ofstream api(base/"scripts/common/api.lua",std::ios::binary);
    ASSERT_TRUE(api.good());
    api << "-- Independent menu fixture; no story API or production files are loaded.\n";
}

void configureHandUnit(Application& app,bool item=false) {
    app.state().setFlag(game::kXiuxianKnownFlag);
    app.state().realm=app.state().realmCap=rules::Realm::QiRefining3;
    app.state().hp=app.state().maxHp=500;
    app.state().mp=app.state().maxMp=100;
    app.state().learnWeaknesses("high_first",core::kCategoryFire);
    app.state().learnWeaknesses("kill_second",core::kCategoryFire);
    if (item) app.state().addItem("hand_unit_scroll",2);
}

void expectActionEvidence(const BattleScene& scene,const std::vector<core::battle::Unit>& before,
                          const test::HandActionUtility& utility,const core::battle::Action& action,int mpCost) {
    int damage=0,kills=0,breaks=0;
    std::set<int> targets;
    for (std::size_t i=0;i<before.size();++i) {
        if (!before[i].alive() || before[i].ally==before.at(static_cast<std::size_t>(action.actorIndex)).ally) continue;
        const auto& after=scene.battle().units()[i];
        const int hurt=before[i].hp-after.hp;
        if (hurt>0) targets.insert(static_cast<int>(i));
        damage+=hurt;
        if (!after.alive()) ++kills;
        if (!before[i].broken() && after.alive() && after.broken()) ++breaks;
    }
    EXPECT_EQ(damage,utility.damage);
    EXPECT_EQ(kills,utility.kills);
    EXPECT_EQ(breaks,utility.breaks);
    int hitDamage=0,breakEvents=0,boostEvents=0;
    std::set<int> hitTargets;
    std::vector<int> previousHp;
    for (const auto& unit : before) previousHp.push_back(unit.hp);
    for (const auto& event : scene.battle().lastActionEvents()) {
        if (event.actor!=action.actorIndex) continue;
        if (event.kind==core::battle::BattleEventKind::Hit) {
            const auto index=static_cast<std::size_t>(event.target);
            ASSERT_LT(index,previousHp.size());
            const int actual=previousHp[index]-event.hp;
            EXPECT_GE(event.value,actual); // Hit.value is nominal; HP loss is capped by remaining life.
            hitDamage+=actual;
            previousHp[index]=event.hp;
            hitTargets.insert(event.target);
        }
        if (event.kind==core::battle::BattleEventKind::Break) ++breakEvents;
        if (event.kind==core::battle::BattleEventKind::BoostSpent) { ++boostEvents; EXPECT_EQ(event.value,action.boost); }
    }
    EXPECT_EQ(hitDamage,damage);
    EXPECT_EQ(hitTargets,targets);
    EXPECT_EQ(breakEvents,breaks);
    EXPECT_EQ(boostEvents,action.boost>0 ? 1 : 0);
    const auto actor=static_cast<std::size_t>(action.actorIndex);
    EXPECT_EQ(scene.battle().units()[actor].mp,before[actor].mp-mpCost);
    EXPECT_EQ(scene.battle().units()[actor].bp,before[actor].bp-action.boost);
}

TEST(Ch10HandUnit, AValidNonFirstAllAnchorCommitsThroughTheRealGroupMenu) {
    const test::TempDir temporary("fanren_ch10_hand_cast_unit");
    prepareHandUnit(temporary);
    Application app;
    ASSERT_TRUE(app.init(temporary.path().string(),true).ok);
    configureHandUnit(app);
    BattleScene scene("hand_unit");
    scene.onEnter(app);
    ASSERT_EQ(scene.runToAllyTurn(),0);
    test::BattleHand hand(app,intentPolicy());
    auto action=hand.decide(scene.battle(),0);
    ASSERT_EQ(action.kind,core::battle::ActionKind::Cast);
    action.targetIndex=2;
    action.boost=1;
    std::vector<int> indices;
    const auto menu=BattleScene::buildTargetItems(app.state(),scene.battle(),action,indices);
    EXPECT_EQ(indices,std::vector<int>{1});
    ASSERT_EQ(menu.size(),2u);
    EXPECT_EQ(menu[0].label,"\u654c\u65b9\u5168\u4f53");
    const auto before=scene.battle().units();
    const auto utility=hand.actionUtility(scene.battle(),action);
    ASSERT_TRUE(hand.issue(scene,action));
    expectActionEvidence(scene,before,utility,action,7);
    EXPECT_EQ(scene.battle().phase(),BattlePhase::Ongoing);
    app.shutdown();
}

TEST(Ch10HandUnit, TheChosenAllCandidateCanActuallyBeIssued) {
    const test::TempDir temporary("fanren_ch10_hand_decide_unit");
    prepareHandUnit(temporary);
    Application app;
    ASSERT_TRUE(app.init(temporary.path().string(),true).ok);
    configureHandUnit(app);
    BattleScene scene("hand_unit");
    scene.onEnter(app);
    ASSERT_EQ(scene.runToAllyTurn(),0);
    test::BattleHand hand(app,intentPolicy());
    const auto action=hand.decide(scene.battle(),0);
    ASSERT_EQ(action.kind,core::battle::ActionKind::Cast);
    const auto utility=hand.actionUtility(scene.battle(),action);
    EXPECT_EQ(utility.targets,2);
    const auto before=scene.battle().units();
    ASSERT_TRUE(hand.issue(scene,action));
    expectActionEvidence(scene,before,utility,action,7);
    app.shutdown();
}

TEST(Ch10HandUnit, DirectGroupMenuIsAPositiveControlForTheSameLegalAction) {
    const test::TempDir temporary("fanren_ch10_hand_group_control");
    prepareHandUnit(temporary);
    Application app;
    ASSERT_TRUE(app.init(temporary.path().string(),true).ok);
    configureHandUnit(app);
    BattleScene scene("hand_unit");
    scene.onEnter(app);
    ASSERT_EQ(scene.runToAllyTurn(),0);
    core::battle::Action action;
    action.kind=core::battle::ActionKind::Cast;
    action.actorIndex=0;
    action.targetIndex=2;
    action.magicId="hand_unit_all";
    action.boost=1;
    test::BattleHand hand(app,intentPolicy());
    const auto utility=hand.actionUtility(scene.battle(),action);
    const auto before=scene.battle().units();
    scene.openMenu(app);
    scene.setBoost(1);
    ASSERT_TRUE(scene.menuChoose(app,game::kBattleMenuCast));
    ASSERT_TRUE(scene.menuChoose(app,0));
    ASSERT_EQ(scene.menuMode(),game::BattleMenuMode::Target);
    ASSERT_TRUE(scene.menuChoose(app,0));
    expectActionEvidence(scene,before,utility,action,7);
    app.shutdown();
}

TEST(Ch10HandUnit, SingleStillIssuesToTheExactSecondOpponent) {
    const test::TempDir temporary("fanren_ch10_hand_single_control");
    prepareHandUnit(temporary,false,true);
    Application app;
    ASSERT_TRUE(app.init(temporary.path().string(),true).ok);
    configureHandUnit(app);
    BattleScene scene("hand_unit");
    scene.onEnter(app);
    ASSERT_EQ(scene.runToAllyTurn(),0);
    core::battle::Action action;
    action.kind=core::battle::ActionKind::Cast;
    action.actorIndex=0;
    action.targetIndex=2;
    action.magicId="hand_unit_all";
    action.boost=1;
    test::BattleHand hand(app,intentPolicy());
    const auto utility=hand.actionUtility(scene.battle(),action);
    const auto before=scene.battle().units();
    ASSERT_TRUE(hand.issue(scene,action));
    EXPECT_EQ(scene.battle().units()[1].hp,before[1].hp);
    EXPECT_EQ(utility.targets,1);
    expectActionEvidence(scene,before,utility,action,7);
    app.shutdown();
}

TEST(Ch10HandUnit, ASpellItemAlsoUsesTheGroupAndPaysOnlyOneItem) {
    const test::TempDir temporary("fanren_ch10_hand_item_unit");
    prepareHandUnit(temporary,true);
    Application app;
    ASSERT_TRUE(app.init(temporary.path().string(),true).ok);
    configureHandUnit(app,true);
    BattleScene scene("hand_unit");
    scene.onEnter(app);
    ASSERT_EQ(scene.runToAllyTurn(),0);
    core::battle::Action action;
    action.kind=core::battle::ActionKind::Item;
    action.actorIndex=0;
    action.targetIndex=2;
    action.magicId="hand_unit_scroll";
    test::BattleHand hand(app,intentPolicy());
    const auto before=scene.battle().units();
    const auto utility=hand.actionUtility(scene.battle(),action);
    ASSERT_TRUE(hand.issue(scene,action));
    EXPECT_EQ(app.state().itemCount("hand_unit_scroll"),1);
    EXPECT_EQ(scene.battle().units()[0].mp,before[0].mp);
    EXPECT_EQ(scene.battle().units()[0].bp,before[0].bp);
    EXPECT_LT(scene.battle().units()[1].hp,before[1].hp);
    EXPECT_EQ(scene.battle().units()[2].hp,0);
    EXPECT_EQ(utility.targets,2);
    expectActionEvidence(scene,before,utility,action,0);
    app.shutdown();
}

std::vector<std::string> errors(const std::array<Json,4>& battles,const std::map<std::string,Json>& roles) {
    std::vector<std::string> out;
    const auto require=[&](bool condition,const std::string& message) { if (!condition) out.push_back(message); };
    for (std::size_t i=0;i<battles.size();++i) {
        const auto& b=battles[i];
        const std::string id=kBattles[i];
        require(b.at("id")==id,id+" id");
        require(b.at("chapter")==10,id+" chapter");
        require(!b.at("can_escape").get<bool>(),id+" can_escape");
        require(b.at("defeat_is_fatal").get<bool>()==(i!=0),id+" defeat_is_fatal");
        require(b.value("hero_absent",false)==(i==1 || i==3),id+" hero_absent");
        require(b.at("rewards").at("spirit_stones")==0,id+" spirit_stones");
        require(b.at("rewards").at("drops").empty(),id+" drops");
        require(i==0 ? b.at("rewards").at("cultivation").get<int>()>0
                     : b.at("rewards").at("cultivation")==0,id+" cultivation");
        std::vector<std::string> allies;
        std::multiset<std::pair<std::string,int>> foes;
        for (const auto& unit : b.at("units")) {
            const std::string role=unit.at("role_id");
            if (unit.at("faction")=="ally") allies.push_back(role);
            else foes.emplace(role,unit.value("wave",0));
        }
        if (i==0) {
            require(allies.empty(),id+" unexpected ally");
            require(foes==std::multiset<std::pair<std::string,int>>{{"daoyu_husui",0}},id+" foe");
        } else if (i==1) {
            require(allies==std::vector<std::string>{"qu_hun_huashen","feng_sanniang","qing_suanzi","yan_daoyou"},id+" four allies");
            require(foes==std::multiset<std::pair<std::string,int>>{{"yingli_shou",0}},id+" foe");
        } else if (i==2) {
            require(foes==std::multiset<std::pair<std::string,int>>{{"gu_zhanglao",0}},id+" foe");
        } else {
            require(allies==std::vector<std::string>{"qu_hun_shadan"},id+" sole avatar");
            const std::multiset<std::pair<std::string,int>> expected{
                {"zhifa_xiushi",0},{"zhifa_xiushi",0},{"zhifa_xiushi",0},{"zhifa_xiushi",0},
                {"zhifa_laozhe",1},{"zhifa_xiushi",1},{"zhifa_xiushi",1}};
            require(foes==expected,id+" two waves of seven");
        }
    }
    const auto weak=[&](const char* role) { return roles.at(role).at("weaknesses").get<std::set<std::string>>(); };
    require(weak("daoyu_husui")==std::set<std::string>{"\u706b","\u6697\u5668"},"arena exact weaknesses");
    require(weak("yingli_shou").count("\u6728") && weak("yingli_shou").count("\u5251"),"beast wood/sword");
    require(weak("gu_zhanglao").count("\u91d1") && weak("gu_zhanglao").count("\u706b"),"elder metal/fire");
    for (const auto& spec : {std::pair{"daoyu_husui",5},std::pair{"yingli_shou",8},std::pair{"gu_zhanglao",7},
                            std::pair{"zhifa_xiushi",2},std::pair{"zhifa_laozhe",4}})
        require(roles.at(spec.first).at("toughness")==spec.second,std::string(spec.first)+" toughness");
    require(roles.at("yingli_shou").at("actions")==2,"beast actions");
    require(roles.at("gu_zhanglao").at("actions")==1,"elder actions");
    require(roles.at("yingli_shou").at("charge").at("all")==true,"beast full-field charge");
    require(roles.at("gu_zhanglao").at("charge").at("all")==true,"elder full-field charge");
    return out;
}

std::array<Json,4> battleData() {
    std::array<Json,4> data;
    for (std::size_t i=0;i<data.size();++i) data[i]=json(root()/"data"/"battles"/(std::string(kBattles[i])+".json"));
    return data;
}
std::map<std::string,Json> roleData() {
    std::map<std::string,Json> data;
    for (const char* id : {"daoyu_husui","yingli_shou","gu_zhanglao","zhifa_xiushi","zhifa_laozhe"})
        data[id]=json(root()/"data"/"roles"/(std::string(id)+".json"));
    return data;
}

TEST(Ch10BattleData, TheFourMandatorySetupsMatchDesignEightPointTwo) {
    EXPECT_TRUE(errors(battleData(),roleData()).empty()) << test::joinLines(errors(battleData(),roleData()));
}

TEST(Ch10BattleData, InMemoryMutationsCatchHeroRewardsWavesAndWeaknesses) {
    const auto original=battleData();
    const auto roles=roleData();
    ASSERT_TRUE(errors(original,roles).empty()) << test::joinLines(errors(original,roles));
    for (int variant=0;variant<6;++variant) {
        auto bad=original;
        auto badRoles=roles;
        switch(variant) {
        case 0: bad[1]["hero_absent"]=false; break;
        case 1: bad[3]["units"][1]["wave"]=1; break;
        case 2: bad[2]["rewards"]["spirit_stones"]=1; break;
        case 3: bad[0]["defeat_is_fatal"]=true; break;
        case 4: badRoles["daoyu_husui"]["weaknesses"].push_back("\u6c34"); break;
        case 5: badRoles["gu_zhanglao"]["actions"]=2; break;
        }
        EXPECT_FALSE(errors(bad,badRoles).empty()) << "mutation=" << variant;
    }
}

TEST(Ch10BattleData, OrdinaryFullFieldMagicUsesTheFrozenE3Contract) {
    for (const char* file : {"xuelian_ziyan","ch10_wulang"}) {
        const auto magic=json(root()/"data"/"magics"/(std::string(file)+".json"));
        EXPECT_EQ(magic.at("target"),"all");
        EXPECT_GT(magic.value("power",0)+magic.value("poison",0),0);
        EXPECT_NE(magic.value("effect",""),"reveal");
        EXPECT_NE(magic.value("effect",""),"stagger");
    }
    for (const auto& spec : {std::pair{"qu_hun_huashen","FoundationLate"},std::pair{"qu_hun_shadan","CoreEarly"}}) {
        const auto role=json(root()/"data"/"roles"/(std::string(spec.first)+".json"));
        EXPECT_EQ(role.at("realm"),spec.second);
        EXPECT_NE(std::find(role.at("weapons").begin(),role.at("weapons").end(),"\u5251"),role.at("weapons").end());
        EXPECT_NE(std::find(role.at("magics").begin(),role.at("magics").end(),"magic_xuelian_guangzhu"),role.at("magics").end());
    }
}

void replayFight(Driver& driver,const Fight& reached,bool withholdNeedles,bool delayFirstAction,int maxRounds) {
    const test::TempDir temporary("fanren_ch10_reachable_battle");
    const auto file=temporary.path()/"reached.sav";
    ASSERT_TRUE(io::saveGame(reached.before,file.string()).ok);
    Application isolated;
    ASSERT_TRUE(isolated.init(root().string(),true).ok);
    const auto loaded=io::loadGame(file.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    isolated.state()=loaded.value;
    ASSERT_TRUE(isolated.loadMap(reached.before.mapId,"").ok);
    isolated.state().position=reached.before.position;
    isolated.state().facing=reached.before.facing;
    EXPECT_EQ(test::comparableSaveLines(isolated.state()),test::comparableSaveLines(reached.before));
    if (withholdNeedles) {
        ASSERT_TRUE(isolated.state().knowsMagic("magic_ji_hongxianzhen"));
        ASSERT_TRUE(isolated.state().forgetMagic("magic_ji_hongxianzhen"));
    }
    BattleScene scene(reached.id);
    scene.onEnter(isolated);
    EXPECT_EQ(game::battleBackdropFor(isolated,reached.id),reached.id=="b10_gu_zhanglao" ? "cliff_top" : "dock_river");
    if (delayFirstAction) {
        const int actor=scene.runToAllyTurn();
        ASSERT_GE(actor,0);
        core::battle::Action action;
        action.kind=core::battle::ActionKind::Defend;
        action.actorIndex=actor;
        ASSERT_TRUE(scene.issuePlayerAction(isolated,action).ok);
    }
    test::BattleHand hand(isolated,intentPolicy());
    EXPECT_EQ(hand.play(scene),BattlePhase::Won) << reached.id << " noNeedles=" << withholdNeedles
        << " delayed=" << delayFirstAction << " realCheckpoint=" << reached.before.day << '\n' << test::joinLines(scene.battle().log());
    if (maxRounds>0) EXPECT_LE(scene.battle().round(),maxRounds) << test::joinLines(scene.battle().log());
    EXPECT_EQ(driver.start().formerRealm,rules::Realm::FoundationMid);
    isolated.shutdown();
}

TEST(Ch10BattleBalance, TheActualElderCheckpointWinsWithNoLearnedNeedles) {
    for (const char* file : {"ch09-end-first.sav","ch09-end-second.sav"}) {
        Driver driver;
        ASSERT_TRUE(driver.load(file)) << driver.problem();
        ASSERT_TRUE(driver.runTo(29)) << driver.problem();
        ASSERT_EQ(driver.fights().size(),3u);
        replayFight(driver,driver.fights()[2],true,false,0);
    }
}

TEST(Ch10BattleBalance, TheSoloAvatarWinsWithinFourRoundsAndSurvivesOneRealGuardDelay) {
    for (const char* file : {"ch09-end-first.sav","ch09-end-second.sav"}) {
        Driver driver;
        ASSERT_TRUE(driver.load(file)) << driver.problem();
        ASSERT_TRUE(driver.runTo(32)) << driver.problem();
        ASSERT_EQ(driver.fights().size(),4u);
        EXPECT_LE(driver.fights()[3].rounds,4) << test::joinLines(driver.fights()[3].log);
        replayFight(driver,driver.fights()[3],false,true,4);
    }
}

TEST(Ch10BattleBalance, TheActualPurpleActionAndUtilityAgreeOnAllLiveEnemies) {
    Driver driver;
    ASSERT_TRUE(driver.load()) << driver.problem();
    ASSERT_TRUE(driver.runTo(32)) << driver.problem();
    ASSERT_EQ(driver.fights().size(),4u);
    const Fight& reached=driver.fights()[3];
    Application isolated;
    ASSERT_TRUE(isolated.init(root().string(),true).ok);
    isolated.state()=reached.before;
    BattleScene scene(reached.id);
    scene.onEnter(isolated);
    const int actor=scene.runToAllyTurn();
    ASSERT_GE(actor,0);
    test::BattleHand hand(isolated,intentPolicy());
    const auto selected=hand.decide(scene.battle(),actor);
    core::battle::Action purple;
    purple.kind=core::battle::ActionKind::Cast;
    purple.actorIndex=actor;
    purple.magicId="magic_xuelian_ziyan";
    purple.boost=std::min(1,scene.battle().units().at(static_cast<std::size_t>(actor)).bp);
    for (std::size_t i=0;i<scene.battle().units().size();++i) {
        const auto& unit=scene.battle().units()[i];
        if (!unit.ally && unit.alive()) { purple.targetIndex=static_cast<int>(i); break; }
    }
    ASSERT_GE(purple.targetIndex,0);
    const auto oldChoiceUtility=hand.actionUtility(scene.battle(),selected);
    const auto purpleUtility=hand.actionUtility(scene.battle(),purple);
    std::cout << "[ch10 utility] selectedKind=" << static_cast<int>(selected.kind)
              << " selectedMagic=" << selected.magicId << " targets=" << oldChoiceUtility.targets
              << " damage=" << oldChoiceUtility.damage << " kills=" << oldChoiceUtility.kills
              << " breaks=" << oldChoiceUtility.breaks << " purpleTargets=" << purpleUtility.targets
              << " purpleDamage=" << purpleUtility.damage << " purpleKills=" << purpleUtility.kills
              << " purpleBreaks=" << purpleUtility.breaks << '\n';
    EXPECT_EQ(purpleUtility.targets,4);
    EXPECT_GE(purpleUtility.damage,oldChoiceUtility.damage);
    EXPECT_GT(purpleUtility.damage,std::min(scene.battle().units().at(static_cast<std::size_t>(purple.targetIndex)).hp,
        scene.battle().estimateHitDamage(purple,purple.targetIndex)));
    const auto before=scene.battle().units();
    ASSERT_TRUE(hand.issue(scene,purple));
    int actuallyHit=0;
    for (std::size_t i=0;i<before.size();++i) {
        if (!before[i].ally && before[i].alive() && scene.battle().units()[i].hp<before[i].hp) ++actuallyHit;
    }
    EXPECT_EQ(actuallyHit,4);
    expectActionEvidence(scene,before,purpleUtility,purple,35);
    for (const auto& line : scene.battle().log()) std::cout << "[ch10 actual purple] " << line << '\n';
    // The red diagnostic must execute the real all-target action before asking the hand to prefer it.
    EXPECT_EQ(selected.kind,core::battle::ActionKind::Cast);
    EXPECT_EQ(selected.magicId,"magic_xuelian_ziyan");
    isolated.shutdown();
}
} // namespace
