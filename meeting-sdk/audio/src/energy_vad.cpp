#include "meeting_sdk/audio/energy_vad.hpp"

#include <cmath>

namespace meeting_sdk::audio {

EnergyVad::EnergyVad(float energyThreshold, int hangoverFrames)
    : energyThreshold_(energyThreshold), hangoverFrames_(hangoverFrames) {}

core::Result<core::VadDecision> EnergyVad::process(const core::AudioFrame& frame) {
    if (frame.samples.empty()) {
        return core::Error{
            .category = core::ErrorCategory::Audio,
            .code = "audio.empty_frame",
            .message = "VAD received a frame with no samples",
        };
    }

    double sumSquares = 0.0;
    for (float sample : frame.samples) {
        sumSquares += static_cast<double>(sample) * static_cast<double>(sample);
    }
    const auto rms =
        static_cast<float>(std::sqrt(sumSquares / static_cast<double>(frame.samples.size())));

    if (rms >= energyThreshold_) {
        remainingHangover_ = hangoverFrames_;
        return core::VadDecision::Speech;
    }
    if (remainingHangover_ > 0) {
        --remainingHangover_;
        return core::VadDecision::Speech;
    }
    return core::VadDecision::Silence;
}

}  // namespace meeting_sdk::audio
