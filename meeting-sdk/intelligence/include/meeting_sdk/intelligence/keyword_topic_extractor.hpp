#pragma once

#include <cstddef>
#include <vector>

#include "meeting_sdk/core/domain.hpp"

namespace meeting_sdk::intelligence {

// Ranks content words (stopwords excluded) by frequency across the whole transcript and
// returns the top N as Topics, each linked to every segment that mentions it. Frequency is a
// crude proxy for "topic" — a real embedding-clustering provider is a strict upgrade — but
// it's deterministic, needs no model, and gives every meeting at least some searchable topic
// labels when no LLM/embedding provider is configured.
class KeywordTopicExtractor {
public:
    explicit KeywordTopicExtractor(std::size_t maxTopics = 5);

    std::vector<core::Topic> extract(const std::vector<core::TranscriptSegment>& segments) const;

private:
    std::size_t maxTopics_;
};

}  // namespace meeting_sdk::intelligence
