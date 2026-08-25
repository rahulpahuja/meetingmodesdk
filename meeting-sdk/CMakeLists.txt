cmake_minimum_required(VERSION 3.24)
project(meeting_sdk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MEETING_SDK_BUILD_TESTS "Build unit tests" ON)
option(MEETING_SDK_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(MEETING_SDK_ENABLE_ASAN "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
option(MEETING_SDK_ENABLE_TSAN "Build with ThreadSanitizer" OFF)

if(MEETING_SDK_ENABLE_ASAN AND MEETING_SDK_ENABLE_TSAN)
    message(FATAL_ERROR "MEETING_SDK_ENABLE_ASAN and MEETING_SDK_ENABLE_TSAN are mutually exclusive")
endif()

add_library(meeting_sdk_compiler_options INTERFACE)
target_compile_options(meeting_sdk_compiler_options INTERFACE
    -Wall -Wextra -Wpedantic
    $<$<BOOL:${MEETING_SDK_WARNINGS_AS_ERRORS}>:-Werror>
)
if(MEETING_SDK_ENABLE_ASAN)
    target_compile_options(meeting_sdk_compiler_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(meeting_sdk_compiler_options INTERFACE -fsanitize=address,undefined)
endif()
if(MEETING_SDK_ENABLE_TSAN)
    target_compile_options(meeting_sdk_compiler_options INTERFACE -fsanitize=thread)
    target_link_options(meeting_sdk_compiler_options INTERFACE -fsanitize=thread)
endif()

add_subdirectory(core)
add_subdirectory(audio)
add_subdirectory(speech)
add_subdirectory(speaker)
add_subdirectory(translation)
add_subdirectory(intelligence)
add_subdirectory(storage)
add_subdirectory(search)
add_subdirectory(orchestration)

if(MEETING_SDK_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()
