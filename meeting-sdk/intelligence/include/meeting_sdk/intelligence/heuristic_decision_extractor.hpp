#pragma once

#include <vector>

#include "meeting_sdk/core/domain.hpp"

namespace meeting_sdk::intelligence {

// Flags sentences containing a small closed set of decision-marker phrases ("we will",
// "let's", "decided to", "agreed to", ...) as Decisions. This is a precision-oriented
// heuristic baseline — it will miss decisions phrased unusually and is not a substitute for
// an LLM-backed provider, but gives a working default when no LLM is configured (see
// core::AIProviderConfig::llm). Confidence is deliberately mid-range (0.5) to reflect that
// this is a keyword match, not semantic understanding.
class HeuristicDecisionExtractor {
public:
    std::vector<core::Decision> extract(const std::vector<core::TranscriptSegment>& segments) const;
};

}  // namespace meeting_sdk::intelligence
