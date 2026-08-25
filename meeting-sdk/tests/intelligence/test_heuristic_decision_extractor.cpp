#include "meeting_sdk/intelligence/heuristic_decision_extractor.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::intelligence {
namespace {

core::TranscriptSegment makeSegment(std::string text, std::string id) {
    core::TranscriptSegment segment;
    segment.text = std::move(text);
    segment.id = core::SegmentId{std::move(id)};
    return segment;
}

TEST(HeuristicDecisionExtractor, FindsADecisionMarkerSentence) {
    HeuristicDecisionExtractor extractor;
    auto decisions = extractor.extract({makeSegment("Good point. We will ship next Friday.", "seg1")});
    ASSERT_EQ(decisions.size(), 1U);
    EXPECT_EQ(decisions[0].text, "We will ship next Friday.");
    EXPECT_EQ(decisions[0].sourceSegment.value, "seg1");
    EXPECT_FLOAT_EQ(decisions[0].confidence, 0.5F);
}

TEST(HeuristicDecisionExtractor, MatchIsCaseInsensitive) {
    HeuristicDecisionExtractor extractor;
    auto decisions = extractor.extract({makeSegment("WE'LL GO WITH option two.", "seg1")});
    ASSERT_EQ(decisions.size(), 1U);
}

TEST(HeuristicDecisionExtractor, IgnoresSentencesWithNoMarker) {
    HeuristicDecisionExtractor extractor;
    auto decisions = extractor.extract({makeSegment("That sounds reasonable to me.", "seg1")});
    EXPECT_TRUE(decisions.empty());
}

}  // namespace
}  // namespace meeting_sdk::intelligence
