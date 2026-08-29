# Desktop links the system sqlite3/libsodium (present on macOS/Linux already, or via Homebrew —
# see README §Prerequisites). Neither is usable straight from a cross-compile sysroot — the
# Android NDK ships no sqlite3.h/libsodium at all, and the iOS SDK's libsqlite3 can't be relied
# on across arches/simulator — so those builds fetch and compile real source for both. Not a
# stub: the same libraries, just built from source instead of found on the host.
if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # sqlite3.c/libsodium are C sources; the top-level project() only enables CXX (no first-party
    # code here is C), so C must be turned on explicitly before compiling them.
    enable_language(C)
    include(FetchContent)

    FetchContent_Declare(sqlite3_amalgamation
        URL https://www.sqlite.org/2025/sqlite-amalgamation-3500100.zip
    )
    FetchContent_MakeAvailable(sqlite3_amalgamation)
    add_library(meeting_sdk_sqlite3 STATIC ${sqlite3_amalgamation_SOURCE_DIR}/sqlite3.c)
    target_include_directories(meeting_sdk_sqlite3 PUBLIC ${sqlite3_amalgamation_SOURCE_DIR})
    set(SQLITE3_LIBRARY meeting_sdk_sqlite3)

    set(SODIUM_DISABLE_TESTS ON CACHE BOOL "" FORCE)
    # Pinned for reproducible cross-compiles — this is robinlinden/libsodium-cmake master as of
    # 2026-08-29 (it wraps libsodium 1.0.20 via its own submodule pin).
    FetchContent_Declare(libsodium_cmake
        GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
        GIT_TAG 9b2848dfc1b917a9410f0de9d81059b26cbfaa8d
    )
    FetchContent_MakeAvailable(libsodium_cmake)
    set(SODIUM_LIBRARY sodium)
    set(SODIUM_INCLUDE_DIR "")  # sodium target already exports its own public include dir
else()
    find_library(SQLITE3_LIBRARY sqlite3 REQUIRED)

    find_path(SODIUM_INCLUDE_DIR sodium.h HINTS /opt/homebrew/opt/libsodium/include /usr/local/opt/libsodium/include)
    find_library(SODIUM_LIBRARY sodium HINTS /opt/homebrew/opt/libsodium/lib /usr/local/opt/libsodium/lib)
    if(NOT SODIUM_INCLUDE_DIR OR NOT SODIUM_LIBRARY)
        message(FATAL_ERROR "libsodium not found — install it (e.g. 'brew install libsodium' on macOS).")
    endif()
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
