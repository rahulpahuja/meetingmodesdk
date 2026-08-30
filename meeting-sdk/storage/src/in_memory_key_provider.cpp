#include "meeting_sdk/storage/in_memory_key_provider.hpp"

#include <sodium.h>

#include "sodium_runtime.hpp"

namespace meeting_sdk::storage {

InMemoryKeyProvider::InMemoryKeyProvider(std::size_t keyLengthBytes) : keyLengthBytes_(keyLengthBytes) {
    static_cast<void>(detail::ensureSodiumInitialized());
}

core::Result<std::vector<std::uint8_t>> InMemoryKeyProvider::getOrCreateKey(const core::MeetingId& id) {
    if (!detail::ensureSodiumInitialized()) {
        return core::Error{
            .category = core::ErrorCategory::Security,
            .code = "storage.crypto_init_failed",
            .message = "libsodium failed to initialize; cannot generate a key",
        };
    }
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
