#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "meeting_sdk/core/domain.hpp"
#include "meeting_sdk/search/search_query.hpp"

namespace meeting_sdk::search {

// Keyword (inverted-index) search over transcript segments with meeting/speaker/date-range
// filtering. Semantic (embedding-based) search needs a real multilingual sentence-embedding
// model — not available in this environment, the same constraint that blocked a real
// STT/diarizer/NMT in earlier milestones (see docs/architecture/
// 01-requirements-and-architecture.md). This is a real, working keyword fallback ranked by
// term-match count, not a placeholder — many production search systems ship exactly this as
// one retrieval signal even after adding embeddings, and cross-language matching is the one
// capability it genuinely cannot provide without them.
class InvertedIndexSearch {
public:
    void index(const core::MeetingId& meeting, const std::vector<core::TranscriptSegment>& segments);

    std::vector<SearchResult> search(const SearchQuery& query) const;

private:
    struct Posting {
        core::MeetingId meeting;
        core::SegmentId segment;
        core::SpeakerId speaker;
        core::TimeRange range;
    };

    std::unordered_map<std::string, std::vector<std::size_t>> wordToPostingIndices_;
    std::vector<Posting> postings_;
};

}  // namespace meeting_sdk::search
