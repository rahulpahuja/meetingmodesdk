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

**Fixed:** `sodium_init()`'s return value used to be discarded in `SodiumEncryptor`
and `InMemoryKeyProvider`, so a `-1` (init failure) went unnoticed. Initialization
is now funnelled through `storage/src/sodium_runtime.cpp`'s
`detail::ensureSodiumInitialized()` (a thread-safe function-local static), and
`encrypt` / `decrypt` / `getOrCreateKey` return
`Security / storage.crypto_init_failed` instead of touching the CSPRNG when it
reports failure.

## Reproducibility — FIXED

`storage/CMakeLists.txt` fetched `robinlinden/libsodium-cmake` at `GIT_TAG master`
for the Android and iOS builds — a moving target. Now pinned to an explicit commit.

## Fuzzing — ADDED

`bindings/c/fuzz/` drives arbitrary bytes through the whole C ABI (translate,
`escapeJson` / `meetingToJson`, the encrypt → SQLite → decrypt → deserialize round
trip, id listing, inverted-index search). Two build targets behind
`-DMEETING_SDK_BUILD_FUZZERS=ON`:

- `fuzz_c_abi_replay` — portable ASan/UBSan corpus + crash-file replayer, no
  libFuzzer runtime required (Apple Clang ships none). Ran clean over empty,
  control-char, quote-/backslash-heavy, 200 KB random, and word-dense inputs.
- `fuzz_c_abi` — coverage-guided libFuzzer, built only where the toolchain provides
  the runtime (Linux Clang / `brew install llvm`); for CI and longer campaigns.

A small seed corpus lives in `bindings/c/fuzz/corpus/`.

## Packaging — PARTIAL

- **KMP** — `kmp/build.gradle.kts` applies `maven-publish` (group `com.meetingsdk`,
  version `0.1.0`); `gradle publishToMavenLocal` produces the multiplatform module
  + per-target artifacts, and a `GitHubPackages` repo is wired for `publish`.
- **Apple** — root `Package.swift` exposes `MeetingSdkKit` as an SPM `binaryTarget`
  pointing at the built XCFramework (`swift package describe` validates it).
- **Android AAR** — not done; the app still builds the JNI/core via its own
  `externalNativeBuild`.

## Not in this pass

Performance/memory profiling, a coverage-guided fuzz campaign of meaningful length
in CI, crash-reporting integration, an Android AAR, and publishing signed releases.
