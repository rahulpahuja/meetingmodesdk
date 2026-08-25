add_library(meeting_sdk_translation STATIC
    src/gated_translator.cpp
    src/dictionary_translator.cpp
)
add_library(meeting_sdk::translation ALIAS meeting_sdk_translation)

target_include_directories(meeting_sdk_translation PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(meeting_sdk_translation PUBLIC cxx_std_20)
target_link_libraries(meeting_sdk_translation
    PUBLIC meeting_sdk::core
    PRIVATE meeting_sdk_compiler_options
)
