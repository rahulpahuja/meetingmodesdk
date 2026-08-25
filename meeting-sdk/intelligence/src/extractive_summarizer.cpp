#include "meeting_sdk/intelligence/extractive_summarizer.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "meeting_sdk/intelligence/word_frequency.hpp"

namespace meeting_sdk::intelligence {

ExtractiveSummarizer::ExtractiveSummarizer(std::size_t maxKeyPoints) : maxKeyPoints_(maxKeyPoints) {}

core::Summary ExtractiveSummarizer::summarize(const std::vector<core::TranscriptSegment>& segments) const {
    core::Summary summary;
    if (segments.empty()) {
        return summary;
    }

    WordFrequencyAnalyzer analyzer;
    std::vector<std::string> texts;
    texts.reserve(segments.size());
    for (const auto& segment : segments) {
        texts.push_back(segment.text);
    }
    const auto freq = analyzer.countWords(texts);

    std::vector<std::pair<float, std::size_t>> scored;  // (score, original index)
    scored.reserve(segments.size());
    for (std::size_t i = 0; i < segments.size(); ++i) {
        const auto words = analyzer.contentWords(segments[i].text);
        float score = 0.0F;
        for (const auto& word : words) {
            const auto it = freq.find(word);
            score += (it != freq.end()) ? static_cast<float>(it->second) : 0.0F;
        }
        if (!words.empty()) {
            score /= static_cast<float>(words.size());
        }
        scored.emplace_back(score, i);
    }
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    const std::size_t k = std::min(maxKeyPoints_, scored.size());
    std::vector<std::size_t> chosen;
    chosen.reserve(k);
    for (std::size_t i = 0; i < k; ++i) {
        chosen.push_back(scored[i].second);
    }
    std::sort(chosen.begin(), chosen.end());  // restore chronological order

    for (std::size_t idx : chosen) {
        summary.keyPoints.push_back(segments[idx].text);
    }
    for (std::size_t i = 0; i < summary.keyPoints.size(); ++i) {
        if (i > 0) {
            summary.text += " ";
        }
        summary.text += summary.keyPoints[i];
    }
    return summary;
}

}  // namespace meeting_sdk::intelligence
