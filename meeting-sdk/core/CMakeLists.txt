add_library(meeting_sdk_core STATIC
    src/errors.cpp
    src/domain.cpp
)
add_library(meeting_sdk::core ALIAS meeting_sdk_core)

target_include_directories(meeting_sdk_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(meeting_sdk_core PUBLIC cxx_std_20)
target_link_libraries(meeting_sdk_core PRIVATE meeting_sdk_compiler_options)
