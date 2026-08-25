#include "meeting_sdk/audio/energy_vad.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace meeting_sdk::audio {
namespace {

core::AudioFrame makeFrame(std::vector<float> samples) {
    core::AudioFrame frame;
    frame.samples = std::move(samples);
    frame.sampleRateHz = 16000;
    return frame;
}

TEST(EnergyVad, ClassifiesLoudFrameAsSpeech) {
    EnergyVad vad(/*energyThreshold=*/0.02F, /*hangoverFrames=*/0);
    auto result = vad.process(makeFrame({0.5F, -0.5F, 0.5F, -0.5F}));
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), core::VadDecision::Speech);
}

TEST(EnergyVad, ClassifiesQuietFrameAsSilenceWithNoHangover) {
    EnergyVad vad(/*energyThreshold=*/0.02F, /*hangoverFrames=*/0);
    auto result = vad.process(makeFrame({0.001F, -0.001F, 0.001F}));
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), core::VadDecision::Silence);
}

TEST(EnergyVad, HangoverKeepsReportingSpeechAfterEnergyDrops) {
    EnergyVad vad(/*energyThreshold=*/0.02F, /*hangoverFrames=*/2);
    ASSERT_EQ(vad.process(makeFrame({0.5F, -0.5F})).value(), core::VadDecision::Speech);
    // Two quiet frames still count as Speech (hangover), the third does not.
    EXPECT_EQ(vad.process(makeFrame({0.0F, 0.0F})).value(), core::VadDecision::Speech);
    EXPECT_EQ(vad.process(makeFrame({0.0F, 0.0F})).value(), core::VadDecision::Speech);
    EXPECT_EQ(vad.process(makeFrame({0.0F, 0.0F})).value(), core::VadDecision::Silence);
}

TEST(EnergyVad, EmptyFrameIsAnAudioError) {
    EnergyVad vad;
    auto result = vad.process(makeFrame({}));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().category, core::ErrorCategory::Audio);
    EXPECT_EQ(result.error().code, "audio.empty_frame");
}

}  // namespace
}  // namespace meeting_sdk::audio
