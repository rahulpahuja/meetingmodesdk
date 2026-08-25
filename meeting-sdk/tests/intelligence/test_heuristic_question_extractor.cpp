#include "meeting_sdk/intelligence/heuristic_question_extractor.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::intelligence {
namespace {

core::TranscriptSegment makeSegment(std::string text, std::string id) {
    core::TranscriptSegment segment;
    segment.text = std::move(text);
    segment.id = core::SegmentId{std::move(id)};
    return segment;
}

TEST(HeuristicQuestionExtractor, FindsAQuestionSentence) {
    HeuristicQuestionExtractor extractor;
    auto questions = extractor.extract({makeSegment("We shipped it. Are we ready for launch?", "seg1")});
    ASSERT_EQ(questions.size(), 1U);
    EXPECT_EQ(questions[0].text, "Are we ready for launch?");
    EXPECT_EQ(questions[0].sourceSegment.value, "seg1");
    EXPECT_FALSE(questions[0].resolved);
}

TEST(HeuristicQuestionExtractor, IgnoresSegmentsWithNoQuestion) {
    HeuristicQuestionExtractor extractor;
    auto questions = extractor.extract({makeSegment("We shipped it on time.", "seg1")});
    EXPECT_TRUE(questions.empty());
}

TEST(HeuristicQuestionExtractor, FindsMultipleQuestionsAcrossSegments) {
    HeuristicQuestionExtractor extractor;
    auto questions = extractor.extract({
        makeSegment("Is this ready?", "seg1"),
        makeSegment("What about security?", "seg2"),
    });
    ASSERT_EQ(questions.size(), 2U);
    EXPECT_EQ(questions[0].sourceSegment.value, "seg1");
    EXPECT_EQ(questions[1].sourceSegment.value, "seg2");
}

}  // namespace
}  // namespace meeting_sdk::intelligence
