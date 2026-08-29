# iOS consumer — Milestone 10

A SwiftUI app that drives the meeting-sdk C++ core through the shared KMP module
(`../kmp`), exactly as the Android app drives it over JNI. The Swift ⇄ Kotlin/Native
⇄ C ABI (`../bindings/c/meeting_sdk.h`) ⇄ C++ core path carries no business logic on
the Swift side — `ContentView.swift` just calls `NativeMeetingRepository` /
`NativeSearchService` and the `translateOnDeviceOrNull` helper.

## Build

Requires Xcode (iOS SDK), CMake, `xcodegen` (`brew install xcodegen`), and a JDK for
the Gradle build.

```sh
# 1. cross-compile the C++ core + C ABI + sqlite3 + libsodium as static libs for
#    device + simulator (arm64), bundled per-SDK into ios/native/<sdk>/libMeetingSdkNative.a
./scripts/build-native.sh

# 2. assemble MeetingSdkKit.xcframework (Kotlin/Native links the static lib above
#    into each slice) -> kmp/build/XCFrameworks/release/MeetingSdkKit.xcframework
(cd ../kmp && gradle assembleMeetingSdkKitReleaseXCFramework)

# 3. generate the Xcode project and build/run
xcodegen generate
xcodebuild -project MeetingSDKDemo.xcodeproj -scheme MeetingSDKDemo \
  -sdk iphonesimulator -destination 'platform=iOS Simulator,name=iPhone 17' build
```

`project.yml`, `scripts/`, and `MeetingSDKDemo/` are checked in; `MeetingSDKDemo.xcodeproj`,
`native/`, and `build/` are generated and git-ignored.

## Smoke test

Launch with `-autorun` to exercise the demo pipeline, search, and translation on
startup without any taps:

```sh
xcrun simctl launch booted com.meetingsdk.demo -autorun
```
