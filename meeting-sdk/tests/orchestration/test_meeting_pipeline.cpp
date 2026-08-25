#include "meeting_sdk/orchestration/meeting_pipeline.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "meeting_sdk/audio/synthetic_audio_source.hpp"
#include "meeting_sdk/intelligence/heuristic_llm_engine.hpp"
#include "meeting_sdk/speech/heuristic_language_detector.hpp"
#include "meeting_sdk/storage/in_memory_key_provider.hpp"
#include "meeting_sdk/storage/sodium_encryptor.hpp"
#include "meeting_sdk/storage/sqlite_meeting_repository.hpp"
#include "meeting_sdk/translation/dictionary_translator.hpp"
#include "meeting_sdk/translation/gated_translator.hpp"
#include "support/time_helpers.hpp"

namespace meeting_sdk::orchestration {
namespace {

using test_support::ts;

// Hand-written fakes, not shipped SDK components — per docs/architecture/
// 04-testing-and-performance-strategy.md §1.

// Reports Speech for the first speechFrames calls, then Silence — so Segmenter finalizes exactly
// one SpeechSegment partway through a synchronous audio::SyntheticAudioSource::start() call,
// without depending on real energy thresholds tuned to arbitrary sample values.
class ToggleVad final : public core::IVAD {
public:
    explicit ToggleVad(int speechFrames) : speechFrames_(speechFrames) {}

    core::Result<core::VadDecision> process(const core::AudioFrame&) override {
        ++seen_;
        return seen_ <= speechFrames_ ? core::VadDecision::Speech : core::VadDecision::Silence;
    }

private:
    int speechFrames_;
    int seen_ = 0;
};

// Mirrors a real batch/streaming engine's contract (interfaces.hpp: "batch is streaming with a
// single final chunk") — finish() synchronously fires onFinal with a fixed transcript.
class FakeSpeechToTextEngine final : public core::ISpeechToTextEngine {
public:
    explicit FakeSpeechToTextEngine(std::string text) : text_(std::move(text)) {}

    core::Result<core::TranscriptionSessionHandle> start(
        core::TranscriptionOptions, std::function<void(core::PartialResult)>,
        std::function<void(core::FinalResult)> onFinal) override {
        onFinal_ = std::move(onFinal);
        return core::TranscriptionSessionHandle{++nextHandle_};
    }

    core::Result<void> pushAudio(core::TranscriptionSessionHandle, const core::AudioFrame&) override {
        return {};
    }

    core::Result<void> finish(core::TranscriptionSessionHandle handle) override {
        core::TranscriptSegment segment;
        segment.id = core::SegmentId{"seg-" + std::to_string(handle.value)};
        segment.range = core::TimeRange{ts(0), ts(1000)};
        segment.text = text_;
        segment.confidence = 0.9F;
        onFinal_(core::FinalResult{handle, segment});
        return {};
    }

    core::Result<void> cancel(core::TranscriptionSessionHandle) override { return {}; }

private:
    std::string text_;
    std::function<void(core::FinalResult)> onFinal_;
    std::uint64_t nextHandle_ = 0;
};

class FailingSpeechToTextEngine final : public core::ISpeechToTextEngine {
public:
    core::Result<core::TranscriptionSessionHandle> start(core::TranscriptionOptions,
                                                           std::function<void(core::PartialResult)>,
                                                           std::function<void(core::FinalResult)>) override {
        return core::Error{
            .category = core::ErrorCategory::Model,
            .code = "test.stt_unavailable",
            .message = "engine refused to start",
        };
    }
    core::Result<void> pushAudio(core::TranscriptionSessionHandle, const core::AudioFrame&) override {
        return {};
    }
    core::Result<void> finish(core::TranscriptionSessionHandle) override { return {}; }
    core::Result<void> cancel(core::TranscriptionSessionHandle) override { return {}; }
};

class SingleSpeakerDiarizer final : public core::ISpeakerDiarizer {
public:
    core::Result<std::vector<core::SpeakerId>> diarize(const std::vector<core::AudioFrame>& frames) override {
        return std::vector<core::SpeakerId>(frames.size(), core::SpeakerId{"spk1"});
    }
};

class FixedStepClock final : public core::IClock {
public:
    core::Timestamp now() override {
        current_ += 1000;
        return ts(current_);
    }

private:
    long long current_ = 0;
};

struct Fixture {
    Fixture() : sttEngine("hello world") {}

    ToggleVad vad{5};
    FakeSpeechToTextEngine sttEngine;
    speech::HeuristicLanguageDetector languageDetector;
    SingleSpeakerDiarizer diarizer;
    intelligence::HeuristicLlmEngine llmEngine;
    storage::SodiumEncryptor encryptor;
    storage::InMemoryKeyProvider keyProvider{encryptor.keyLength()};
    std::unique_ptr<storage::SqliteMeetingRepository> repository =
        storage::SqliteMeetingRepository::open(":memory:", encryptor, keyProvider).value();
    FixedStepClock clock;

