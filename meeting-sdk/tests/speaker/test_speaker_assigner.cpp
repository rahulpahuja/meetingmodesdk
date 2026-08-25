#include "meeting_sdk/speaker/speaker_assigner.hpp"

#include <gtest/gtest.h>

#include "support/time_helpers.hpp"

namespace meeting_sdk::speaker {
namespace {

using test_support::ts;

core::TranscriptSegment makeSegment(long long startMs, long long endMs) {
    core::TranscriptSegment segment;
    segment.text = "hello";
    segment.range = core::TimeRange{ts(startMs), ts(endMs)};
    return segment;
}

TEST(SpeakerAssigner, AssignsSpeakerOfFullyCoveringSpan) {
    SpeakerAssigner assigner({DiarizedSpan{core::SpeakerId{"s1"}, core::TimeRange{ts(0), ts(1000)}}});
    auto result = assigner.assign({makeSegment(100, 900)});
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].speaker.value, "s1");
}

TEST(SpeakerAssigner, PicksTheSpanWithGreaterOverlap) {
    SpeakerAssigner assigner({
        DiarizedSpan{core::SpeakerId{"s1"}, core::TimeRange{ts(0), ts(400)}},
        DiarizedSpan{core::SpeakerId{"s2"}, core::TimeRange{ts(400), ts(1000)}},
    });
    // Segment [200, 800): overlaps s1 by 200ms, s2 by 400ms — s2 should win.
    auto result = assigner.assign({makeSegment(200, 800)});
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].speaker.value, "s2");
}

TEST(SpeakerAssigner, LeavesSpeakerUnchangedWhenNoSpanOverlaps) {
    SpeakerAssigner assigner({DiarizedSpan{core::SpeakerId{"s1"}, core::TimeRange{ts(0), ts(100)}}});
    auto result = assigner.assign({makeSegment(500, 600)});
    ASSERT_EQ(result.size(), 1U);
    EXPECT_TRUE(result[0].speaker.value.empty());
}

}  // namespace
}  // namespace meeting_sdk::speaker
