#pragma once

namespace meeting_sdk::storage::detail {

// Thread-safe, once-only libsodium initialization. Returns false only if sodium_init()
// reported failure (-1); callers must then refuse to use the CSPRNG or crypto_secretbox
// rather than proceed with an uninitialized library.
bool ensureSodiumInitialized() noexcept;

}  // namespace meeting_sdk::storage::detail
