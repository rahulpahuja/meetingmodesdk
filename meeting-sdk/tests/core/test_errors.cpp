#include "meeting_sdk/core/errors.hpp"

#include <gtest/gtest.h>

namespace meeting_sdk::core {
namespace {

TEST(Result, HoldsValueOnSuccess) {
    Result<int> r(42);
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(static_cast<bool>(r));
    EXPECT_EQ(r.value(), 42);
}

TEST(Result, HoldsErrorOnFailure) {
    Result<int> r(Error{
        .category = ErrorCategory::Audio,
        .code = "audio.device_unavailable",
        .message = "no input device",
    });
    ASSERT_FALSE(r.has_value());
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(r.error().category, ErrorCategory::Audio);
    EXPECT_EQ(r.error().code, "audio.device_unavailable");
}

TEST(ResultVoid, SuccessHasNoError) {
    Result<void> r;
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(static_cast<bool>(r));
}

TEST(ResultVoid, FailureCarriesError) {
    Result<void> r(Error{
        .category = ErrorCategory::Storage,
        .code = "storage.write_failed",
        .message = "disk full",
    });
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, "storage.write_failed");
}

TEST(Error, CauseChainIsOptionalAndWalkable) {
    auto root = std::make_shared<Error>(Error{
        .category = ErrorCategory::Network,
        .code = "network.timeout",
        .message = "upstream timed out",
    });
    Error wrapped{
        .category = ErrorCategory::Transcription,
        .code = "transcription.cloud_call_failed",
        .message = "cloud STT call failed",
        .cause = root,
    };
    ASSERT_NE(wrapped.cause, nullptr);
    EXPECT_EQ(wrapped.cause->code, "network.timeout");
}

TEST(ErrorCategory, ToStringCoversEveryEnumerator) {
    EXPECT_EQ(toString(ErrorCategory::Audio), "Audio");
    EXPECT_EQ(toString(ErrorCategory::Transcription), "Transcription");
    EXPECT_EQ(toString(ErrorCategory::Model), "Model");
    EXPECT_EQ(toString(ErrorCategory::Storage), "Storage");
    EXPECT_EQ(toString(ErrorCategory::Network), "Network");
    EXPECT_EQ(toString(ErrorCategory::Permission), "Permission");
    EXPECT_EQ(toString(ErrorCategory::Configuration), "Configuration");
    EXPECT_EQ(toString(ErrorCategory::Security), "Security");
    EXPECT_EQ(toString(ErrorCategory::Cancellation), "Cancellation");
}

}  // namespace
}  // namespace meeting_sdk::core
