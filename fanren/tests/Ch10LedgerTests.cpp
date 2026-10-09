#include <gtest/gtest.h>
#include <regex>
#include <tuple>
#include "Ch10Data.h"
#include "TextKeys.h"

namespace {
using namespace fanren::test::ch10;

using Sources=std::map<std::string,std::string>;
using Flow=std::tuple<std::string,std::string,std::string,std::string>;

TEST(TextKeys, FlatStringPairsRetainEscapesLongLinesAndUnknownKeys) {
    const std::string longLine=std::string(8192,'x')+" quoted \"value\" and newline\n";
    const Json document{{"ch05.long",longLine},{"ch06.kept","value"},{"unknown.future.key","kept"},{"chapter",10}};
    const auto parsed=fanren::test::textKeyValues(document.dump());
    ASSERT_EQ(parsed.size(),3u);
    EXPECT_EQ(parsed.at("ch05.long"),longLine);
    EXPECT_EQ(parsed.at("unknown.future.key"),"kept");
    EXPECT_EQ(parsed.count("missing.required.key"),0u);
    EXPECT_EQ(parsed.count("chapter"),0u);
}

TEST(TextKeys, MalformedJsonAndNonObjectsCannotBecomeAnEmptyGreenMap) {
    EXPECT_THROW(fanren::test::textKeyValues("{\"ch05.key\": \"unterminated}"),nlohmann::json::parse_error);
    EXPECT_THROW(fanren::test::textKeyValues("[\"value\"]"),std::invalid_argument);
}

Sources scripts() {
    Sources result;
    for (const auto& file : fs::directory_iterator(root()/"scripts"/"ch10"))
        if (file.path().extension()==".lua") result[file.path().stem().string()]=read(file.path());
    return result;
}

std::string executableLines(const std::string& source) {
    std::istringstream input(source);
    std::string line,out;
    while (std::getline(input,line)) {
        // These source checks cover the chapter's one-line API calls; runtime ledger is separate.
        if (const auto comment=line.find("--"); comment!=std::string::npos) line.resize(comment);
        out+=line+'\n';
    }
    return out;
}

std::multiset<Flow> flows(const Sources& sources) {
    std::multiset<Flow> result;
    const std::regex call(R"lua(\b(give|take)\s*\(\s*"([^"]+)"\s*(?:,\s*([^,)\r\n]+))?)lua");
    const std::regex clamp(R"lua(local\s+(\w+)\s*=\s*math\.min\(item\.count\("([^"]+)"\),\s*(\d+)\))lua");
    for (const auto& [name,source] : sources) {
        const std::string clean=executableLines(source);
        std::map<std::string,std::string> amounts;
        for (std::sregex_iterator it(clean.begin(),clean.end(),clamp),end;it!=end;++it)
            amounts[(*it)[1].str()]="min:"+(*it)[2].str()+":"+(*it)[3].str();
        for (std::sregex_iterator it(clean.begin(),clean.end(),call),end;it!=end;++it) {
            std::string count=(*it)[3].matched ? (*it)[3].str() : "1";
            count=std::regex_replace(count,std::regex("\\s+"),"");
            if (amounts.count(count)) count=amounts.at(count);
            result.emplace(name,(*it)[1].str(),(*it)[2].str(),count);
        }
    }
    return result;
}

std::multiset<Flow> expectedFlows() {
    return {
        {"gujia","give","material_lingshi","120"}, {"zhifadui","give","material_lingshi","80"},
        {"xuandi","take","material_lingshi","min:material_lingshi:5"}, {"matou","take","material_lingshi","min:material_lingshi:6"},
        {"sanzhuan","take","material_lingshi","min:material_lingshi:120"}, {"danyaopu","take","material_lingshi","min:material_lingshi:15"},
        {"danyaopu","take","material_lingshi","min:material_lingshi:20"},
        {"ruzhen","give","material_lingshi_zhong","75"}, {"zhuji","take","pill_zhuji_dan","min:pill_zhuji_dan:3"},
        {"sanzhuan","take","pill_zhenyuan_dan","2"}, {"ruzhen","give","pill_jiangchen_dan","5"},
        {"baishuilou","give","material_xuelingshui","2"}, {"baishuilou","give","material_tianhuoye","2"},
        {"linshi","take","material_tianhuoye","min:material_tianhuoye:2"}, {"linshi","take","pill_xuening_wuxing_dan","min:pill_xuening_wuxing_dan:5"},
        {"linshi","take","story_jin_kuloutou","1"}
    };
}

bool strictAccountsMatch(const Sources& sources) {
    for (const auto& [name,text] : sources) {
        static_cast<void>(name);
        const auto clean=executableLines(text);
        if (clean.find("\"story_jin_kulou\"")!=std::string::npos ||
            clean.find("\"story_xuening_dan\"")!=std::string::npos) return false;
    }
    const auto expected=expectedFlows();
    std::set<std::string> accounts{"pill_dingyan_dan"};
    for (const auto& flow : expected) accounts.insert(std::get<2>(flow));
    std::multiset<Flow> actual;
    for (const auto& flow : flows(sources)) if (accounts.count(std::get<2>(flow))) actual.insert(flow);
    return actual==expected;
}

std::string accountDifference(const Sources& sources) {
    std::ostringstream out;
    const auto actual=flows(sources);
    for (const auto& [script,operation,item,amount] : expectedFlows())
        if (!actual.count({script,operation,item,amount}))
            out << "missing expected tuple: " << script << ' ' << operation << ' ' << item << ' ' << amount << '\n';
    for (const auto& [script,operation,item,amount] : actual)
        out << "actual source tuple: " << script << ' ' << operation << ' ' << item << ' ' << amount << '\n';
    return out.str();
}

TEST(Ch10Ledger, EveryCurrencyPillAndElixirCallMatchesTheApprovedAccount) {
    const auto source=scripts();
    ASSERT_EQ(source.size(),43u);
    EXPECT_TRUE(strictAccountsMatch(source)) << accountDifference(source);
    const auto all=flows(source);
    for (const auto& entry : all) {
        EXPECT_NE(std::get<2>(entry),"story_jin_kulou");
        EXPECT_NE(std::get<2>(entry),"story_xuening_dan");
    }
    const auto incoming=json(root()/"tests"/"fixtures"/"ch09-end-first.sav").at("payload");
    std::map<std::string,int> counts;
    for (const auto& stack : incoming.at("bag")) counts[stack.at("itemId").get<std::string>()]+=stack.at("count").get<int>();
    EXPECT_EQ(counts["story_jin_kuloutou"],1);
    EXPECT_EQ(counts["pill_xuening_wuxing_dan"],5);
    for (const auto& file : fs::recursive_directory_iterator(root()/"data"/"items")) {
        if (file.path().extension()!=".json") continue;
        const auto item=json(file.path());
        EXPECT_NE(item.at("id"),"story_jin_kulou") << file.path();
        EXPECT_NE(item.at("id"),"story_xuening_dan") << file.path();
    }
}

TEST(Ch10Ledger, InMemoryWrongIdsAmountsAndExtraProductGrantsAreRejected) {
    const auto original=scripts();
    ASSERT_TRUE(strictAccountsMatch(original)) << accountDifference(original);
    for (const std::string added : {"\ngive(\"pill_zhenyuan_dan\", 2)\n",
                                   "\ntake(\"pill_dingyan_dan\", 1)\n",
                                   "\ngive(\"material_lingshi_zhong\", 1)\n"}) {
        auto bad=original;
        bad["linshi"]+=added;
        EXPECT_FALSE(strictAccountsMatch(bad));
    }
    for (const auto& [good,badId] : {std::pair{"story_jin_kuloutou","story_jin_kulou"},
                                   std::pair{"pill_xuening_wuxing_dan","story_xuening_dan"}}) {
        auto bad=original;
        const auto found=bad["linshi"].find(good);
        ASSERT_NE(found,std::string::npos);
        bad["linshi"].replace(found,std::string(good).size(),badId);
        EXPECT_FALSE(strictAccountsMatch(bad));
    }
    auto wrong=original;
    const auto found=wrong["gujia"].find(", 120");
    ASSERT_NE(found,std::string::npos);
    wrong["gujia"].replace(found,5,", 121");
    EXPECT_FALSE(strictAccountsMatch(wrong));
    auto badBound=original;
    const auto bound=badBound["zhuji"].find("), 3)");
    ASSERT_NE(bound,std::string::npos);
    badBound["zhuji"].replace(bound,5,"), 4)");
    EXPECT_FALSE(strictAccountsMatch(badBound));
}

TEST(Ch10Ledger, MoneyChargesClampToTheCurrentPurseWithoutAResourceReturnGate) {
    const auto source=scripts();
    for (const auto& [name,amount] : {std::pair{"xuandi",5},std::pair{"matou",6},std::pair{"sanzhuan",120},std::pair{"danyaopu",15}}) {
        const std::string& text=source.at(name);
        EXPECT_NE(text.find("math.min(item.count(\"material_lingshi\"), "+std::to_string(amount)+")"),std::string::npos) << name;
        const std::regex resourceGate(R"lua(if\s+item\.count\("material_lingshi"\)\s*[<>=]+[^\n]*return)lua");
        EXPECT_FALSE(std::regex_search(executableLines(text),resourceGate)) << name;
    }
    EXPECT_NE(source.at("danyaopu").find("math.min(item.count(\"material_lingshi\"), 20)"),std::string::npos);
}

TEST(Ch10Ledger, RealmPartyAndDestroyedToolsHaveOnlyTheirDesignProducers) {
    const auto source=scripts();
    const std::regex call(R"lua(\b(realm\.(?:advance|cap|demote)|magic\.forget|party\.(?:add|remove)|field\.unlock)\s*\(\s*([^,\r\n)]+))lua");
    std::multiset<std::tuple<std::string,std::string,std::string>> actual;
    for (const auto& [name,text] : source) {
        const std::string clean=executableLines(text);
        for (std::sregex_iterator it(clean.begin(),clean.end(),call),end;it!=end;++it)
            actual.emplace(name,(*it)[1].str(),(*it)[2].str());
    }
    const decltype(actual) expected{
        {"muwu","realm.advance","realm.QI_REFINING_5"},{"zhuji","realm.advance","realm.FOUNDATION_EARLY"},
        {"sanzhuan","realm.advance","realm.FOUNDATION_LATE"},{"lifu","realm.cap","realm.QI_REFINING_9"},
        {"lifu","field.unlock","\"field_xiaohuan\""},
        {"leitai","party.remove","\"kuilei_shou\""},{"leitai","party.add","\"kuilei_shou\""},
        {"sanzhuan","party.add","\"qu_hun_huashen\""},{"ruzhen","party.remove","\"kuilei_shou\""},
        {"linshi","party.remove","\"qu_hun_huashen\""},{"linshi","party.add","\"qu_hun_shadan\""},
        {"ruzhen","magic.forget","\"magic_ji_qingning\""},{"ruzhen","magic.forget","\"magic_ji_wulongduo\""},
        {"ruzhen","magic.forget","\"magic_ji_baizhudao\""}};
    EXPECT_EQ(actual,expected);
    const auto recipe=json(root()/"data"/"recipes"/"alchemy"/"zhenyuan_dan.json");
    EXPECT_EQ(recipe.at("productId"),"pill_zhenyuan_dan");
    EXPECT_EQ(recipe.at("productCount"),2);
    EXPECT_EQ(recipe.at("requireFlag"),"ch09.xingchen");
    EXPECT_EQ(recipe.at("inputs"),(Json::array({
        {{"itemId","herb_zishen_cao"},{"count",1},{"minAge",60}},
        {{"itemId","herb_xuehong_zhi"},{"count",1},{"minAge",60}}})));
    EXPECT_EQ(recipe.at("difficulty"),50);
}
} // namespace
