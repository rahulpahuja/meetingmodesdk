# bindings/kotlin

`jni/` is a thin JNI shim: it declares the `Java_com_meetingsdk_jvm_NativeBridge_*` symbols the
JVM resolves when `../../kmp` calls `System.loadLibrary("meeting_sdk_jni")`, and forwards every
call 1:1 into `../c/include/meeting_sdk_c/meeting_sdk.h`. No business logic lives here — see
product spec §20 ("JNI should be thin").

This is JVM-target JNI, used to verify the native-binding pattern end-to-end in Milestone 8.
Android-specific JNI (same header, same shim shape, plus audio capture/permissions/foreground
service) is Milestone 9.

## Build

```sh
cd jni
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

Requires `JAVA_HOME` set (used to locate `jni.h`/`jni_md.h` — not CMake's `find_package(JNI)`,
which pulls in the full embedding API this library doesn't need; see comments in
`jni/CMakeLists.txt`). Produces `jni/build/libmeeting_sdk_jni.dylib` (or `.so`), which
`../../kmp`'s tests load via `java.library.path`.
