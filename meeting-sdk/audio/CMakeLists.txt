add_library(meeting_sdk_audio STATIC
    src/preprocessor.cpp
    src/energy_vad.cpp
    src/synthetic_audio_source.cpp
    src/null_audio_source.cpp
)
add_library(meeting_sdk::audio ALIAS meeting_sdk_audio)

target_include_directories(meeting_sdk_audio PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(meeting_sdk_audio PUBLIC cxx_std_20)
target_link_libraries(meeting_sdk_audio
    PUBLIC meeting_sdk::core
    PRIVATE meeting_sdk_compiler_options
)
