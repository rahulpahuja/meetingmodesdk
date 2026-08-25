#pragma once

#include <cstdint>
#include <vector>

#include "meeting_sdk/core/domain.hpp"
#include "meeting_sdk/core/errors.hpp"

namespace meeting_sdk::storage {

// Serializes a Meeting to/from a compact length-prefixed binary format. This is an internal
// storage-format detail, not a public API contract — only SqliteMeetingRepository depends on
// it. A length-prefixed format avoids the escaping bugs a delimiter-based text format would
// risk, without adding a JSON dependency for a fixed, already-stable set of domain types.
class MeetingSerializer {
public:
    static std::vector<std::uint8_t> serialize(const core::Meeting& meeting);
    static core::Result<core::Meeting> deserialize(const std::vector<std::uint8_t>& bytes);
};

}  // namespace meeting_sdk::storage
