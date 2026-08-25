#include "meeting_sdk/speech/streaming_stt_session.hpp"

#include <utility>

namespace meeting_sdk::speech {

using core::Error;
using core::ErrorCategory;
using core::Result;

StreamingSttSession::StreamingSttSession(core::ISpeechToTextEngine& engine) : engine_(engine) {}

Result<void> StreamingSttSession::start(core::TranscriptionOptions options,
                                         std::function<void(core::PartialResult)> onPartial,
                                         std::function<void(core::FinalResult)> onFinal) {
    if (state_ != State::Idle) {
        return Error{
            .category = ErrorCategory::Configuration,
            .code = "speech.session_already_started",
            .message = "StreamingSttSession::start called more than once",
        };
    }
    auto result = engine_.start(std::move(options), std::move(onPartial), std::move(onFinal));
    if (!result) {
        return result.error();
    }
    handle_ = result.value();
    state_ = State::Started;
    return {};
}

Result<void> StreamingSttSession::pushAudio(const core::AudioFrame& frame) {
    if (state_ != State::Started) {
        return Error{
            .category = ErrorCategory::Configuration,
            .code = "speech.session_not_started",
            .message = "pushAudio called before start or after finish/cancel",
        };
    }
    return engine_.pushAudio(handle_, frame);
}

Result<void> StreamingSttSession::finish() {
    if (state_ != State::Started) {
        return Error{
            .category = ErrorCategory::Configuration,
            .code = "speech.session_not_started",
            .message = "finish called before start or after finish/cancel",
        };
    }
    auto result = engine_.finish(handle_);
    state_ = State::Finished;
    return result;
}

Result<void> StreamingSttSession::cancel() {
    if (state_ != State::Started) {
        return Error{
            .category = ErrorCategory::Configuration,
            .code = "speech.session_not_started",
            .message = "cancel called before start or after finish/cancel",
        };
    }
    auto result = engine_.cancel(handle_);
    state_ = State::Cancelled;
    return result;
}

}  // namespace meeting_sdk::speech
