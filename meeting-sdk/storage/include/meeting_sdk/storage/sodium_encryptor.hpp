#pragma once

#include "meeting_sdk/storage/encryptor.hpp"

namespace meeting_sdk::storage {

// Authenticated encryption via libsodium's crypto_secretbox (XSalsa20-Poly1305). Output is
// nonce || ciphertext — the nonce is public and generated fresh per call via a CSPRNG, never
// reused with the same key (reuse would break XSalsa20's security guarantee).
class SodiumEncryptor final : public IEncryptor {
public:
    SodiumEncryptor();

    core::Result<std::vector<std::uint8_t>> encrypt(const std::vector<std::uint8_t>& plaintext,
                                                      const std::vector<std::uint8_t>& key) override;
    core::Result<std::vector<std::uint8_t>> decrypt(const std::vector<std::uint8_t>& ciphertext,
                                                      const std::vector<std::uint8_t>& key) override;
    std::size_t keyLength() const noexcept override;
};

}  // namespace meeting_sdk::storage
