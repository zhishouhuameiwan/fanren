#pragma once
// 章节标题卡与结局卡（施工图 docs/octopath-overhaul.md 第 4 节）。
//
// 三件事放在一起，因为它们说的是同一张表：
//   1. 章节表：data/chapters.json（章号、章名的文案 key、该章的完成旗标）读进来的样子；
//   2. 排程：哪一次旗标变化该排哪几张卡——纯函数，不开窗口可测；
//   3. 画面：黑底淡入、「第一章」大字 + 章名 + 金色饰线、停一会儿淡出，确认键跳过。
//
// **触发由引擎做，剧情脚本一句不改**：Application 在 SetFlag 把某章的 done_flag 从 0 置成
// 非 0 时记下「第 N 章　终」接「第 N+1 章」开篇，等当前这一场脚本演完、画面回到行走时
// 才压这个场景（Application::flushChapterCards）。新开一局先播第一章的开篇卡。
// 脚本的 ending(title, body) 也落在这里：同一种黑底卡，只是等玩家按确认。
#include <memory>
#include <string>
#include <vector>

#include "core/Result.h"
#include "core/model/Types.h"
#include "game/Scene.h"

namespace fanren::game {

// 章节表的一行，文案已按 key 查好。
struct ChapterEntry {
    int number = 0;
    std::string numeral;    // 「第三章」
    std::string title;      // 「神手谷惊变」
    std::string doneFlag;   // 「ch03.done」
};

struct ChapterTable {
    std::vector<ChapterEntry> chapters;   // 章号严格升序（加载时查过）
    std::string closingWord;              // 「终」
    std::string toBeContinued;            // 「未完待续」

    [[nodiscard]] const ChapterEntry* find(int number) const;
    // 第 number 章是否已经演完：问存档里那一章的 done_flag。表里没有这一章就是「没演完」。
    [[nodiscard]] bool chapterDone(const core::GameState& state, int number) const;
};

// 读 data/chapters.json，把文案 key 换成正文（查不到的 key 按 GameData::lookupText 的
// 规矩回显 key 本身，画面上一眼看得出缺哪条）。
//
// **文件不存在返回空表**：测试夹具的临时资产根多半只拷了一部分 data，而没有章节表的
// 后果只是「不排卡片」，与美术「缺图不崩」同一个口径。文件在而写坏了（语法错、缺字段、
// 章号不升序）如实报错——那是门禁漏过的数据错，不该悄悄吞掉。
[[nodiscard]] core::Result<ChapterTable> loadChapterTable(const std::string& path,
                                                          const core::GameData& data);

enum class CardKind {
    Opening,         // 开篇：「第三章」＋「神手谷惊变」
    Closing,         // 章末：「第三章　终」
    ToBeContinued,   // 最后一章之后：「未完待续」
    Ending,          // 脚本 ending(title, body)：标题 ＋ 正文，等玩家按确认
};

// 排程的结果：播哪一种卡、哪一章的。ToBeContinued 与 Ending 的 chapter 为 0。
struct CardRequest {
    CardKind kind = CardKind::Opening;
    int chapter = 0;
    friend bool operator==(const CardRequest&, const CardRequest&) = default;
};

// 某个旗标从 before 变成 after 时要排的卡。**只有「某一章的 done_flag 由 0 变成非 0」才排**，
// 排的是该章的终卡，接下一章的开篇卡；最后一章之后接「未完待续」。其余一切变化（别的旗标、
// 非 0 改非 0、清回 0）一张都不排——读档、回放、脚本重复置同一个旗标都不会重播。
[[nodiscard]] std::vector<CardRequest> cardsForFlagChange(const ChapterTable& table,
                                                          const std::string& flag, int before,
                                                          int after);

// 新开一局要播的：表里第一章的开篇卡。表空时为空。
[[nodiscard]] std::vector<CardRequest> openingCards(const ChapterTable& table);

// 一张画得出来的卡：字已经定好。
struct Card {
    CardKind kind = CardKind::Opening;
    std::string kicker;     // 小字：开篇卡是章号，终卡是章名，结局卡是标题
    std::string headline;   // 大字：开篇卡是章名，终卡是「第三章　终」
    std::string body;       // 只有结局卡有
};

// 把排程结果翻成字。表里查不到那一章时退成空字（排程本来就只会排表里有的章）。
[[nodiscard]] Card resolveCard(const ChapterTable& table, const CardRequest& request);

class ChapterCardScene : public Scene {
public:
    // 一串章节卡，依次播完即退场。
    explicit ChapterCardScene(std::vector<Card> cards);

    // 结局卡：脚本 ending() 那一条命令。淡入后等玩家按确认，淡出完毕时回填命令。
    // 与对话框同一个口径：**无头下不自己结束**，由驱动它的那一方回填（测试的 pump 就是这么做的）。
    [[nodiscard]] static std::unique_ptr<ChapterCardScene> ending(std::string title,
                                                                  std::string body);

    // 淡入、停留、淡出各多久（秒）。停留约 2.5 秒是施工图写的数。
    static constexpr double kFadeInSeconds = 0.9;
    static constexpr double kHoldSeconds = 2.5;
    static constexpr double kFadeOutSeconds = 0.9;

    // 一张卡从开始算 t 秒时有多「亮」（0 全黑、1 全显）。纯函数，截图口与测试都靠它对时。
    // hold 为负表示一直停着（结局卡等确认）。
    [[nodiscard]] static float visibilityAt(double t, double hold);

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    [[nodiscard]] bool opaque() const override { return true; }
    [[nodiscard]] std::string name() const override { return "ChapterCard"; }

private:
    void startCard(Application& app);
    void drawCard(Application& app, const Card& card, float reveal) const;

    std::vector<Card> cards_;
    std::size_t current_ = 0;
    double t_ = 0.0;          // 当前这一张从开始算的秒数
    // 当前这一张停多久。按确认跳过时改成 0（接着淡出）；结局卡为负，一直停到按确认。
    double hold_ = kHoldSeconds;
    bool started_ = false;    // 当前这一张的音效放过没有
    bool waitsForConfirm_ = false;
};

}  // namespace fanren::game
