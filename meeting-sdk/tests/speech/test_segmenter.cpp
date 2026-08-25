#include "meeting_sdk/speech/segmenter.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::speech {
namespace {

core::AudioFrame makeFrame() {
    core::AudioFrame frame;
    frame.samples = {0.1F};
    frame.sampleRateHz = 16000;
    return frame;
}

TEST(Segmenter, EmitsSegmentOnSilenceAfterEnoughSpeechFrames) {
    std::vector<SpeechSegment> emitted;
    Segmenter segmenter([&](SpeechSegment seg) { emitted.push_back(std::move(seg)); },
                         /*minSegmentFrames=*/2);

    segmenter.pushFrame(makeFrame(), core::VadDecision::Speech);
    segmenter.pushFrame(makeFrame(), core::VadDecision::Speech);
    segmenter.pushFrame(makeFrame(), core::VadDecision::Silence);

    ASSERT_EQ(emitted.size(), 1U);
    EXPECT_EQ(emitted[0].frames.size(), 2U);
}

TEST(Segmenter, DropsSpuriousBlipsShorterThanMinimum) {
    std::vector<SpeechSegment> emitted;
    Segmenter segmenter([&](SpeechSegment seg) { emitted.push_back(std::move(seg)); },
                         /*minSegmentFrames=*/3);

    segmenter.pushFrame(makeFrame(), core::VadDecision::Speech);  // only 1 frame, below minimum
    segmenter.pushFrame(makeFrame(), core::VadDecision::Silence);

    EXPECT_TRUE(emitted.empty());
}

TEST(Segmenter, FlushEmitsInProgressSegmentWithoutSilence) {
    std::vector<SpeechSegment> emitted;
    Segmenter segmenter([&](SpeechSegment seg) { emitted.push_back(std::move(seg)); },
                         /*minSegmentFrames=*/1);

    segmenter.pushFrame(makeFrame(), core::VadDecision::Speech);
    segmenter.flush();

    ASSERT_EQ(emitted.size(), 1U);
}

TEST(Segmenter, SupportsMultipleSegmentsInOneStream) {
    std::vector<SpeechSegment> emitted;
    Segmenter segmenter([&](SpeechSegment seg) { emitted.push_back(std::move(seg)); },
                         /*minSegmentFrames=*/1);

    segmenter.pushFrame(makeFrame(), core::VadDecision::Speech);
    segmenter.pushFrame(makeFrame(), core::VadDecision::Silence);
    segmenter.pushFrame(makeFrame(), core::VadDecision::Speech);
    segmenter.pushFrame(makeFrame(), core::VadDecision::Silence);

    EXPECT_EQ(emitted.size(), 2U);
}

}  // namespace
}  // namespace meeting_sdk::speech
