#include "meeting_sdk/speech/streaming_stt_session.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::speech {
namespace {

// Hand-written fake, not a shipped SDK component — per docs/architecture/
// 04-testing-and-performance-strategy.md §1, domain-layer unit tests use fakes, not mocks.
class FakeSpeechToTextEngine final : public core::ISpeechToTextEngine {
public:
    core::Result<core::TranscriptionSessionHandle> start(
        core::TranscriptionOptions /*options*/, std::function<void(core::PartialResult)> onPartial,
        std::function<void(core::FinalResult)> onFinal) override {
        ++startCalls;
        onPartial_ = std::move(onPartial);
        onFinal_ = std::move(onFinal);
        return core::TranscriptionSessionHandle{42};
    }

    core::Result<void> pushAudio(core::TranscriptionSessionHandle,
                                  const core::AudioFrame& /*frame*/) override {
        ++pushCalls;
        return {};
    }

    core::Result<void> finish(core::TranscriptionSessionHandle) override {
        ++finishCalls;
        return {};
    }

    core::Result<void> cancel(core::TranscriptionSessionHandle) override {
        ++cancelCalls;
        return {};
    }

    int startCalls = 0;
    int pushCalls = 0;
    int finishCalls = 0;
    int cancelCalls = 0;
    std::function<void(core::PartialResult)> onPartial_;
    std::function<void(core::FinalResult)> onFinal_;
};

core::AudioFrame makeFrame() {
    core::AudioFrame frame;
    frame.samples = {0.1F, 0.2F};
    frame.sampleRateHz = 16000;
    return frame;
}

TEST(StreamingSttSession, PushAudioBeforeStartIsRejected) {
    FakeSpeechToTextEngine engine;
    StreamingSttSession session(engine);
    auto result = session.pushAudio(makeFrame());
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().category, core::ErrorCategory::Configuration);
    EXPECT_EQ(result.error().code, "speech.session_not_started");
    EXPECT_EQ(engine.pushCalls, 0);
}

TEST(StreamingSttSession, HappyPathForwardsToEngineInOrder) {
    FakeSpeechToTextEngine engine;
    StreamingSttSession session(engine);

    ASSERT_TRUE(session.start({}, [](core::PartialResult) {}, [](core::FinalResult) {}));
    ASSERT_TRUE(session.pushAudio(makeFrame()));
    ASSERT_TRUE(session.pushAudio(makeFrame()));
    ASSERT_TRUE(session.finish());

    EXPECT_EQ(engine.startCalls, 1);
    EXPECT_EQ(engine.pushCalls, 2);
    EXPECT_EQ(engine.finishCalls, 1);
}

TEST(StreamingSttSession, DoubleStartIsRejected) {
    FakeSpeechToTextEngine engine;
    StreamingSttSession session(engine);
    ASSERT_TRUE(session.start({}, [](core::PartialResult) {}, [](core::FinalResult) {}));

    auto result = session.start({}, [](core::PartialResult) {}, [](core::FinalResult) {});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, "speech.session_already_started");
    EXPECT_EQ(engine.startCalls, 1);  // second start never reached the engine
}

TEST(StreamingSttSession, PushAudioAfterFinishIsRejected) {
    FakeSpeechToTextEngine engine;
    StreamingSttSession session(engine);
    ASSERT_TRUE(session.start({}, [](core::PartialResult) {}, [](core::FinalResult) {}));
    ASSERT_TRUE(session.finish());

    auto result = session.pushAudio(makeFrame());
    ASSERT_FALSE(result);
    EXPECT_EQ(engine.pushCalls, 0);
}

TEST(StreamingSttSession, CancelForwardsAndBlocksFurtherUse) {
    FakeSpeechToTextEngine engine;
    StreamingSttSession session(engine);
    ASSERT_TRUE(session.start({}, [](core::PartialResult) {}, [](core::FinalResult) {}));
    ASSERT_TRUE(session.cancel());
    EXPECT_EQ(engine.cancelCalls, 1);

    EXPECT_FALSE(session.pushAudio(makeFrame()));
    EXPECT_FALSE(session.finish());
}

}  // namespace
}  // namespace meeting_sdk::speech
