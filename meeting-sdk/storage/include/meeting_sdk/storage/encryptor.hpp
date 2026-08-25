#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "meeting_sdk/core/errors.hpp"

namespace meeting_sdk::storage {

// Authenticated symmetric encryption of opaque byte blobs under a caller-supplied key.
// Implementations must use a vetted primitive (e.g. libsodium's crypto_secretbox) — this SDK
// never hand-rolls cryptographic primitives.
class IEncryptor {
public:
    virtual ~IEncryptor() = default;

    virtual core::Result<std::vector<std::uint8_t>> encrypt(const std::vector<std::uint8_t>& plaintext,
                                                              const std::vector<std::uint8_t>& key) = 0;
    virtual core::Result<std::vector<std::uint8_t>> decrypt(const std::vector<std::uint8_t>& ciphertext,
                                                              const std::vector<std::uint8_t>& key) = 0;

    // Required key length in bytes for this implementation.
    virtual std::size_t keyLength() const noexcept = 0;
};

}  // namespace meeting_sdk::storage
