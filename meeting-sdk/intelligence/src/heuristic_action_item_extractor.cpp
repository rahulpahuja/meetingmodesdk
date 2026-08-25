#include "meeting_sdk/intelligence/heuristic_action_item_extractor.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <string_view>

#include "meeting_sdk/intelligence/sentence_splitter.hpp"

namespace meeting_sdk::intelligence {
namespace {

constexpr std::array<std::string_view, 8> kActionMarkers = {
    "i will", "i'll",  "we need to", "need to", "please", "can you", "action item", "todo",
};

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool containsAnyMarker(const std::string& lowerSentence) {
    return std::any_of(kActionMarkers.begin(), kActionMarkers.end(),
                        [&](std::string_view marker) { return lowerSentence.find(marker) != std::string::npos; });
}

}  // namespace

std::vector<core::ActionItem> HeuristicActionItemExtractor::extract(
    const std::vector<core::TranscriptSegment>& segments) const {
    std::vector<core::ActionItem> result;
    for (const auto& segment : segments) {
        for (const auto& sentence : splitSentences(segment.text)) {
            if (containsAnyMarker(toLower(sentence))) {
                result.push_back(core::ActionItem{
                    .action = sentence,
                    .owner = std::nullopt,
                    .deadline = std::nullopt,
                    .sourceSegment = segment.id,
                    .confidence = 0.5F,
                });
            }
        }
    }
    return result;
}

}  // namespace meeting_sdk::intelligence
