#pragma once

#include <optional>
#include <string>

#include "meeting_sdk/core/domain.hpp"

namespace meeting_sdk::search {

struct SearchQuery {
    std::string text;
    std::optional<core::MeetingId> meeting;
    std::optional<core::SpeakerId> speaker;
    std::optional<core::TimeRange> dateRange;  // matches if the segment's range overlaps this
};

struct SearchResult {
    core::MeetingId meeting;
    core::SegmentId segment;
    int score = 0;
};

}  // namespace meeting_sdk::search
