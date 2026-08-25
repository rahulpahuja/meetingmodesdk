#include "meeting_sdk/audio/synthetic_audio_source.hpp"

#include <algorithm>

namespace meeting_sdk::audio {

SyntheticAudioSource::SyntheticAudioSource(std::vector<float> samples, std::uint32_t sampleRateHz,
                                            std::size_t frameSize)
    : samples_(std::move(samples)), sampleRateHz_(sampleRateHz), frameSize_(frameSize) {}

core::Result<void> SyntheticAudioSource::start(std::function<void(core::AudioFrame)> onFrame) {
    if (started_) {
        return core::Error{
            .category = core::ErrorCategory::Configuration,
            .code = "audio.already_started",
            .message = "SyntheticAudioSource::start called twice without an intervening stop",
        };
    }
    if (frameSize_ == 0) {
        return core::Error{
            .category = core::ErrorCategory::Configuration,
            .code = "audio.invalid_frame_size",
            .message = "frameSize must be greater than zero",
        };
    }
    started_ = true;

    for (std::size_t offset = 0; offset < samples_.size() && started_; offset += frameSize_) {
        const std::size_t end = std::min(offset + frameSize_, samples_.size());
        core::AudioFrame frame;
        frame.samples.assign(samples_.begin() + static_cast<std::ptrdiff_t>(offset),
                              samples_.begin() + static_cast<std::ptrdiff_t>(end));
        frame.sampleRateHz = sampleRateHz_;
        onFrame(frame);
    }
    started_ = false;
    return {};
}

core::Result<void> SyntheticAudioSource::stop() {
    started_ = false;
    return {};
}

}  // namespace meeting_sdk::audio
