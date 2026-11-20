#include "core/rules/Tutorials.h"

namespace fanren::rules {

std::string tutorialSeenFlag(const std::string& cardId) {
    return "tutorial.seen." + cardId;
}

bool tutorialSeen(const core::TutorialCard& card, const core::GameState& state) {
    return state.flag(tutorialSeenFlag(card.id)) != 0;
}

bool tutorialInChapter(const core::TutorialCard& card, const ChapterDoneFn& chapterDone) {
    const auto done = [&chapterDone](int number) { return chapterDone && chapterDone(number); };
    if (card.minChapter > 1 && !done(card.minChapter - 1)) return false;
    if (card.maxChapter > 0 && done(card.maxChapter)) return false;
    return true;
}

bool tutorialEligible(const core::TutorialCard& card, const core::GameState& state, const ChapterDoneFn& chapterDone) {
    return tutorialInChapter(card, chapterDone) && !tutorialSeen(card, state);
}

const core::TutorialCard* tutorialForEvent(const std::vector<core::TutorialCard>& cards, const std::string& event,
                                           const core::GameState& state, const ChapterDoneFn& chapterDone) {
    if (event.empty()) return nullptr;
    for (const core::TutorialCard& card : cards) {
        if (card.trigger == event && tutorialEligible(card, state, chapterDone)) return &card;
    }
    return nullptr;
}

const core::TutorialCard* findTutorial(const std::vector<core::TutorialCard>& cards, const std::string& id) {
    for (const core::TutorialCard& card : cards) {
        if (card.id == id) return &card;
    }
    return nullptr;
}

std::vector<const core::TutorialCard*> seenTutorials(const std::vector<core::TutorialCard>& cards,
                                                     const core::GameState& state) {
    std::vector<const core::TutorialCard*> seen;
    for (const core::TutorialCard& card : cards) {
        if (tutorialSeen(card, state)) seen.push_back(&card);
    }
    return seen;
}

}  // namespace fanren::rules
