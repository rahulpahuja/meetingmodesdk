#include "meeting_sdk/orchestration/transcript_assembler.hpp"

#include <gtest/gtest.h>

#include "support/time_helpers.hpp"

namespace meeting_sdk::orchestration {
namespace {

using test_support::ts;

core::TranscriptSegment makeSegment(std::string text, long long startMs) {
    core::TranscriptSegment segment;
    segment.text = std::move(text);
    segment.range.start = ts(startMs);
    segment.range.end = ts(startMs + 500);
    return segment;
}

TEST(TranscriptAssembler, OrdersOutOfOrderSegmentsByStartTime) {
    TranscriptAssembler assembler;
    assembler.addSegment(makeSegment("second", 1000));
    assembler.addSegment(makeSegment("first", 0));
    assembler.addSegment(makeSegment("third", 2000));

    auto result = assembler.assemble();
    ASSERT_EQ(result.size(), 3U);
    EXPECT_EQ(result[0].text, "first");
    EXPECT_EQ(result[1].text, "second");
    EXPECT_EQ(result[2].text, "third");
}

TEST(TranscriptAssembler, DropsSegmentsWithEmptyText) {
    TranscriptAssembler assembler;
    assembler.addSegment(makeSegment("", 0));
    assembler.addSegment(makeSegment("real", 500));

    auto result = assembler.assemble();
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].text, "real");
}

TEST(TranscriptAssembler, EmptyAssemblerProducesEmptyTranscript) {
    TranscriptAssembler assembler;
    EXPECT_TRUE(assembler.assemble().empty());
}

}  // namespace
}  // namespace meeting_sdk::orchestration
