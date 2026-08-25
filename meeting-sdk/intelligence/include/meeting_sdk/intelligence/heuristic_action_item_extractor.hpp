#pragma once

#include <vector>

#include "meeting_sdk/core/domain.hpp"

namespace meeting_sdk::intelligence {

// Flags sentences containing a small closed set of commitment/assignment markers ("I will",
// "please", "need to", "action item", ...) as ActionItems. Owner and deadline extraction need
// named-entity and date-parsing capability this heuristic doesn't attempt — both are left
// unset (nullopt); a real LLM-backed provider fills them in. Confidence is deliberately
// mid-range (0.5) to reflect that this is a keyword match, not semantic understanding.
class HeuristicActionItemExtractor {
public:
    std::vector<core::ActionItem> extract(const std::vector<core::TranscriptSegment>& segments) const;
};

}  // namespace meeting_sdk::intelligence
