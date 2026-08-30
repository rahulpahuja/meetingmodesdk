cmake_minimum_required(VERSION 3.24)
project(meeting_sdk_c_bindings LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# meeting-sdk builds fully standalone; bindings/c depends on it but lives outside its module
# tree, per docs/architecture/01-requirements-and-architecture.md §3.
set(MEETING_SDK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../meeting-sdk ${CMAKE_BINARY_DIR}/meeting-sdk)

# SHARED for the JVM/JNI host (dlopen'd at runtime); STATIC for Apple, where Kotlin/Native
# links the C ABI straight into the framework binary. Defaults to SHARED so existing desktop
# and Android consumers are unaffected.
option(MEETING_SDK_C_SHARED "Build meeting_sdk_c as a shared library" ON)
if(MEETING_SDK_C_SHARED)
    add_library(meeting_sdk_c SHARED src/meeting_sdk.cpp)
else()
    add_library(meeting_sdk_c STATIC src/meeting_sdk.cpp)
endif()
add_library(meeting_sdk::c ALIAS meeting_sdk_c)

target_include_directories(meeting_sdk_c PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(meeting_sdk_c PRIVATE
    meeting_sdk::core
    meeting_sdk::audio
    meeting_sdk::speech
    meeting_sdk::intelligence
    meeting_sdk::search
    meeting_sdk::speaker
    meeting_sdk::storage
    meeting_sdk::translation
    meeting_sdk::orchestration
    meeting_sdk_compiler_options
)

# The smoke test is a host executable — skip it when cross-compiling for a device we can't run on.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_executable(meeting_sdk_c_smoke_test test/smoke_test.cpp)
    target_link_libraries(meeting_sdk_c_smoke_test PRIVATE meeting_sdk::c)

    enable_testing()
    add_test(NAME smoke_test COMMAND meeting_sdk_c_smoke_test)
endif()

# Fuzzing the C ABI. Enable with -DMEETING_SDK_BUILD_FUZZERS=ON and, for a meaningful run, also
# -DMEETING_SDK_C_SHARED=OFF -DMEETING_SDK_ENABLE_ASAN=ON so the whole core is instrumented and
# linked in statically.
#   * fuzz_c_abi_replay — portable: replays a corpus / crash file under ASan+UBSan, no libFuzzer
#     runtime needed, builds on any toolchain (incl. Apple Clang, which ships no fuzzer runtime).
#   * fuzz_c_abi        — coverage-guided libFuzzer; only when the toolchain provides its runtime.
option(MEETING_SDK_BUILD_FUZZERS "Build the C ABI fuzz harness" OFF)
if(MEETING_SDK_BUILD_FUZZERS)
    add_executable(fuzz_c_abi_replay fuzz/fuzz_c_abi.cpp fuzz/standalone_main.cpp)
    target_link_libraries(fuzz_c_abi_replay PRIVATE meeting_sdk::c meeting_sdk_compiler_options)

    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=fuzzer")
    check_cxx_source_compiles(
        "extern \"C\" int LLVMFuzzerTestOneInput(const unsigned char*, long){return 0;}"
        MEETING_SDK_HAVE_LIBFUZZER)
    unset(CMAKE_REQUIRED_FLAGS)
    if(MEETING_SDK_HAVE_LIBFUZZER)
        add_executable(fuzz_c_abi fuzz/fuzz_c_abi.cpp)
        target_link_libraries(fuzz_c_abi PRIVATE meeting_sdk::c meeting_sdk_compiler_options)
        target_compile_options(fuzz_c_abi PRIVATE -fsanitize=fuzzer-no-link)
        target_link_options(fuzz_c_abi PRIVATE -fsanitize=fuzzer)
    else()
        message(STATUS "libFuzzer runtime unavailable; building fuzz_c_abi_replay only")
    endif()
endif()
