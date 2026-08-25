#include "meeting_sdk/storage/sodium_encryptor.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::storage {
namespace {

std::vector<std::uint8_t> makeKey(SodiumEncryptor& enc, std::uint8_t fill = 0x42) {
    return std::vector<std::uint8_t>(enc.keyLength(), fill);
}

TEST(SodiumEncryptor, RoundTripsPlaintext) {
    SodiumEncryptor enc;
    const std::vector<std::uint8_t> key = makeKey(enc);
    const std::vector<std::uint8_t> plaintext = {'h', 'e', 'l', 'l', 'o'};

    auto ciphertext = enc.encrypt(plaintext, key);
    ASSERT_TRUE(ciphertext);
    EXPECT_NE(ciphertext.value(), plaintext);  // must not look like plaintext

    auto decrypted = enc.decrypt(ciphertext.value(), key);
    ASSERT_TRUE(decrypted);
    EXPECT_EQ(decrypted.value(), plaintext);
}

TEST(SodiumEncryptor, RoundTripsEmptyPlaintext) {
    SodiumEncryptor enc;
    const std::vector<std::uint8_t> key = makeKey(enc);
    auto ciphertext = enc.encrypt({}, key);
    ASSERT_TRUE(ciphertext);
    auto decrypted = enc.decrypt(ciphertext.value(), key);
    ASSERT_TRUE(decrypted);
    EXPECT_TRUE(decrypted.value().empty());
}

TEST(SodiumEncryptor, TamperedCiphertextFailsAuthentication) {
    SodiumEncryptor enc;
    const std::vector<std::uint8_t> key = makeKey(enc);
    auto ciphertext = enc.encrypt({'s', 'e', 'c', 'r', 'e', 't'}, key);
    ASSERT_TRUE(ciphertext);

    auto tampered = ciphertext.value();
    tampered.back() ^= 0xFF;

    auto decrypted = enc.decrypt(tampered, key);
    ASSERT_FALSE(decrypted);
    EXPECT_EQ(decrypted.error().code, "storage.decryption_failed");
}

TEST(SodiumEncryptor, WrongKeyFailsAuthentication) {
    SodiumEncryptor enc;
    auto ciphertext = enc.encrypt({'s', 'e', 'c', 'r', 'e', 't'}, makeKey(enc, 0x01));
    ASSERT_TRUE(ciphertext);

    auto decrypted = enc.decrypt(ciphertext.value(), makeKey(enc, 0x02));
    ASSERT_FALSE(decrypted);
}

TEST(SodiumEncryptor, RejectsWrongKeyLength) {
    SodiumEncryptor enc;
    std::vector<std::uint8_t> shortKey(4, 0x00);
    auto result = enc.encrypt({'x'}, shortKey);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, "storage.invalid_key_length");
}

}  // namespace
}  // namespace meeting_sdk::storage
