#include "sodium_runtime.hpp"

#include <sodium.h>

namespace meeting_sdk::storage::detail {

bool ensureSodiumInitialized() noexcept {
    // A function-local static is initialized exactly once even under concurrent first calls
    // (C++11 [stmt.dcl.dcl]/4), which also honours libsodium's rule that sodium_init() must
    // not run reentrantly. 0 = first init, 1 = already initialized, -1 = failure.
    static const bool ok = sodium_init() >= 0;
    return ok;
}

}  // namespace meeting_sdk::storage::detail
