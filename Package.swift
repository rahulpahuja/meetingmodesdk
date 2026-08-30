// swift-tools-version:5.9
import PackageDescription

// Swift Package exposing the shared meeting-sdk module to Apple consumers. The binary is the
// XCFramework produced by `(cd kmp && gradle assembleMeetingSdkKitReleaseXCFramework)`, which in
// turn statically links the C++ core cross-compiled by `ios/scripts/build-native.sh`. Build
// those first; then `.package(path: "…/meetingmodesdk")` or drag this package into Xcode.
let package = Package(
    name: "MeetingSdkKit",
    platforms: [.iOS(.v16)],
    products: [
        .library(name: "MeetingSdkKit", targets: ["MeetingSdkKit"])
    ],
    targets: [
        .binaryTarget(
            name: "MeetingSdkKit",
            path: "kmp/build/XCFrameworks/release/MeetingSdkKit.xcframework"
        )
    ]
)
