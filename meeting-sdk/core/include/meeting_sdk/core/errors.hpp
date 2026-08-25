#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace meeting_sdk::core {

enum class ErrorCategory {
    Audio,
    Transcription,
    Model,
    Storage,
    Network,
    Permission,
    Configuration,
    Security,
    Cancellation,
};

std::string_view toString(ErrorCategory category) noexcept;

struct Error {
    ErrorCategory category;
    std::string code;     // stable, machine-readable, e.g. "audio.device_unavailable"
    std::string message;  // developer-facing; must never contain meeting content (see threat model)
    std::shared_ptr<Error> cause = nullptr;
};

// Hand-rolled Result<T>, not std::expected: the SDK targets C++20 and std::expected is
// a C++23 addition. Every domain/interface boundary returns this instead of throwing —
// exceptions don't propagate cleanly across the JNI/Obj-C++ boundary (see 02-interfaces-
// and-data-models.md §1).
template <typename T>
class [[nodiscard]] Result {
public:
    Result(T value) : storage_(std::move(value)) {}
    Result(Error error) : storage_(std::move(error)) {}

    bool has_value() const noexcept { return std::holds_alternative<T>(storage_); }
    explicit operator bool() const noexcept { return has_value(); }

    const T& value() const& { return std::get<T>(storage_); }
    T& value() & { return std::get<T>(storage_); }
    T&& value() && { return std::get<T>(std::move(storage_)); }

    const Error& error() const& { return std::get<Error>(storage_); }
    Error& error() & { return std::get<Error>(storage_); }
    Error&& error() && { return std::get<Error>(std::move(storage_)); }

private:
    std::variant<T, Error> storage_;
};

// Specialized for operations with no success payload (e.g. IAudioSource::stop()).
template <>
class [[nodiscard]] Result<void> {
public:
    Result() : error_(std::nullopt) {}
    Result(Error error) : error_(std::move(error)) {}

    bool has_value() const noexcept { return !error_.has_value(); }
    explicit operator bool() const noexcept { return has_value(); }

    const Error& error() const& { return *error_; }
    Error&& error() && { return std::move(*error_); }

private:
    std::optional<Error> error_;
};

}  // namespace meeting_sdk::core
