#include "meeting_sdk/audio/null_audio_source.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::audio {
namespace {

TEST(NullAudioSource, StartSucceedsAndDeliversNoFrames) {
    NullAudioSource source;
    int framesSeen = 0;
    auto result = source.start([&](core::AudioFrame) { ++framesSeen; });
    EXPECT_TRUE(result);
    EXPECT_EQ(framesSeen, 0);
}

TEST(NullAudioSource, StopSucceeds) {
    NullAudioSource source;
    ASSERT_TRUE(source.start([](core::AudioFrame) {}));
    EXPECT_TRUE(source.stop());
}

}  // namespace
}  // namespace meeting_sdk::audio
