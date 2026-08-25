#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "meeting_sdk/core/interfaces.hpp"

namespace meeting_sdk::speech {

struct SpeechSegment {
    std::vector<core::AudioFrame> frames;
    core::TimeRange range;
};

// Accumulates frames while VAD reports Speech and emits a complete SpeechSegment once VAD
// reports Silence. VAD's own hangover (see audio::EnergyVad) already prevents trailing
// consonants from being cut, so segmentation only needs a minimum-duration filter here to
// reject spurious noise blips that briefly cross the VAD threshold.
class Segmenter {
public:
    explicit Segmenter(std::function<void(SpeechSegment)> onSegment, std::size_t minSegmentFrames = 3);

    void pushFrame(core::AudioFrame frame, core::VadDecision decision);

    // Flushes any in-progress segment (e.g. on stream end) without waiting for a Silence decision.
    void flush();

private:
    void finalizeIfLongEnough();

    std::function<void(SpeechSegment)> onSegment_;
    std::size_t minSegmentFrames_;
    std::vector<core::AudioFrame> current_;
};

}  // namespace meeting_sdk::speech
