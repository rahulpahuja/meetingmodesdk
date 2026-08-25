#include "meeting_sdk/speaker/speaker_assigner.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace meeting_sdk::speaker {
namespace {

std::chrono::nanoseconds overlap(const core::TimeRange& a, const core::TimeRange& b) {
    const auto start = std::max(a.start.value, b.start.value);
    const auto end = std::min(a.end.value, b.end.value);
    if (end <= start) {
        return std::chrono::nanoseconds::zero();
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

}  // namespace

SpeakerAssigner::SpeakerAssigner(std::vector<DiarizedSpan> spans) : spans_(std::move(spans)) {}

std::vector<core::TranscriptSegment> SpeakerAssigner::assign(
    std::vector<core::TranscriptSegment> segments) const {
    for (auto& segment : segments) {
        const DiarizedSpan* best = nullptr;
        auto bestOverlap = std::chrono::nanoseconds::zero();
        for (const auto& span : spans_) {
            const auto ov = overlap(segment.range, span.range);
            if (ov > bestOverlap) {
                bestOverlap = ov;
                best = &span;
            }
        }
        if (best != nullptr) {
            segment.speaker = best->speaker;
        }
    }
    return segments;
}

}  // namespace meeting_sdk::speaker
