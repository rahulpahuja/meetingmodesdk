#include "meeting_sdk/intelligence/heuristic_llm_engine.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::intelligence {
namespace {

std::vector<core::TranscriptSegment> makeTranscript() {
    core::TranscriptSegment s1;
    s1.id = core::SegmentId{"seg1"};
    s1.text = "We will ship the release Friday. Is security approval done?";

    core::TranscriptSegment s2;
    s2.id = core::SegmentId{"seg2"};
    s2.text = "I'll follow up with the security team.";

    return {s1, s2};
}

TEST(HeuristicLlmEngine, SummarizeReturnsNonEmptySuccess) {
    HeuristicLlmEngine engine;
    auto result = engine.summarize(makeTranscript());
    ASSERT_TRUE(result);
    EXPECT_FALSE(result.value().text.empty());
}

TEST(HeuristicLlmEngine, ExtractActionItemsFindsTheCommitment) {
    HeuristicLlmEngine engine;
    auto result = engine.extractActionItems(makeTranscript());
    ASSERT_TRUE(result);
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value()[0].sourceSegment.value, "seg2");
}

TEST(HeuristicLlmEngine, ExtractDecisionsFindsTheDecision) {
    HeuristicLlmEngine engine;
    auto result = engine.extractDecisions(makeTranscript());
    ASSERT_TRUE(result);
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value()[0].sourceSegment.value, "seg1");
}

TEST(HeuristicLlmEngine, ExtractTopicsReturnsRankedKeywords) {
    HeuristicLlmEngine engine;
    auto result = engine.extractTopics(makeTranscript());
    ASSERT_TRUE(result);
    EXPECT_FALSE(result.value().empty());
}

TEST(HeuristicLlmEngine, ExtractQuestionsFindsTheQuestion) {
    HeuristicLlmEngine engine;
    auto result = engine.extractQuestions(makeTranscript());
    ASSERT_TRUE(result);
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value()[0].text, "Is security approval done?");
}

}  // namespace
}  // namespace meeting_sdk::intelligence
