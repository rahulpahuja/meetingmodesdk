#include "meeting_sdk/storage/in_memory_key_provider.hpp"

#include <sodium.h>

namespace meeting_sdk::storage {

InMemoryKeyProvider::InMemoryKeyProvider(std::size_t keyLengthBytes) : keyLengthBytes_(keyLengthBytes) {
    // Return value intentionally ignored: 0 = first init, 1 = already initialized, both fine;
    // -1 (init failed) would make randombytes_buf below unsafe, but libsodium has no documented
    // failure mode on supported platforms — idempotent and safe to call from multiple sites.
    static_cast<void>(sodium_init());
}

core::Result<std::vector<std::uint8_t>> InMemoryKeyProvider::getOrCreateKey(const core::MeetingId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = keys_.find(id.value);
    if (it != keys_.end()) {
        return it->second;
    }
    std::vector<std::uint8_t> key(keyLengthBytes_);
    randombytes_buf(key.data(), key.size());
    keys_[id.value] = key;
    return key;
}

core::Result<void> InMemoryKeyProvider::deleteKey(const core::MeetingId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    keys_.erase(id.value);
    return {};
}

}  // namespace meeting_sdk::storage
