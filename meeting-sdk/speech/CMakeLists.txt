add_library(meeting_sdk_speech STATIC
    src/segmenter.cpp
    src/streaming_stt_session.cpp
    src/heuristic_language_detector.cpp
)
add_library(meeting_sdk::speech ALIAS meeting_sdk_speech)

target_include_directories(meeting_sdk_speech PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(meeting_sdk_speech PUBLIC cxx_std_20)
target_link_libraries(meeting_sdk_speech
    PUBLIC meeting_sdk::core
    PRIVATE meeting_sdk_compiler_options
)
