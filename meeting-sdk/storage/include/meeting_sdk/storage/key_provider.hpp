#pragma once

#include <cstdint>
#include <vector>

#include "meeting_sdk/core/domain.hpp"
#include "meeting_sdk/core/errors.hpp"

namespace meeting_sdk::storage {

// Supplies the per-meeting data-encryption key (DEK) used to encrypt that meeting's stored
// data. A real deployment wraps this key with a hardware-backed master key (Android Keystore /
// iOS Secure Enclave) inside a platform bridge (Milestones 9-10; see docs/architecture/
// 03-threat-model-and-security.md §5) — this interface is what the platform-independent C++
// core depends on instead, so it never touches a platform keystore API directly.
class IKeyProvider {
public:
    virtual ~IKeyProvider() = default;

    // Returns the existing key for this meeting, or creates and persists a new one.
    virtual core::Result<std::vector<std::uint8_t>> getOrCreateKey(const core::MeetingId& id) = 0;

    // Crypto-erase: discards the key so any ciphertext encrypted with it becomes permanently
    // unrecoverable, even if the ciphertext itself is not immediately deleted from disk.
    virtual core::Result<void> deleteKey(const core::MeetingId& id) = 0;
};

}  // namespace meeting_sdk::storage
