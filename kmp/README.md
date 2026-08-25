# kmp

Kotlin Multiplatform API over the C++ core, per product spec §19: platform-neutral models and
service interfaces in `commonMain`, no native types (pointers, JNI handles) leaking across the
boundary.

## Layout

- `src/commonMain/kotlin/com/meetingsdk/model/` — `Meeting`, `TranscriptSegment`, `MeetingState`,
  `ProviderMode`. Plain data classes; a subset of `meeting_sdk::core` matching what
  `bindings/c` exposes today (see that module's README) — extended, not reshaped, as the C API
  grows.
- `src/commonMain/kotlin/com/meetingsdk/api/` — `TranslationService`, `MeetingRepository`
  interfaces. Plain Kotlin interfaces, not `expect`/`actual`: with only a JVM target so far,
  per-target concrete classes are enough, and this shape extends cleanly to `androidMain`/
  `iosMain` in Milestones 9-10 without changing `commonMain`.
- `src/jvmMain/kotlin/com/meetingsdk/jvm/` — `NativeBridge` (the JNI `external fun` declarations
  and `System.loadLibrary` call) plus `JvmTranslationService`/`JvmMeetingRepository`, the actual
  implementations of the `commonMain` interfaces for this target.
- `src/jvmTest/` — end-to-end integration tests: Kotlin → JNI → C API → C++ core, for real, not
  mocked at any layer.

Only the `jvm()` target is built here. `androidTarget()` and `iosArm64()`/
`iosSimulatorArm64()` are added in Milestones 9 and 10 respectively, alongside the
platform-specific capture/permissions/lifecycle work those milestones own.

## Build

Native artifacts are built via CMake first (Gradle does not invoke CMake automatically — that
integration is Milestone 11/CI polish, not required for this to work today):

```sh
cd ../bindings/kotlin/jni
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

cd ../../../kmp
export JAVA_HOME=/opt/homebrew/opt/openjdk@21   # or wherever your JDK 17+ lives
gradle jvmTest
```

A known, non-blocking warning: the Kotlin Multiplatform Gradle plugin (2.1.20) still declares a
legacy `Usage` attribute Gradle 9.7 has deprecated ahead of Gradle 10 — this is upstream, not
fixable from this build script; track it via a Kotlin Gradle Plugin version bump when one lands.
