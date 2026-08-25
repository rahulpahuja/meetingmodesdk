#pragma once

#include <vector>

#include "meeting_sdk/core/domain.hpp"

namespace meeting_sdk::orchestration {

// Accumulates TranscriptSegments as they arrive — potentially out of order, e.g. across
// concurrent STT sessions for different speakers — and produces them in temporal order for
// Meeting::transcript. This is the "Transcript Assembly" pipeline stage (see
// docs/architecture/05-diagrams.md §3); it does not perform STT, diarization, or language
// detection itself, only orders and minimally validates their combined output.
class TranscriptAssembler {
public:
    // No-op if segment.text is empty — engines should not emit those, but assembly is the
    // last point before storage where that invariant can still be enforced cheaply.
    void addSegment(core::TranscriptSegment segment);

    // Returns accumulated segments ordered by TimeRange::start.
    std::vector<core::TranscriptSegment> assemble() const;

private:
    std::vector<core::TranscriptSegment> segments_;
};

}  // namespace meeting_sdk::orchestration
