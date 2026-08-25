#include "meeting_sdk/intelligence/keyword_topic_extractor.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::intelligence {
namespace {

core::TranscriptSegment makeSegment(std::string text, std::string id) {
    core::TranscriptSegment segment;
    segment.text = std::move(text);
    segment.id = core::SegmentId{std::move(id)};
    return segment;
}

TEST(KeywordTopicExtractor, MostFrequentWordRanksFirst) {
    KeywordTopicExtractor extractor(/*maxTopics=*/2);
    auto topics = extractor.extract({
        makeSegment("security review security approval", "seg1"),
        makeSegment("security is important", "seg2"),
    });
    ASSERT_FALSE(topics.empty());
    EXPECT_EQ(topics[0].label, "security");
}

TEST(KeywordTopicExtractor, RelatedSegmentsListsEveryMentioningSegment) {
    KeywordTopicExtractor extractor(/*maxTopics=*/1);
    auto topics = extractor.extract({
        makeSegment("security review", "seg1"),
        makeSegment("no mention here", "seg2"),
        makeSegment("security approval", "seg3"),
    });
    ASSERT_EQ(topics.size(), 1U);
    EXPECT_EQ(topics[0].label, "security");
    ASSERT_EQ(topics[0].relatedSegments.size(), 2U);
    EXPECT_EQ(topics[0].relatedSegments[0].value, "seg1");
    EXPECT_EQ(topics[0].relatedSegments[1].value, "seg3");
}

TEST(KeywordTopicExtractor, RespectsMaxTopicsLimit) {
    KeywordTopicExtractor extractor(/*maxTopics=*/1);
    auto topics = extractor.extract({makeSegment("alpha bravo charlie delta", "seg1")});
    EXPECT_LE(topics.size(), 1U);
}

TEST(KeywordTopicExtractor, EmptyTranscriptYieldsNoTopics) {
    KeywordTopicExtractor extractor;
    EXPECT_TRUE(extractor.extract({}).empty());
}

}  // namespace
}  // namespace meeting_sdk::intelligence
