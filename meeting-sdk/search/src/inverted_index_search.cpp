#include "meeting_sdk/search/inverted_index_search.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace meeting_sdk::search {
namespace {

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> words;
    std::string current;
    auto flush = [&] {
        if (current.empty()) {
            return;
        }
        std::transform(current.begin(), current.end(), current.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        words.push_back(current);
        current.clear();
    };
    for (char ch : text) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            current.push_back(ch);
        } else {
            flush();
        }
    }
    flush();
    return words;
}

bool overlaps(const core::TimeRange& a, const core::TimeRange& b) {
    return a.start.value < b.end.value && b.start.value < a.end.value;
}

}  // namespace

void InvertedIndexSearch::index(const core::MeetingId& meeting,
                                 const std::vector<core::TranscriptSegment>& segments) {
    for (const auto& segment : segments) {
        const std::size_t postingIndex = postings_.size();
        postings_.push_back(Posting{meeting, segment.id, segment.speaker, segment.range});
        for (const auto& word : tokenize(segment.text)) {
            wordToPostingIndices_[word].push_back(postingIndex);
        }
    }
}

std::vector<SearchResult> InvertedIndexSearch::search(const SearchQuery& query) const {
    std::unordered_map<std::size_t, int> scoreByPosting;
    for (const auto& word : tokenize(query.text)) {
        const auto it = wordToPostingIndices_.find(word);
        if (it == wordToPostingIndices_.end()) {
            continue;
        }
        for (std::size_t postingIndex : it->second) {
            ++scoreByPosting[postingIndex];
        }
    }

    std::vector<SearchResult> results;
    for (const auto& [postingIndex, score] : scoreByPosting) {
        const auto& posting = postings_[postingIndex];
        if (query.meeting && !(posting.meeting == *query.meeting)) {
            continue;
        }
        if (query.speaker && !(posting.speaker == *query.speaker)) {
            continue;
        }
        if (query.dateRange && !overlaps(posting.range, *query.dateRange)) {
            continue;
        }
        results.push_back(SearchResult{posting.meeting, posting.segment, score});
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) { return a.score > b.score; });
    return results;
}

}  // namespace meeting_sdk::search
