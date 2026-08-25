#include "meeting_sdk/intelligence/extractive_summarizer.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::intelligence {
namespace {

core::TranscriptSegment makeSegment(std::string text, std::string id) {
    core::TranscriptSegment segment;
    segment.text = std::move(text);
    segment.id = core::SegmentId{std::move(id)};
    return segment;
}

TEST(ExtractiveSummarizer, EmptyTranscriptYieldsEmptySummary) {
    ExtractiveSummarizer summarizer;
    auto summary = summarizer.summarize({});
    EXPECT_TRUE(summary.text.empty());
    EXPECT_TRUE(summary.keyPoints.empty());
}

TEST(ExtractiveSummarizer, KeyPointsPreserveChronologicalOrder) {
    ExtractiveSummarizer summarizer(/*maxKeyPoints=*/2);
    auto summary = summarizer.summarize({
        makeSegment("security review happens Monday", "seg1"),
        makeSegment("lunch was fine", "seg2"),
        makeSegment("security approval needed before launch", "seg3"),
    });
    ASSERT_EQ(summary.keyPoints.size(), 2U);
    // Both security-heavy segments should outscore the filler one, and stay in original order.
    EXPECT_EQ(summary.keyPoints[0], "security review happens Monday");
    EXPECT_EQ(summary.keyPoints[1], "security approval needed before launch");
}

TEST(ExtractiveSummarizer, TextIsKeyPointsJoinedWithSpaces) {
    ExtractiveSummarizer summarizer(/*maxKeyPoints=*/1);
    auto summary = summarizer.summarize({makeSegment("only one segment here", "seg1")});
    EXPECT_EQ(summary.text, "only one segment here");
}

TEST(ExtractiveSummarizer, NeverExceedsMaxKeyPoints) {
    ExtractiveSummarizer summarizer(/*maxKeyPoints=*/1);
    auto summary = summarizer.summarize({
        makeSegment("alpha bravo", "seg1"),
        makeSegment("charlie delta", "seg2"),
        makeSegment("echo foxtrot", "seg3"),
    });
    EXPECT_EQ(summary.keyPoints.size(), 1U);
}

}  // namespace
}  // namespace meeting_sdk::intelligence