    MeetingPipeline::Dependencies deps(core::IAudioSource& audioSource,
                                        core::ISpeechToTextEngine* sttOverride = nullptr) {
        return MeetingPipeline::Dependencies{
            .audioSource = audioSource,
            .vad = vad,
            .sttEngine = sttOverride != nullptr ? *sttOverride : sttEngine,
            .languageDetector = languageDetector,
            .diarizer = diarizer,
            .llmEngine = llmEngine,
            .repository = *repository,
            .clock = clock,
        };
    }
};

audio::SyntheticAudioSource makeUtterance() {
    // 6 frames of 160 samples (10ms @ 16kHz, a realistic frame size): 5 clear Segmenter's default
    // minSegmentFrames of 3 and are reported Speech by ToggleVad{5}, the 6th is reported Silence
    // so the segment finalizes synchronously within start()'s delivery loop. A realistic frame
    // size matters here: SyntheticAudioSource never stamps AudioFrame::capturedAt (it's a
    // deterministic in-memory fixture, not a timed capture thread), so MeetingPipeline's
    // diarization-span timing falls back entirely to per-frame sample-duration math — too tiny a
    // frame would round to a zero-duration span and never overlap the transcript segment.
    return audio::SyntheticAudioSource(std::vector<float>(960, 0.5F), 16000, 160);
}

TEST(MeetingPipeline, StartTranscribesDiarizesAndAssemblesOneUtterance) {
    Fixture fx;
    auto source = makeUtterance();
    MeetingPipeline pipeline(core::MeetingId{"m1"}, fx.deps(source));

    ASSERT_TRUE(pipeline.start());
    EXPECT_EQ(pipeline.state(), core::MeetingState::Recording);

    const auto& meeting = pipeline.snapshot();
    ASSERT_EQ(meeting.transcript.size(), 1U);
    EXPECT_EQ(meeting.transcript[0].text, "hello world");
    EXPECT_EQ(meeting.transcript[0].speaker, core::SpeakerId{"spk1"});
    ASSERT_EQ(meeting.speakers.size(), 1U);
    EXPECT_EQ(meeting.speakers[0].id, core::SpeakerId{"spk1"});
}

TEST(MeetingPipeline, StopRunsIntelligenceExtractionAndPersists) {
    Fixture fx;
    auto source = makeUtterance();
    MeetingPipeline pipeline(core::MeetingId{"m1"}, fx.deps(source));
    ASSERT_TRUE(pipeline.start());

    auto stopped = pipeline.stop();
    ASSERT_TRUE(stopped);
    EXPECT_EQ(stopped.value().state, core::MeetingState::Completed);
    EXPECT_EQ(pipeline.state(), core::MeetingState::Completed);
    ASSERT_TRUE(stopped.value().summary.has_value());

    auto fetched = fx.repository->get(core::MeetingId{"m1"});
    ASSERT_TRUE(fetched);
    EXPECT_EQ(fetched.value().transcript.size(), 1U);
}

TEST(MeetingPipeline, PauseThenResumeReturnsToRecording) {
    Fixture fx;
    auto source = makeUtterance();
    MeetingPipeline pipeline(core::MeetingId{"m1"}, fx.deps(source));
    ASSERT_TRUE(pipeline.start());

    ASSERT_TRUE(pipeline.pause());
    EXPECT_EQ(pipeline.state(), core::MeetingState::Paused);

    ASSERT_TRUE(pipeline.resume());
    EXPECT_EQ(pipeline.state(), core::MeetingState::Recording);
}

TEST(MeetingPipeline, StopBeforeStartIsRejected) {
    Fixture fx;
    auto source = makeUtterance();
    MeetingPipeline pipeline(core::MeetingId{"m1"}, fx.deps(source));

    auto result = pipeline.stop();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().category, core::ErrorCategory::Configuration);
}

TEST(MeetingPipeline, SttFailureDuringSynchronousCaptureIsSurfacedWithoutEnteringRecording) {
    Fixture fx;
    FailingSpeechToTextEngine failingEngine;
    auto source = makeUtterance();
    MeetingPipeline pipeline(core::MeetingId{"m1"}, fx.deps(source, &failingEngine));

    auto result = pipeline.start();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, "test.stt_unavailable");
    EXPECT_EQ(pipeline.state(), core::MeetingState::Idle);
}

TEST(MeetingPipeline, TranslateSegmentWithoutConfiguredTranslatorIsRejected) {
    Fixture fx;
    auto source = makeUtterance();
    MeetingPipeline pipeline(core::MeetingId{"m1"}, fx.deps(source));
    ASSERT_TRUE(pipeline.start());

    auto result = pipeline.translateSegment(pipeline.snapshot().transcript[0].id, "hi");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, "orchestration.translation_not_configured");
}

TEST(MeetingPipeline, TranslateSegmentUsesConfiguredTranslator) {
    Fixture fx;
    translation::DictionaryTranslator dictionary;
    translation::GatedTranslator translator(core::ProviderMode::OnDevice, &dictionary);
    auto source = makeUtterance();
    auto deps = fx.deps(source);
    deps.translator = &translator;
    MeetingPipeline pipeline(core::MeetingId{"m1"}, deps);
    ASSERT_TRUE(pipeline.start());

    auto result = pipeline.translateSegment(pipeline.snapshot().transcript[0].id, "hi");
    ASSERT_TRUE(result);
}

}  // namespace
}  // namespace meeting_sdk::orchestration
