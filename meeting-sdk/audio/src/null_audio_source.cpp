#include "meeting_sdk/audio/null_audio_source.hpp"

namespace meeting_sdk::audio {

core::Result<void> NullAudioSource::start(std::function<void(core::AudioFrame)>) { return {}; }

core::Result<void> NullAudioSource::stop() { return {}; }

}  // namespace meeting_sdk::audio
