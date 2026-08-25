add_library(meeting_sdk_orchestration STATIC
    src/meeting_state_machine.cpp
    src/transcript_assembler.cpp
    src/meeting_pipeline.cpp
)
add_library(meeting_sdk::orchestration ALIAS meeting_sdk_orchestration)

target_include_directories(meeting_sdk_orchestration PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(meeting_sdk_orchestration PUBLIC cxx_std_20)
target_link_libraries(meeting_sdk_orchestration
    PUBLIC
        meeting_sdk::core
        meeting_sdk::audio
        meeting_sdk::speech
    PRIVATE
        meeting_sdk::speaker
        meeting_sdk_compiler_options
)
