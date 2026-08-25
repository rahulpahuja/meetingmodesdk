#include "meeting_sdk/audio/preprocessor.hpp"

#include <algorithm>
#include <cmath>

namespace meeting_sdk::audio {

Preprocessor::Preprocessor(float dcOffsetSmoothing, float targetPeak)
    : dcOffsetSmoothing_(dcOffsetSmoothing), targetPeak_(targetPeak) {}

void Preprocessor::process(core::AudioFrame& frame) {
    for (float& sample : frame.samples) {
        runningDcOffset_ = dcOffsetSmoothing_ * runningDcOffset_ + (1.0F - dcOffsetSmoothing_) * sample;
        sample -= runningDcOffset_;
    }

    float peak = 0.0F;
    for (float sample : frame.samples) {
        peak = std::max(peak, std::fabs(sample));
    }
    if (peak > targetPeak_) {
        const float scale = targetPeak_ / peak;
        for (float& sample : frame.samples) {
            sample *= scale;
        }
    }
}

}  // namespace meeting_sdk::audio
