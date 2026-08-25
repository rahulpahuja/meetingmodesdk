cmake_minimum_required(VERSION 3.24)
project(meeting_sdk_c_bindings LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# meeting-sdk builds fully standalone; bindings/c depends on it but lives outside its module
# tree, per docs/architecture/01-requirements-and-architecture.md §3.
set(MEETING_SDK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../meeting-sdk ${CMAKE_BINARY_DIR}/meeting-sdk)

add_library(meeting_sdk_c SHARED src/meeting_sdk.cpp)
add_library(meeting_sdk::c ALIAS meeting_sdk_c)

target_include_directories(meeting_sdk_c PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(meeting_sdk_c PRIVATE
    meeting_sdk::core
    meeting_sdk::storage
    meeting_sdk::translation
    meeting_sdk_compiler_options
)

add_executable(meeting_sdk_c_smoke_test test/smoke_test.cpp)
target_link_libraries(meeting_sdk_c_smoke_test PRIVATE meeting_sdk::c)

enable_testing()
add_test(NAME smoke_test COMMAND meeting_sdk_c_smoke_test)
