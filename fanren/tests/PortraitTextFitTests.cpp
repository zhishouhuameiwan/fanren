// 伙伴改版 · 带头像时正文还装不装得下（只报告，不判红）。
//
// 对白框正文那一块由 ui::drawTextBlock 排：**排不下的行直接截断**（不分页、不滚动，见 ui/Widgets.cpp 的
// drawTextBlock：「画不下的行直接截断，不许溢出面板」）。带头像时正文区窄了「头像宽 + 20」= 132 像素，
// 一个角色一登记头像，他全部的台词都按窄的宽度排——原来刚好五行的，可能就丢了最后半句。
//
// 这条读真树：scripts/ 下所有写死说话人的 talk("角色", "文案键"…)，取真文案（Application::text，键位占位已展开），
// 用真字体量字宽（无头引擎照样载字体、measureText 是真尺寸），按不带头像与带头像（定稿 56×64 → ×2）两种宽度各排一遍，
// 数排出来的行数超过框里放得下的行数的有多少条、是哪些，打到标准输出。
// 文案不归世界画面路：这里只报告，不改文案，也不因为有装不下的就判红——判红只判「真的量到了东西」。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "engine/TextLayout.h"
#include "game/Application.h"
#include "game/DialogueScene.h"
#include "game/Portraits.h"
#include "ui/Widgets.h"

namespace {

namespace fs = std::filesystem;

std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_hanjiacun.tmj")) return candidate;
    }
    return ".";
}

// 写死说话人的 talk：talk("han_mu", "ch01.cunkou.mu_bing"…)。旁白 talk("", …) 不带头像，不算。
std::vector<std::pair<std::string, std::string>> spokenLines(const fs::path& scripts) {
    const std::regex call(R"re(talk\(\s*"([a-z0-9_]+)"\s*,\s*"([^"]+)")re");
    std::set<std::pair<std::string, std::string>> seen;
    for (const auto& entry : fs::recursive_directory_iterator(scripts)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") continue;
        std::ifstream in(entry.path(), std::ios::binary);
        std::stringstream buffer;
        buffer << in.rdbuf();
        const std::string text = buffer.str();
        for (auto it = std::sregex_iterator(text.begin(), text.end(), call); it != std::sregex_iterator(); ++it) {
            seen.emplace((*it)[1].str(), (*it)[2].str());
        }
    }
    return {seen.begin(), seen.end()};
}

// drawTextBlock 在高 h 的一块里画得下几行：从上内边距起，每行 fontSize + lineSpacing，
// 这一行的字底不越过下内边距才画（与 ui/Widgets.cpp 的循环同一条）。
int rowsThatFit(int areaH, const fanren::ui::Theme& theme) {
    int rows = 0;
    const int bottom = areaH - theme.padding;
    for (int y = theme.padding; y + theme.bodyFontSize <= bottom; y += theme.bodyFontSize + theme.lineSpacing) ++rows;
    return rows;
}

TEST(PortraitTextFit, ReportsSpokenLinesThatNoLongerFitBesideAPortrait) {
    fanren::game::Application app;
    ASSERT_TRUE(app.init(repoRoot(), /*headless=*/true).ok);
    const fanren::ui::Theme& theme = app.theme();
    // 不带选项的对白框；带选项时正文那一块的高度一样（选项区另加在框上），只看这一种。
    const fanren::engine::Rect box = fanren::game::DialogueScene::boxAreaFor(0, theme);
    const fanren::engine::Rect area = fanren::ui::panelContentArea(box, std::string{}, theme);
    const auto layout = fanren::game::portraitLayout(area, theme.padding, 56, 64);
    const int plainWidth = area.w - theme.padding * 2;
    const int portraitWidth = layout.text.w - theme.padding * 2;
    const int rows = rowsThatFit(area.h, theme);
    ASSERT_GT(rows, 0);
    ASSERT_LT(portraitWidth, plainWidth);

    fanren::engine::Engine& engine = app.engine();
    const auto measure = [&engine, &theme](const std::string& sample) {
        return engine.measureText(sample, theme.bodyFontSize).x;
    };
    ASSERT_GT(measure("韩立"), 0) << "无头引擎量不出字宽：字体没载进来";

    const auto lines = spokenLines(fs::path(repoRoot()) / "scripts");
    std::vector<std::string> plainOver;
    std::vector<std::string> portraitOver;
    std::set<std::string> speakersOver;
    // 阳性对照：一段明显排不下的话（同一句接六遍）必须被判成装不下，否则下面的 0 什么也没证明。
    std::string tooLong;
    for (int i = 0; i < 6; ++i) tooLong += app.text("ch01.zhenshang.arrive");
    ASSERT_GT(static_cast<int>(fanren::engine::layoutText(tooLong, portraitWidth, measure).size()), rows);

    std::size_t longest = 0;
    std::string longestKey;
    for (const auto& [role, key] : lines) {
        const std::string body = app.text(key);
        const auto plain = fanren::engine::layoutText(body, plainWidth, measure).size();
        const auto narrow = fanren::engine::layoutText(body, portraitWidth, measure).size();
        if (narrow > longest) {
            longest = narrow;
            longestKey = key;
        }
        if (static_cast<int>(plain) > rows) plainOver.push_back(role + " " + key);
        if (static_cast<int>(narrow) > rows) {
            portraitOver.push_back(role + " " + key + "（" + std::to_string(narrow) + " 行）");
            speakersOver.insert(role);
        }
    }

    std::printf("[portrait fit] 量了 %zu 句写死说话人的 talk；框里放得下 %d 行；正文宽 %d → 带头像 %d 像素\n",
                lines.size(), rows, plainWidth, portraitWidth);
    std::printf("[portrait fit] 带头像时最长的一句排 %zu 行（%s）\n", longest, longestKey.c_str());
    std::printf("[portrait fit] 不带头像就已装不下：%zu 句\n", plainOver.size());
    for (const std::string& line : plainOver) std::printf("    %s\n", line.c_str());
    std::printf("[portrait fit] 带头像装不下：%zu 句，涉及 %zu 个角色\n", portraitOver.size(), speakersOver.size());
    for (const std::string& line : portraitOver) std::printf("    %s\n", line.c_str());
    RecordProperty("lines", static_cast<int>(lines.size()));
    RecordProperty("plain_over", static_cast<int>(plainOver.size()));
    RecordProperty("portrait_over", static_cast<int>(portraitOver.size()));

    EXPECT_GT(lines.size(), 1000u) << "真树里写死说话人的 talk 应当上千句：正则没认出来就什么也没量";
    app.shutdown();
}

}  // namespace
