#pragma once

#include <vector>

namespace meeting_sdk::speaker {

// Cosine similarity in [-1, 1]. Returns 0 for a dimension mismatch or a zero-magnitude vector
// — both are treated as "no signal" rather than propagating a NaN into the clustering/
// identification decisions built on top of this.
float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b);

}  // namespace meeting_sdk::speaker
