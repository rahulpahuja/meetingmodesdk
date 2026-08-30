#include "meeting_sdk/storage/sodium_encryptor.hpp"

#include <sodium.h>

#include "sodium_runtime.hpp"

namespace meeting_sdk::storage {

namespace {

core::Error cryptoInitError() {
    return core::Error{
        .category = core::ErrorCategory::Security,
        .code = "storage.crypto_init_failed",
        .message = "libsodium failed to initialize; encryption is unavailable",
    };
}

}  // namespace

SodiumEncryptor::SodiumEncryptor() { static_cast<void>(detail::ensureSodiumInitialized()); }

std::size_t SodiumEncryptor::keyLength() const noexcept { return crypto_secretbox_KEYBYTES; }

core::Result<std::vector<std::uint8_t>> SodiumEncryptor::encrypt(const std::vector<std::uint8_t>& plaintext,
                                                                   const std::vector<std::uint8_t>& key) {
    if (!detail::ensureSodiumInitialized()) {
        return cryptoInitError();
    }
    if (key.size() != crypto_secretbox_KEYBYTES) {
        return core::Error{
            .category = core::ErrorCategory::Security,
            .code = "storage.invalid_key_length",
            .message = "encryption key must be crypto_secretbox_KEYBYTES bytes",
        };
    }

    std::vector<std::uint8_t> nonce(crypto_secretbox_NONCEBYTES);
    randombytes_buf(nonce.data(), nonce.size());

    std::vector<std::uint8_t> box(plaintext.size() + crypto_secretbox_MACBYTES);
    crypto_secretbox_easy(box.data(), plaintext.data(), plaintext.size(), nonce.data(), key.data());

    std::vector<std::uint8_t> output;
    output.reserve(nonce.size() + box.size());
    output.insert(output.end(), nonce.begin(), nonce.end());
    output.insert(output.end(), box.begin(), box.end());
    return output;
}

core::Result<std::vector<std::uint8_t>> SodiumEncryptor::decrypt(const std::vector<std::uint8_t>& ciphertext,
                                                                   const std::vector<std::uint8_t>& key) {
    if (!detail::ensureSodiumInitialized()) {
        return cryptoInitError();
    }
    if (key.size() != crypto_secretbox_KEYBYTES) {
        return core::Error{
            .category = core::ErrorCategory::Security,
            .code = "storage.invalid_key_length",
            .message = "encryption key must be crypto_secretbox_KEYBYTES bytes",
        };
    }
    if (ciphertext.size() < static_cast<std::size_t>(crypto_secretbox_NONCEBYTES) +
                                 static_cast<std::size_t>(crypto_secretbox_MACBYTES)) {
        return core::Error{
            .category = core::ErrorCategory::Security,
            .code = "storage.ciphertext_too_short",
            .message = "ciphertext is shorter than nonce + MAC overhead",
        };
    }

    const std::uint8_t* nonce = ciphertext.data();
    const std::uint8_t* box = ciphertext.data() + crypto_secretbox_NONCEBYTES;
    const std::size_t boxLen = ciphertext.size() - crypto_secretbox_NONCEBYTES;

    std::vector<std::uint8_t> plaintext(boxLen - crypto_secretbox_MACBYTES);
    if (crypto_secretbox_open_easy(plaintext.data(), box, boxLen, nonce, key.data()) != 0) {
        return core::Error{
            .category = core::ErrorCategory::Security,
            .code = "storage.decryption_failed",
            .message = "authentication failed: ciphertext is corrupt, tampered, or the wrong key was used",
        };
    }
    return plaintext;
}

}  // namespace meeting_sdk::storage
