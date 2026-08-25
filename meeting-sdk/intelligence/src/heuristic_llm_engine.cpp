#include "meeting_sdk/intelligence/heuristic_llm_engine.hpp"

namespace meeting_sdk::intelligence {

core::Result<core::Summary> HeuristicLlmEngine::summarize(
    const std::vector<core::TranscriptSegment>& transcript) {
    return summarizer_.summarize(transcript);
}

core::Result<std::vector<core::ActionItem>> HeuristicLlmEngine::extractActionItems(
    const std::vector<core::TranscriptSegment>& transcript) {
    return actionItemExtractor_.extract(transcript);
}

core::Result<std::vector<core::Decision>> HeuristicLlmEngine::extractDecisions(
    const std::vector<core::TranscriptSegment>& transcript) {
    return decisionExtractor_.extract(transcript);
}

core::Result<std::vector<core::Topic>> HeuristicLlmEngine::extractTopics(
    const std::vector<core::TranscriptSegment>& transcript) {
    return topicExtractor_.extract(transcript);
}

core::Result<std::vector<core::Question>> HeuristicLlmEngine::extractQuestions(
    const std::vector<core::TranscriptSegment>& transcript) {
    return questionExtractor_.extract(transcript);
}

}  // namespace meeting_sdk::intelligence
