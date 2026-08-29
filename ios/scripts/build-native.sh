#!/usr/bin/env bash
# Cross-compiles the meeting-sdk C++ core + its C ABI (bindings/c) as static libraries for
# Apple platforms, then bundles every resulting archive into one libMeetingSdkNative.a per SDK.
# Kotlin/Native's iOS targets link that single archive into MeetingSdkKit.framework.
#
# Output:
#   ios/native/iphonesimulator/libMeetingSdkNative.a   (arm64 simulator)
#   ios/native/iphoneos/libMeetingSdkNative.a          (arm64 device)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEPLOYMENT_TARGET="15.0"

build_sdk() {
  local sdk="$1"          # iphonesimulator | iphoneos
  local build_dir="$ROOT/ios/build/native-$sdk"
  local out_dir="$ROOT/ios/native/$sdk"

  echo ">>> configuring bindings/c for $sdk (arm64)"
  rm -rf "$build_dir"
  cmake -S "$ROOT/bindings/c" -B "$build_dir" -G "Unix Makefiles" \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_SYSROOT="$sdk" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOYMENT_TARGET" \
    -DCMAKE_BUILD_TYPE=Release \
    -DMEETING_SDK_C_SHARED=OFF \
    -DMEETING_SDK_BUILD_TESTS=OFF \
    -DMEETING_SDK_WARNINGS_AS_ERRORS=OFF

  echo ">>> building $sdk"
  cmake --build "$build_dir" -j"$(sysctl -n hw.ncpu)"

  mkdir -p "$out_dir"
  # Every static archive the build produced: our modules, the C ABI, plus the fetched
  # sqlite3 amalgamation and libsodium.
  local archives
  archives=$(find "$build_dir" -name '*.a' -type f | sort -u)
  echo ">>> bundling into $out_dir/libMeetingSdkNative.a"
  echo "$archives" | sed "s|$build_dir/||"
  libtool -static -o "$out_dir/libMeetingSdkNative.a" $archives 2>/dev/null
  echo ">>> $(lipo -info "$out_dir/libMeetingSdkNative.a")"
}

build_sdk iphonesimulator
build_sdk iphoneos
echo ">>> native build complete"
