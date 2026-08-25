add_library(meeting_sdk_speaker STATIC
    src/embedding_similarity.cpp
    src/speaker_clusterer.cpp
    src/enrolled_speaker_identifier.cpp
    src/speaker_assigner.cpp
)
add_library(meeting_sdk::speaker ALIAS meeting_sdk_speaker)

target_include_directories(meeting_sdk_speaker PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(meeting_sdk_speaker PUBLIC cxx_std_20)
target_link_libraries(meeting_sdk_speaker
    PUBLIC meeting_sdk::core
    PRIVATE meeting_sdk_compiler_options
)
