# Milestone 11 — hardening review

A focused pass over memory safety, the crypto at rest, the C ABI boundary, and the
storage query surface. Companion to `../architecture/03-threat-model-and-security.md`
(design-level); this records what was actually checked and run.

Date: 2026-08-29. Scope: `meeting-sdk/`, `bindings/c/`.

## Memory safety — PASS

Full GoogleTest suite (138 tests) built and run under **AddressSanitizer +
UndefinedBehaviorSanitizer** (`-DMEETING_SDK_ENABLE_ASAN=ON`): 138/138 pass, no ASan
reports, no UBSan runtime errors. Wired into CI as the `cpp-sanitizers` job so it
can't regress. (LeakSanitizer runs on the Linux CI job; it is unsupported on macOS
and simply not requested there.)

## C ABI boundary — FIXED

**Finding:** `bindings/c/src/meeting_sdk.cpp` had no exception handling. The header
promised "no C++ exception … ever crosses this header" but nothing enforced it — a
`std::bad_alloc` (or anything else the core threw despite its `Result<T>` contract)
would unwind through `extern "C"` into Kotlin/JNI/Swift, which is undefined
behaviour.

**Fix:** every entry point now runs its body through a `guarded(...)` /
`guardedVoid(...)` wrapper that catches `...` and maps it to the new
`MSDK_ERROR_INTERNAL` (`-6`) — or silently absorbs it for the `void` destructors.
No exception can reach a caller; the library stays usable after one.

## Storage query surface — PASS

`SqliteMeetingRepository` uses only `sqlite3_prepare_v2` + `sqlite3_bind_*` with `?`
placeholders (`INSERT`, `SELECT … WHERE id = ?`, `DELETE … WHERE id = ?`, `SELECT id
… ORDER BY id`). No string-concatenated SQL; meeting ids and ciphertext are always
bound, never interpolated.

## Crypto at rest — PASS (one low-severity note)

`SodiumEncryptor` uses libsodium `crypto_secretbox_easy` (XSalsa20-Poly1305 AEAD).
Each record gets a fresh 24-byte nonce from `randombytes_buf`, prepended to the
ciphertext — no nonce reuse. Key length is validated on both encrypt and decrypt;
decrypt rejects anything shorter than nonce + MAC and returns an error (never
partial plaintext) on authentication failure.

*Low-severity:* `sodium_init()`'s return value is discarded in `SodiumEncryptor` and
`InMemoryKeyProvider`. A `-1` (init failure) is not surfaced. In practice
`randombytes_buf` is safe pre-init on the supported platforms; worth returning a
`Result` error here in a future pass.

## Reproducibility — FIXED

`storage/CMakeLists.txt` fetched `robinlinden/libsodium-cmake` at `GIT_TAG master`
for the Android and iOS builds — a moving target. Now pinned to an explicit commit.

## Not in this pass

Performance/memory profiling, fuzzing of the JSON writer and the C ABI string
inputs, crash-reporting integration, and release packaging (Maven / SPM / AAR).
