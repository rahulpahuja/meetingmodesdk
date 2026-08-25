add_library(meeting_sdk_search STATIC
    src/inverted_index_search.cpp
)
add_library(meeting_sdk::search ALIAS meeting_sdk_search)

target_include_directories(meeting_sdk_search PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(meeting_sdk_search PUBLIC cxx_std_20)
target_link_libraries(meeting_sdk_search
    PUBLIC meeting_sdk::core
    PRIVATE meeting_sdk_compiler_options
)
