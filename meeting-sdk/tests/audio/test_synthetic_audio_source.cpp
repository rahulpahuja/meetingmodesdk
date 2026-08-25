#include "meeting_sdk/audio/synthetic_audio_source.hpp"

#include <gtest/gtest.h>

#include <tuple>
#include <vector>

namespace meeting_sdk::audio {
namespace {

TEST(SyntheticAudioSource, DeliversAllSamplesAsFixedSizeFrames) {
    SyntheticAudioSource source({1.F, 2.F, 3.F, 4.F, 5.F}, /*sampleRateHz=*/16000, /*frameSize=*/2);
    std::vector<std::vector<float>> delivered;
    auto result = source.start([&](core::AudioFrame frame) { delivered.push_back(frame.samples); });
    ASSERT_TRUE(result);
    ASSERT_EQ(delivered.size(), 3U);  // 2 + 2 + 1 (final short frame)
    EXPECT_EQ(delivered[0], (std::vector<float>{1.F, 2.F}));
    EXPECT_EQ(delivered[1], (std::vector<float>{3.F, 4.F}));
    EXPECT_EQ(delivered[2], (std::vector<float>{5.F}));
}

TEST(SyntheticAudioSource, StopCalledFromCallbackHaltsDeliveryEarly) {
    SyntheticAudioSource source({1.F, 2.F, 3.F, 4.F}, 16000, 1);
    int framesSeen = 0;
    auto result = source.start([&](core::AudioFrame) {
        ++framesSeen;
        if (framesSeen == 2) {
            std::ignore = source.stop();
        }
    });
    ASSERT_TRUE(result);
    EXPECT_EQ(framesSeen, 2);
}

TEST(SyntheticAudioSource, CanBeRestartedAfterNaturalCompletion) {
    SyntheticAudioSource source({1.F, 2.F}, 16000, 1);
    int firstRun = 0;
    ASSERT_TRUE(source.start([&](core::AudioFrame) { ++firstRun; }));
    int secondRun = 0;
    ASSERT_TRUE(source.start([&](core::AudioFrame) { ++secondRun; }));
    EXPECT_EQ(firstRun, 2);
    EXPECT_EQ(secondRun, 2);
}

TEST(SyntheticAudioSource, RejectsZeroFrameSize) {
    SyntheticAudioSource source({1.F}, 16000, 0);
    auto result = source.start([](core::AudioFrame) {});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, "audio.invalid_frame_size");
}

}  // namespace
}  // namespace meeting_sdk::audio
