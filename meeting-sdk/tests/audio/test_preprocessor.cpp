#include "meeting_sdk/audio/preprocessor.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace meeting_sdk::audio {
namespace {

core::AudioFrame makeFrame(std::vector<float> samples) {
    core::AudioFrame frame;
    frame.samples = std::move(samples);
    frame.sampleRateHz = 16000;
    return frame;
}

TEST(Preprocessor, RemovesSustainedDcOffset) {
    Preprocessor pre(/*dcOffsetSmoothing=*/0.9F, /*targetPeak=*/1.0F);
    core::AudioFrame frame = makeFrame(std::vector<float>(50, 0.5F));  // constant offset, no AC content
    pre.process(frame);
    // After enough samples the running estimate should track close to the constant offset,
    // leaving the tail of the frame near zero.
    EXPECT_NEAR(frame.samples.back(), 0.0F, 0.05F);
}

TEST(Preprocessor, ScalesDownPeaksAboveTarget) {
    // smoothing=1.0 freezes the running offset estimate at 0, isolating the scaling behavior.
    Preprocessor pre(/*dcOffsetSmoothing=*/1.0F, /*targetPeak=*/0.5F);
    core::AudioFrame frame = makeFrame({0.0F, 1.0F, -1.0F, 0.2F});
    pre.process(frame);
    float peak = 0.0F;
    for (float s : frame.samples) peak = std::max(peak, std::fabs(s));
    EXPECT_NEAR(peak, 0.5F, 1e-4F);
}

TEST(Preprocessor, NeverAmplifiesQuietFrames) {
    Preprocessor pre(/*dcOffsetSmoothing=*/1.0F, /*targetPeak=*/0.9F);
    core::AudioFrame frame = makeFrame({0.01F, -0.02F, 0.015F});
    auto original = frame.samples;
    pre.process(frame);
    for (std::size_t i = 0; i < original.size(); ++i) {
        EXPECT_NEAR(frame.samples[i], original[i], 1e-4F);
    }
}

}  // namespace
}  // namespace meeting_sdk::audio
