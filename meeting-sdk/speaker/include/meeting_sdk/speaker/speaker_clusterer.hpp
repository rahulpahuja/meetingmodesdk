#pragma once

#include <cstdint>
#include <vector>

#include "meeting_sdk/core/domain.hpp"

namespace meeting_sdk::speaker {

// Online nearest-centroid speaker clustering over voice embeddings: an incoming embedding is
// assigned to the closest known speaker if cosine similarity to that speaker's centroid exceeds
// a threshold, otherwise a new speaker is created. This is the clustering half of diarization —
// it does not extract embeddings from audio itself (that's a neural model behind a future
// on-device/cloud provider); it operates on whatever embeddings that model produces, so it's
// testable and swappable independent of it.
class SpeakerClusterer {
public:
    explicit SpeakerClusterer(float similarityThreshold = 0.75F);

    // Assigns embedding to an existing speaker or mints a new one; returns the assigned SpeakerId.
    core::SpeakerId assign(core::SpeakerEmbedding embedding);

    const std::vector<core::Speaker>& speakers() const noexcept;

private:
    float similarityThreshold_;
    std::vector<core::Speaker> speakers_;
    std::uint64_t nextSpeakerNumber_ = 1;
};

}  // namespace meeting_sdk::speaker
