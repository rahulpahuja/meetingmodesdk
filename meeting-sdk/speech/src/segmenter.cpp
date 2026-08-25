#include "meeting_sdk/speech/segmenter.hpp"

namespace meeting_sdk::speech {

Segmenter::Segmenter(std::function<void(SpeechSegment)> onSegment, std::size_t minSegmentFrames)
    : onSegment_(std::move(onSegment)), minSegmentFrames_(minSegmentFrames) {}

void Segmenter::pushFrame(core::AudioFrame frame, core::VadDecision decision) {
    if (decision == core::VadDecision::Speech) {
        current_.push_back(std::move(frame));
        return;
    }
    finalizeIfLongEnough();
}

void Segmenter::flush() { finalizeIfLongEnough(); }

void Segmenter::finalizeIfLongEnough() {
    if (current_.size() >= minSegmentFrames_) {
        SpeechSegment segment;
        segment.range = core::TimeRange{current_.front().capturedAt, current_.back().capturedAt};
        segment.frames = std::move(current_);
        onSegment_(std::move(segment));
    }
    current_.clear();
}

}  // namespace meeting_sdk::speech
