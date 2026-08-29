#pragma once

#include "meeting_sdk/core/interfaces.hpp"

namespace meeting_sdk::audio {

// core::IAudioSource that starts successfully and delivers no frames. For hosts whose real
// capture+transcription happens as one atomic step outside this SDK's frame-driven pipeline
// (e.g. Android's SpeechRecognizer, which owns its own microphone capture and hands back
// recognized text, not raw PCM) — orchestration::MeetingPipeline still needs an IAudioSource to
// reach Recording via start(), but such a host drives transcript content entirely through
// MeetingPipeline::ingestTranscribedSegment() instead of frame delivery. Not a fake: it makes no
// claim to capture anything, unlike audio::SyntheticAudioSource's deterministic fixture data.
class NullAudioSource final : public core::IAudioSource {
public:
    core::Result<void> start(std::function<void(core::AudioFrame)> onFrame) override;
    core::Result<void> stop() override;
};

}  // namespace meeting_sdk::audio
