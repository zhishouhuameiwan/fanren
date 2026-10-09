#include <gtest/gtest.h>
#include <regex>
#include "Ch10Data.h"

namespace {
using namespace fanren::test::ch10;

std::vector<std::string> hookErrors(const Json& object,const Hook& spec) {
    const Json p=properties(object);
    std::vector<std::string> result;
    const auto check=[&](bool ok,const char* field) { if (!ok) result.push_back(std::string(spec.object)+" "+field); };
    check(p.value("mode","")==std::string(spec.enter ? "enter" : "interact"),"mode");
    check(p.contains("once") && p.at("once").is_boolean() && p.at("once").get<bool>()==spec.once,"once");
    check(p.value("guard_flag","")==spec.guard,"guard_flag");
    check(p.value("set_flag","")==spec.done,"set_flag");
    check(p.value("script","")=="ch10/"+std::string(spec.script)+".lua","script");
    if (!spec.once) check(!p.contains("set_flag"),"repeatable has set_flag");
    return result;
}

TEST(Ch10TriggerMode, EveryHookMatchesTheIndependentDesignTable) {
    const auto expected=hooks();
    ASSERT_EQ(expected.size(),42u);
    std::set<std::string> seen;
    for (const char* mapId : kMaps) {
        const auto map=json(root()/"maps"/(std::string(mapId)+".tmj"));
        const auto actual=objects(map);
        for (const auto& spec : expected) {
            if (std::string(spec.map)!=mapId) continue;
            const auto found=actual.find(spec.object);
            ASSERT_NE(found,actual.end()) << spec.map << '/' << spec.object;
            const auto errors=hookErrors(found->second,spec);
            EXPECT_TRUE(errors.empty()) << (errors.empty() ? "" : errors.front());
            seen.insert(std::string(spec.map)+"/"+spec.object);
        }
        for (const auto& [name,object] : actual) {
            const auto p=properties(object);
            EXPECT_NE(p.value("mode",""),"auto") << mapId << '/' << name;
            if (p.value("script","").rfind("ch10/",0)==0 && name.rfind("trigger_",0)==0)
                EXPECT_EQ(seen.count(std::string(mapId)+"/"+name),1u) << "Undocumented trigger " << name;
        }
    }
    EXPECT_EQ(seen.size(),42u);
}

TEST(Ch10TriggerMode, InMemoryMutationsCatchModeOnceGuardAndProducerDrift) {
    const Hook spec=kStory.front();
    const auto original=objects(json(root()/"maps"/"ch10_gudao.tmj")).at(spec.object);
    ASSERT_TRUE(hookErrors(original,spec).empty());
    for (const auto& [field,value] : {std::pair<std::string,Json>{"mode","enter"},{"once",false},
                                    {"guard_flag","ch10.done"},{"set_flag","ch10.done"},{"script","ch10/jushi.lua"}}) {
        auto bad=original;
        bool changed=false;
        for (auto& property : bad["properties"]) if (property["name"]==field) { property["value"]=value; changed=true; }
        ASSERT_TRUE(changed) << field;
        EXPECT_FALSE(hookErrors(bad,spec).empty()) << field;
    }
}

TEST(Ch10TriggerMode, TheThreePlayerFacilitiesHaveTheirExactUnlocksAndNoSitePenalty) {
    const auto actual=objects(json(root()/"maps"/"ch10_xiaohuan.tmj"));
    for (const auto& [name,flag] : {std::pair{"facility_yaoyuan_xh","ch10.dingce"},
                                   std::pair{"facility_jingshi","ch10.dingce"},std::pair{"facility_danfang_xh","ch10.zhuji"}})
        EXPECT_EQ(properties(actual.at(name)).at("require_flag"),flag);
    EXPECT_FALSE(properties(actual.at("facility_jingshi")).contains("effectiveness"));
    EXPECT_EQ(properties(actual.at("facility_danfang_xh")).at("grade"),3);
    EXPECT_EQ(properties(actual.at("facility_yaoyuan_xh")).at("slots"),4);
    EXPECT_EQ(properties(actual.at("facility_yaoyuan_xh")).at("ref_id"),"field_xiaohuan");
}

TEST(Ch10TriggerMode, OnlyTheApprovedGujiaPortalPairExistsAndNoEncounterCanInventCultivation) {
    std::set<std::pair<std::string,std::string>> portals;
    for (const char* mapId : kMaps) {
        for (const auto& [name,object] : objects(json(root()/"maps"/(std::string(mapId)+".tmj")))) {
            const std::string type=object.value("class",object.value("type",""));
            EXPECT_NE(type,"encounter") << mapId << '/' << name;
            if (type!="portal") continue;
            const auto p=properties(object);
            EXPECT_EQ(p.at("require_flag"),"ch10.chudao");
            portals.emplace(mapId,p.at("target_map").get<std::string>());
        }
    }
    EXPECT_EQ(portals,(std::set<std::pair<std::string,std::string>>{
        {"ch10_tiandujie","ch10_kuixing"},{"ch10_kuixing","ch10_tiandujie"}}));
}

TEST(Ch10TriggerMode, EveryMandatoryBattleIsAnUnskippableObjectiveProducer) {
    const auto objective=json(root()/"data"/"objectives"/"ch10.json").at("steps");
    for (const auto& [node,battle] : {std::pair{10,"b10_gujia_bidou"},std::pair{28,"b10_liuliandian_weisha"},
                                    std::pair{29,"b10_gu_zhanglao"},std::pair{32,"b10_zhifadui"}}) {
        const auto& hook=kStory.at(static_cast<std::size_t>(node-1));
        const auto found=std::find_if(objective.begin(),objective.end(),[&](const auto& step){ return step.at("done_flag")==hook.done; });
        ASSERT_NE(found,objective.end()) << hook.done;
        EXPECT_EQ(found->at("target_object"),hook.object);
        EXPECT_EQ(found->at("target_map"),hook.map);
        if (node<32) EXPECT_EQ(kStory.at(static_cast<std::size_t>(node)).guard,std::string(hook.done));
        int occurrences=0;
        const std::regex call("\\bbattle\\s*\\(\\s*\""+std::string(battle)+"\"");
        for (const auto& file : fs::directory_iterator(root()/"scripts"/"ch10")) {
            if (file.path().extension()!=".lua") continue;
            const auto text=read(file.path());
            const int count=static_cast<int>(std::distance(std::sregex_iterator(text.begin(),text.end(),call),std::sregex_iterator{}));
            if (count) EXPECT_EQ(file.path().stem().string(),hook.script);
            occurrences+=count;
        }
        EXPECT_EQ(occurrences,1) << battle;
    }
}
} // namespace
