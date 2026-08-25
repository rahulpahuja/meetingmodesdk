#pragma once

#include "meeting_sdk/core/interfaces.hpp"
#include "meeting_sdk/intelligence/extractive_summarizer.hpp"
#include "meeting_sdk/intelligence/heuristic_action_item_extractor.hpp"
#include "meeting_sdk/intelligence/heuristic_decision_extractor.hpp"
#include "meeting_sdk/intelligence/heuristic_question_extractor.hpp"
#include "meeting_sdk/intelligence/keyword_topic_extractor.hpp"

namespace meeting_sdk::intelligence {

// A deterministic, on-device, keyword/pattern-based core::ILLMEngine — the default when no
// LLM provider is configured (core::AIProviderConfig::llm != ProviderMode::Cloud) or as a
// fast local fallback. Each extraction concern is a small, independently-testable component
// composed here, not reimplemented; this class only wires them to the ILLMEngine contract.
// Swappable in full for an LLM-backed providers/cloud or providers/on_device implementation
// without any orchestration-layer change (Strategy pattern — see
// docs/architecture/01-requirements-and-architecture.md §4).
class HeuristicLlmEngine final : public core::ILLMEngine {
public:
    core::Result<core::Summary> summarize(const std::vector<core::TranscriptSegment>& transcript) override;
    core::Result<std::vector<core::ActionItem>> extractActionItems(
        const std::vector<core::TranscriptSegment>& transcript) override;
    core::Result<std::vector<core::Decision>> extractDecisions(
        const std::vector<core::TranscriptSegment>& transcript) override;
    core::Result<std::vector<core::Topic>> extractTopics(
        const std::vector<core::TranscriptSegment>& transcript) override;
    core::Result<std::vector<core::Question>> extractQuestions(
        const std::vector<core::TranscriptSegment>& transcript) override;

private:
    ExtractiveSummarizer summarizer_;
    HeuristicActionItemExtractor actionItemExtractor_;
    HeuristicDecisionExtractor decisionExtractor_;
    KeywordTopicExtractor topicExtractor_;
    HeuristicQuestionExtractor questionExtractor_;
};

}  // namespace meeting_sdk::intelligence
