#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <string>
#include "ChapterFixture.h"

namespace {
using namespace fanren;
std::string source(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    std::string all, line;
    while (std::getline(in, line)) all += line.substr(0, line.find("--")) + '\n';
    return all;
}
std::multiset<std::string> callers(const std::string& expression) {
    std::multiset<std::string> out;
    const std::regex pattern(expression);
    for (const auto& e : std::filesystem::directory_iterator(test::chapterNineAssetRoot() + "/scripts/ch09")) {
        if (e.path().extension() != ".lua") continue;
        const auto code = source(e.path());
        for (auto it = std::sregex_iterator(code.begin(), code.end(), pattern); it != std::sregex_iterator(); ++it)
            out.insert(e.path().stem().string());
    }
    return out;
}
TEST(Ch09Ledger, CurrencyPillsLearningAndDemotionHaveOnlyTheDesignProducers) {
    ASSERT_TRUE(std::regex_search(std::string("give(\"material_lingshi\", 60)"),
                                  std::regex(R"re(\bgive\("material_lingshi")re")));
    EXPECT_EQ(callers(R"re(\bgive\("material_lingshi")re"), (std::multiset<std::string>{"feixu"}));
    EXPECT_EQ(callers(R"re(\btake\("material_lingshi")re"),
              (std::multiset<std::string>{"xingchenge", "goucai", "zhenyan_jiu", "huicheng", "qidong"}));
    // Design 18: zhenyan_jiu may debit an existing low stone only to cover a missing middle stone.
    EXPECT_EQ(callers(R"re(\bgive\("material_lingshi_zhong")re"), (std::multiset<std::string>{"shudong"}));
    EXPECT_EQ(callers(R"re(\btake\("material_lingshi_zhong")re"), (std::multiset<std::string>{"zhenyan_jiu", "qidong"}));
    EXPECT_EQ(callers(R"re(\btake\("pill_dingyan_dan")re"), (std::multiset<std::string>{"xingchenge"}));
    EXPECT_TRUE(callers(R"re(\bgive\("pill_dingyan_dan")re").empty());
    EXPECT_TRUE(callers(R"re(\b(?:give|take)\("pill_zhuji_dan")re").empty());
    EXPECT_EQ(callers(R"re(\brealm\.demote\(realm\.QI_REFINING_3\))re"), (std::multiset<std::string>{"shudong"}));
    EXPECT_TRUE(callers(R"re(\brealm\.(?:advance|cap)\()re").empty());
    EXPECT_EQ(callers(R"re(\bmagic\.learn\()re"), (std::multiset<std::string>{"xingchenge", "houyuan_jiu"}));
    EXPECT_TRUE(callers(R"re(\bmagic\.forget\()re").empty());
    EXPECT_TRUE(callers(R"re(\bparty\.(?:add|remove)\()re").empty());
    EXPECT_EQ(callers(R"re(\bbattle\()re"), (std::multiset<std::string>{"fuji", "feixu", "jiuren"}));
    EXPECT_EQ(callers(R"re(\btake\("story_diandao_zhenqi_gai")re"), (std::multiset<std::string>{"qidong"}));
}
TEST(Ch09Ledger, TheSealingPredicateNamesExactlyTheFourInheritedFields) {
    const auto code = source(test::chapterNineAssetRoot() + "/scripts/ch09/fengfu.lua");
    for (const char* method : {"ripe", "planted"}) {
        const std::regex pattern(std::string("field\\.") + method + R"re(\("([^"]*)"\))re");
        std::multiset<std::string> fields;
        for (auto it = std::sregex_iterator(code.begin(), code.end(), pattern); it != std::sregex_iterator(); ++it)
            fields.insert((*it)[1].str());
        EXPECT_EQ(fields, (std::multiset<std::string>{"field_baiyaoyuan", "field_baiyaoyuan_jiao", "field_dongfu", "field_dongfu_nei"}));
    }
}
}  // namespace
