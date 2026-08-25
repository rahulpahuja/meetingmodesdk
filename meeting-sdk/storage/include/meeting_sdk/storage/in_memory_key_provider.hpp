#pragma once

#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>

#include "meeting_sdk/storage/key_provider.hpp"

namespace meeting_sdk::storage {

// Development/test IKeyProvider: keys live only in process memory, generated via libsodium's
// CSPRNG. This is NOT the production key provider — a real deployment wraps keys with a
// hardware-backed master key via a platform bridge (see key_provider.hpp). This implementation
// exists so the C++ core's encryption path is fully real and testable without a platform
// keystore, the same role SyntheticAudioSource plays for audio capture.
class InMemoryKeyProvider final : public IKeyProvider {
public:
    explicit InMemoryKeyProvider(std::size_t keyLengthBytes);

    core::Result<std::vector<std::uint8_t>> getOrCreateKey(const core::MeetingId& id) override;
    core::Result<void> deleteKey(const core::MeetingId& id) override;

private:
    std::size_t keyLengthBytes_;
    std::mutex mutex_;
    std::unordered_map<std::string, std::vector<std::uint8_t>> keys_;
};

}  // namespace meeting_sdk::storage
