#pragma once

#include <cstddef>
#include <vector>

#include "meeting_sdk/core/domain.hpp"

namespace meeting_sdk::intelligence {

// Extractive summarization baseline: scores each segment by the average frequency of its
// content words across the whole transcript (a simplified, single-document salience signal)
// and returns the top-scoring segments, restored to chronological order, as both the summary
// text and keyPoints. This is extractive, not abstractive — it never generates a sentence
// that wasn't spoken — which is the deliberate trade-off for not depending on an LLM.
class ExtractiveSummarizer {
public:
    explicit ExtractiveSummarizer(std::size_t maxKeyPoints = 3);

    core::Summary summarize(const std::vector<core::TranscriptSegment>& segments) const;

private:
    std::size_t maxKeyPoints_;
};

}  // namespace meeting_sdk::intelligence
