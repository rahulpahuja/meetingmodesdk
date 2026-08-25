#include "meeting_sdk/search/inverted_index_search.hpp"

#include <gtest/gtest.h>

#include "support/time_helpers.hpp"

namespace meeting_sdk::search {
namespace {

using test_support::ts;

core::TranscriptSegment makeSegment(std::string id, std::string speaker, std::string text,
                                     long long startMs) {
    core::TranscriptSegment seg;
    seg.id = core::SegmentId{std::move(id)};
    seg.speaker = core::SpeakerId{std::move(speaker)};
    seg.text = std::move(text);
    seg.range = core::TimeRange{ts(startMs), ts(startMs + 500)};
    return seg;
}

TEST(InvertedIndexSearch, FindsASegmentByKeyword) {
    InvertedIndexSearch index;
    index.index(core::MeetingId{"m1"},
                {makeSegment("seg1", "s1", "we need security approval", 0)});

    auto results = index.search(SearchQuery{.text = "security"});
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0].segment.value, "seg1");
}

TEST(InvertedIndexSearch, RanksMoreMatchingTermsHigher) {
    InvertedIndexSearch index;
    index.index(core::MeetingId{"m1"}, {
        makeSegment("seg1", "s1", "security review happens Monday", 0),
        makeSegment("seg2", "s1", "security approval security review needed", 1000),
    });

    auto results = index.search(SearchQuery{.text = "security review"});
    ASSERT_EQ(results.size(), 2U);
    EXPECT_EQ(results[0].segment.value, "seg2");  // matches "security" twice + "review" once = 3
}

TEST(InvertedIndexSearch, FiltersBySpeaker) {
    InvertedIndexSearch index;
    index.index(core::MeetingId{"m1"}, {
        makeSegment("seg1", "alice", "security topic", 0),
        makeSegment("seg2", "bob", "security topic", 1000),
    });

    auto results = index.search(SearchQuery{.text = "security", .speaker = core::SpeakerId{"alice"}});
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0].segment.value, "seg1");
}

TEST(InvertedIndexSearch, FiltersByMeeting) {
    InvertedIndexSearch index;
    index.index(core::MeetingId{"m1"}, {makeSegment("seg1", "s1", "security topic", 0)});
    index.index(core::MeetingId{"m2"}, {makeSegment("seg2", "s1", "security topic", 0)});

    auto results = index.search(SearchQuery{.text = "security", .meeting = core::MeetingId{"m2"}});
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0].meeting.value, "m2");
}

TEST(InvertedIndexSearch, FiltersByNonOverlappingDateRange) {
    InvertedIndexSearch index;
    index.index(core::MeetingId{"m1"}, {makeSegment("seg1", "s1", "security topic", 0)});

    core::TimeRange farAway{ts(100000), ts(101000)};
    auto results = index.search(SearchQuery{.text = "security", .dateRange = farAway});
    EXPECT_TRUE(results.empty());
}

TEST(InvertedIndexSearch, QueryWithNoMatchingTermsReturnsEmpty) {
    InvertedIndexSearch index;
    index.index(core::MeetingId{"m1"}, {makeSegment("seg1", "s1", "hello world", 0)});

    auto results = index.search(SearchQuery{.text = "blockchain"});
    EXPECT_TRUE(results.empty());
}

}  // namespace
}  // namespace meeting_sdk::search
