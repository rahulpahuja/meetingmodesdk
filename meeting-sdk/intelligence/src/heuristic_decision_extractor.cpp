#include "meeting_sdk/intelligence/heuristic_decision_extractor.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>

#include "meeting_sdk/intelligence/sentence_splitter.hpp"

namespace meeting_sdk::intelligence {
namespace {

constexpr std::array<std::string_view, 9> kDecisionMarkers = {
    "we will",  "we'll",       "let's",       "lets",     "decided to",
    "decision is", "agreed to", "we agree", "we'll go with",
};

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool containsAnyMarker(const std::string& lowerSentence) {
    return std::any_of(kDecisionMarkers.begin(), kDecisionMarkers.end(),
                        [&](std::string_view marker) { return lowerSentence.find(marker) != std::string::npos; });
}

}  // namespace

std::vector<core::Decision> HeuristicDecisionExtractor::extract(
    const std::vector<core::TranscriptSegment>& segments) const {
    std::vector<core::Decision> result;
    for (const auto& segment : segments) {
        for (const auto& sentence : splitSentences(segment.text)) {
            if (containsAnyMarker(toLower(sentence))) {
                result.push_back(core::Decision{
                    .text = sentence,
                    .sourceSegment = segment.id,
                    .confidence = 0.5F,
                });
            }
        }
    }
    return result;
}

}  // namespace meeting_sdk::intelligence
