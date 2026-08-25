add_library(meeting_sdk_intelligence STATIC
    src/word_frequency.cpp
    src/sentence_splitter.cpp
    src/heuristic_question_extractor.cpp
    src/heuristic_decision_extractor.cpp
    src/heuristic_action_item_extractor.cpp
    src/keyword_topic_extractor.cpp
    src/extractive_summarizer.cpp
    src/heuristic_llm_engine.cpp
)
add_library(meeting_sdk::intelligence ALIAS meeting_sdk_intelligence)

target_include_directories(meeting_sdk_intelligence PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(meeting_sdk_intelligence PUBLIC cxx_std_20)
target_link_libraries(meeting_sdk_intelligence
    PUBLIC meeting_sdk::core
    PRIVATE meeting_sdk_compiler_options
)
