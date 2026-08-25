#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "meeting_sdk/core/interfaces.hpp"

namespace meeting_sdk::speaker {

// Identifies a speaker embedding against a small enrolled voiceprint registry (name -> a
// reference embedding) via nearest-neighbor cosine similarity. Deliberately separate from
// SpeakerClusterer/ISpeakerDiarizer: diarization groups "same unidentified speaker" spans,
// identification answers "which known person is this" — different data requirements (see
// docs/architecture/01-requirements-and-architecture.md §4, ISpeakerIdentifier rationale).
class EnrolledSpeakerIdentifier final : public core::ISpeakerIdentifier {
public:
    explicit EnrolledSpeakerIdentifier(float similarityThreshold = 0.8F);

    void enroll(std::string name, core::SpeakerEmbedding referenceEmbedding);

    core::Result<std::optional<std::string>> identify(const core::SpeakerEmbedding& embedding) override;

private:
    float similarityThreshold_;
    std::vector<std::pair<std::string, core::SpeakerEmbedding>> enrolled_;
};

}  // namespace meeting_sdk::speaker
