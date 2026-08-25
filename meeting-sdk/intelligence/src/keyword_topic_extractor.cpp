#include "meeting_sdk/intelligence/keyword_topic_extractor.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "meeting_sdk/intelligence/word_frequency.hpp"

namespace meeting_sdk::intelligence {

KeywordTopicExtractor::KeywordTopicExtractor(std::size_t maxTopics) : maxTopics_(maxTopics) {}

std::vector<core::Topic> KeywordTopicExtractor::extract(
    const std::vector<core::TranscriptSegment>& segments) const {
    WordFrequencyAnalyzer analyzer;

    std::vector<std::string> texts;
    texts.reserve(segments.size());
    for (const auto& segment : segments) {
        texts.push_back(segment.text);
    }
    const auto counts = analyzer.countWords(texts);

    std::vector<std::pair<std::string, int>> ranked(counts.begin(), counts.end());
    std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) {
            return a.second > b.second;
        }
        return a.first < b.first;  // deterministic tie-break
    });

    std::vector<core::Topic> topics;
    const std::size_t limit = std::min(maxTopics_, ranked.size());
    for (std::size_t i = 0; i < limit; ++i) {
        core::Topic topic;
        topic.label = ranked[i].first;
        for (const auto& segment : segments) {
            const auto words = analyzer.contentWords(segment.text);
            if (std::find(words.begin(), words.end(), topic.label) != words.end()) {
                topic.relatedSegments.push_back(segment.id);
            }
        }
        topics.push_back(std::move(topic));
    }
    return topics;
}

}  // namespace meeting_sdk::intelligence
