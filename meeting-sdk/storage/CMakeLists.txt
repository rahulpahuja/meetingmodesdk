find_library(SQLITE3_LIBRARY sqlite3 REQUIRED)

find_path(SODIUM_INCLUDE_DIR sodium.h HINTS /opt/homebrew/opt/libsodium/include /usr/local/opt/libsodium/include)
find_library(SODIUM_LIBRARY sodium HINTS /opt/homebrew/opt/libsodium/lib /usr/local/opt/libsodium/lib)
if(NOT SODIUM_INCLUDE_DIR OR NOT SODIUM_LIBRARY)
    message(FATAL_ERROR "libsodium not found — install it (e.g. 'brew install libsodium' on macOS).")
endif()

add_library(meeting_sdk_storage STATIC
    src/in_memory_key_provider.cpp
    src/sodium_encryptor.cpp
    src/meeting_serializer.cpp
    src/sqlite_meeting_repository.cpp
)
add_library(meeting_sdk::storage ALIAS meeting_sdk_storage)

target_include_directories(meeting_sdk_storage
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${SODIUM_INCLUDE_DIR}
)
target_compile_features(meeting_sdk_storage PUBLIC cxx_std_20)
target_link_libraries(meeting_sdk_storage
    PUBLIC meeting_sdk::core
    PRIVATE meeting_sdk_compiler_options ${SQLITE3_LIBRARY} ${SODIUM_LIBRARY}
)
