# bindings/c

The stable C API boundary between the C++ core (`../../meeting-sdk`) and every native binding
(JNI, Kotlin/Native cinterop, Objective-C++) — see `../../docs/architecture/
01-requirements-and-architecture.md` §2 for why a single C API was chosen over separate
hand-maintained JNI/Obj-C++ bridges. No C++ exception, class, or STL type crosses
`include/meeting_sdk_c/meeting_sdk.h`; every function returns an `int32_t` error code and
opaque handles are plain pointers.

## Scope (Milestone 8)

Exposes exactly the C++ functionality that is genuinely complete and model-independent today:
on-device translation and the meeting repository. Audio capture, STT, and diarization are not
exposed here yet — those interfaces exist in the C++ core but have no real (non-model) provider
behind them until Milestones 9-11, and exposing a non-functional surface would be misleading to
a native-binding consumer.

`msdk_repository_save_simple_meeting` creates a single-segment meeting from plain text rather
than a full structured `Meeting` — without real capture/STT wired up, that's the only way to
honestly produce meeting content right now. A richer creation API grows alongside real capture.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

This project pulls in `../../meeting-sdk` via `add_subdirectory` — it is not part of that
tree's own module graph (see the architecture doc), just a consumer of it.
