#pragma once

#include <vector>

#include "meeting_sdk/core/domain.hpp"

namespace meeting_sdk::intelligence {

// Splits each TranscriptSegment's text into sentences and flags any ending in '?' as a
// Question. A literal question mark is strong, precise signal, so this needs no keyword list
// (unlike decisions/action items below). Resolution tracking — whether a later segment
// answered the question — needs cross-segment reasoning a single-pass heuristic can't do
// reliably, so every extracted Question defaults to resolved = false.
class HeuristicQuestionExtractor {
public:
    std::vector<core::Question> extract(const std::vector<core::TranscriptSegment>& segments) const;
};

}  // namespace meeting_sdk::intelligence
