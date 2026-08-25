#include "meeting_sdk/orchestration/transcript_assembler.hpp"

#include <algorithm>

namespace meeting_sdk::orchestration {

void TranscriptAssembler::addSegment(core::TranscriptSegment segment) {
    if (segment.text.empty()) {
        return;
    }
    segments_.push_back(std::move(segment));
}

std::vector<core::TranscriptSegment> TranscriptAssembler::assemble() const {
    std::vector<core::TranscriptSegment> sorted = segments_;
    std::sort(sorted.begin(), sorted.end(),
              [](const core::TranscriptSegment& a, const core::TranscriptSegment& b) {
                  return a.range.start < b.range.start;
              });
    return sorted;
}

}  // namespace meeting_sdk::orchestration
