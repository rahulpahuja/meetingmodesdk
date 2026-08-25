#pragma once

#include <functional>

#include "meeting_sdk/core/interfaces.hpp"

namespace meeting_sdk::speech {

// Wraps a core::ISpeechToTextEngine and enforces correct call sequencing for one streaming
// session (start -> pushAudio* -> finish|cancel), independent of which concrete engine
// (on-device or cloud) is behind it. orchestration drives sessions through this wrapper, not
// the raw engine, so a provider that mishandles out-of-sequence calls can't corrupt pipeline
// state — the guard lives in one place instead of being re-implemented per provider.
class StreamingSttSession {
public:
    explicit StreamingSttSession(core::ISpeechToTextEngine& engine);

    core::Result<void> start(core::TranscriptionOptions options,
                              std::function<void(core::PartialResult)> onPartial,
                              std::function<void(core::FinalResult)> onFinal);
    core::Result<void> pushAudio(const core::AudioFrame& frame);
    core::Result<void> finish();
    core::Result<void> cancel();

private:
    enum class State { Idle, Started, Finished, Cancelled };

    core::ISpeechToTextEngine& engine_;
    State state_ = State::Idle;
    core::TranscriptionSessionHandle handle_{};
};

}  // namespace meeting_sdk::speech
