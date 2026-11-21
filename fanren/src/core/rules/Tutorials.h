#pragma once
// 教学卡的规则（第一期地基包，docs/ch07-foundation.md 2.3）：哪张卡此刻该弹、哪些看过了。
//
// core 层硬约束：不含 SDL / JSON / Lua / 文件 IO。「看过没有」只认旗标 tutorial.seen.<id>，
// 不开新的存档字段（总则 1.1：存档版本不升，新状态一律用旗标存）。
#include <functional>
#include <string>
#include <vector>

#include "core/model/Types.h"

namespace fanren::rules {

// 记「这张看过了」的旗标名。
[[nodiscard]] std::string tutorialSeenFlag(const std::string& cardId);

[[nodiscard]] bool tutorialSeen(const core::TutorialCard& card, const core::GameState& state);

// 「第 n 章演完了没有」。与对白头像、角色外观同一个判据（game/SpriteAtlas.h 的 ChapterDone，按章节表的 done_flag）：
// **不看 GameState::chapter**——那个字段剧情不维护，第 7 章章首存档里它还是 1。
using ChapterDoneFn = std::function<bool(int)>;

// 眼下在不在这张卡的章里：maxChapter 那一章还没演完，且 minChapter 的前一章已经演完（0 = 那一头不设限）。
// chapterDone 为空当作哪一章都没演完。
[[nodiscard]] bool tutorialInChapter(const core::TutorialCard& card, const ChapterDoneFn& chapterDone);

// 能弹：章对得上，而且还没看过。
[[nodiscard]] bool tutorialEligible(const core::TutorialCard& card, const core::GameState& state,
                                    const ChapterDoneFn& chapterDone);

// 引擎事件 event 此刻该弹的那张：trigger 等于 event、能弹的里头按表的次序第一张；没有返回 nullptr。
// 空事件名一律没有（trigger 为空的卡只由脚本按 id 叫）。指针指进 cards，即取即用。
[[nodiscard]] const core::TutorialCard* tutorialForEvent(const std::vector<core::TutorialCard>& cards,
                                                         const std::string& event, const core::GameState& state,
                                                         const ChapterDoneFn& chapterDone);

[[nodiscard]] const core::TutorialCard* findTutorial(const std::vector<core::TutorialCard>& cards,
                                                     const std::string& id);

// 记事「指南」页：看过的那些，按表的次序。
[[nodiscard]] std::vector<const core::TutorialCard*> seenTutorials(const std::vector<core::TutorialCard>& cards,
                                                                   const core::GameState& state);

}  // namespace fanren::rules
