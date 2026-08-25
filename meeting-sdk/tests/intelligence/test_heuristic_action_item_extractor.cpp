#include "meeting_sdk/intelligence/heuristic_action_item_extractor.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::intelligence {
namespace {

core::TranscriptSegment makeSegment(std::string text, std::string id) {
    core::TranscriptSegment segment;
    segment.text = std::move(text);
    segment.id = core::SegmentId{std::move(id)};
    return segment;
}

TEST(HeuristicActionItemExtractor, FindsACommitmentSentence) {
    HeuristicActionItemExtractor extractor;
    auto items = extractor.extract({makeSegment("Sounds good. I'll send the doc by Friday.", "seg1")});
    ASSERT_EQ(items.size(), 1U);
    EXPECT_EQ(items[0].action, "I'll send the doc by Friday.");
    EXPECT_EQ(items[0].sourceSegment.value, "seg1");
    EXPECT_FALSE(items[0].owner.has_value());
    EXPECT_FALSE(items[0].deadline.has_value());
}

TEST(HeuristicActionItemExtractor, FindsAnActionItemMarker) {
    HeuristicActionItemExtractor extractor;
    auto items = extractor.extract({makeSegment("Action item: update the deck.", "seg1")});
    ASSERT_EQ(items.size(), 1U);
}

TEST(HeuristicActionItemExtractor, IgnoresSentencesWithNoMarker) {
    HeuristicActionItemExtractor extractor;
    auto items = extractor.extract({makeSegment("That was a great meeting.", "seg1")});
    EXPECT_TRUE(items.empty());
}

}  // namespace
}  // namespace meeting_sdk::intelligence
