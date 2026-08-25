#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "meeting_sdk/core/interfaces.hpp"

namespace meeting_sdk::audio {

// Deterministic core::IAudioSource backed by an in-memory sample buffer — used for tests,
// fixtures, and pipeline demos where real hardware capture (Milestones 9-10) isn't available.
// Unlike a hardware-backed source, start() delivers every frame synchronously on the calling
// thread rather than from a realtime capture thread; code that must exercise the threading
// contract in core/interfaces.hpp needs a hardware-backed source, not this one.
class SyntheticAudioSource final : public core::IAudioSource {
public:
    SyntheticAudioSource(std::vector<float> samples, std::uint32_t sampleRateHz, std::size_t frameSize);

    core::Result<void> start(std::function<void(core::AudioFrame)> onFrame) override;
    // Safe to call re-entrantly from inside the onFrame callback to stop delivery early.
    core::Result<void> stop() override;

private:
    std::vector<float> samples_;
    std::uint32_t sampleRateHz_;
    std::size_t frameSize_;
    bool started_ = false;
};

}  // namespace meeting_sdk::audio
