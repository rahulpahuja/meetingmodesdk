#pragma once

#include "meeting_sdk/core/types.hpp"

namespace meeting_sdk::audio {

// Minimal, deterministic signal conditioning ahead of VAD: DC-offset removal (a running-mean
// high-pass) and downward-only peak normalization. Deliberately not a full noise-reduction
// model — spectral denoising belongs behind a swappable provider (providers/on_device), not
// hardcoded into the core pipeline.
class Preprocessor {
public:
    explicit Preprocessor(float dcOffsetSmoothing = 0.995F, float targetPeak = 0.95F);

    // Mutates frame in place: removes DC offset, then scales down to targetPeak if the frame's
    // peak amplitude exceeds it. Never amplifies — a quiet/near-silent frame is left as-is so
    // background noise isn't boosted.
    void process(core::AudioFrame& frame);

private:
    float dcOffsetSmoothing_;
    float targetPeak_;
    float runningDcOffset_ = 0.0F;
};

}  // namespace meeting_sdk::audio
