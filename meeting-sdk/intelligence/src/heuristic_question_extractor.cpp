#include "meeting_sdk/intelligence/heuristic_question_extractor.hpp"

#include "meeting_sdk/intelligence/sentence_splitter.hpp"

namespace meeting_sdk::intelligence {

std::vector<core::Question> HeuristicQuestionExtractor::extract(
    const std::vector<core::TranscriptSegment>& segments) const {
    std::vector<core::Question> result;
    for (const auto& segment : segments) {
        for (const auto& sentence : splitSentences(segment.text)) {
            if (sentence.back() == '?') {
                result.push_back(core::Question{
                    .text = sentence,
                    .sourceSegment = segment.id,
                    .resolved = false,
                });
            }
        }
    }
    return result;
}

}  // namespace meeting_sdk::intelligence
