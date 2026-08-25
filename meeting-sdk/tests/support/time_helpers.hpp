#pragma once

#include <chrono>

#include "meeting_sdk/core/types.hpp"

namespace meeting_sdk::test_support {

inline core::Timestamp ts(long long millisFromEpoch) {
    return core::Timestamp{std::chrono::system_clock::time_point{} +
                            std::chrono::milliseconds(millisFromEpoch)};
}

}  // namespace meeting_sdk::test_support
