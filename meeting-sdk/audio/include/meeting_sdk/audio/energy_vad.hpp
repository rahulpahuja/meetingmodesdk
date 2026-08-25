#pragma once

#include "meeting_sdk/core/interfaces.hpp"

namespace meeting_sdk::audio {

// Short-term energy (RMS) voice activity detector with hangover, so trailing low-energy
// consonants at the end of an utterance aren't misclassified as silence. This is the default
// on-device VAD; a neural VAD can be swapped in later behind the same core::IVAD interface
// (Strategy pattern — docs/architecture/01-requirements-and-architecture.md §4).
class EnergyVad final : public core::IVAD {
public:
    // energyThreshold: RMS level above which a frame is classified as speech.
    // hangoverFrames: number of subsequent low-energy frames still reported as Speech after
    //                 the last frame that crossed the threshold.
    explicit EnergyVad(float energyThreshold = 0.02F, int hangoverFrames = 5);

    core::Result<core::VadDecision> process(const core::AudioFrame& frame) override;

private:
    float energyThreshold_;
    int hangoverFrames_;
    int remainingHangover_ = 0;
};

}  // namespace meeting_sdk::audio
