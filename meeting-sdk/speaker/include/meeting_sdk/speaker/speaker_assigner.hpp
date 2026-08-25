#pragma once

#include <vector>

#include "meeting_sdk/core/domain.hpp"

namespace meeting_sdk::speaker {

struct DiarizedSpan {
    core::SpeakerId speaker;
    core::TimeRange range;
};

// Associates each TranscriptSegment with the speaker whose diarized span overlaps it the most
// (by wall-clock duration) — the "Speaker Assignment" pipeline stage (see
// docs/architecture/05-diagrams.md §3), decoupled from how diarization spans or transcript
// segments were themselves produced.
class SpeakerAssigner {
public:
    explicit SpeakerAssigner(std::vector<DiarizedSpan> spans);

    // Returns segments with .speaker set to the best-overlapping span's speaker. A segment
    // with no overlapping span is left with its original (unassigned/default) speaker.
    std::vector<core::TranscriptSegment> assign(std::vector<core::TranscriptSegment> segments) const;

private:
    std::vector<DiarizedSpan> spans_;
};

}  // namespace meeting_sdk::speaker
