cmake_minimum_required(VERSION 3.24)
project(meeting_sdk_jni_bindings LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(MEETING_SDK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../c ${CMAKE_BINARY_DIR}/bindings_c)

# This library is dlopen'd by an already-running JVM/ART (via System.loadLibrary), which
# resolves JNI symbols itself — it is never linked against libjvm. Only the jni.h/jni_md.h
# headers are needed, not CMake's find_package(JNI) (which requires the full embedding API:
# libjvm, AWT).
#
# On Android, the NDK toolchain file already puts jni.h on the sysroot include path for every
# target it configures — there is no JAVA_HOME to locate it from (a device has no desktop JDK),
# and no jni_md.h to find (Android's jni.h has no platform-specific companion header).
if(NOT ANDROID)
    if(NOT DEFINED ENV{JAVA_HOME})
        message(FATAL_ERROR "JAVA_HOME is not set — required to locate jni.h")
    endif()
    find_path(JNI_HEADER_DIR jni.h
        HINTS "$ENV{JAVA_HOME}/include" "$ENV{JAVA_HOME}/include/darwin" "$ENV{JAVA_HOME}/include/linux"
    )
    find_path(JNI_MD_HEADER_DIR jni_md.h
        HINTS "$ENV{JAVA_HOME}/include" "$ENV{JAVA_HOME}/include/darwin" "$ENV{JAVA_HOME}/include/linux"
    )
    if(NOT JNI_HEADER_DIR OR NOT JNI_MD_HEADER_DIR)
        message(FATAL_ERROR "jni.h/jni_md.h not found under $ENV{JAVA_HOME}/include")
    endif()
endif()

add_library(meeting_sdk_jni SHARED src/meeting_sdk_jni.cpp)
if(NOT ANDROID)
    target_include_directories(meeting_sdk_jni PRIVATE ${JNI_HEADER_DIR} ${JNI_MD_HEADER_DIR})
endif()
target_link_libraries(meeting_sdk_jni PRIVATE meeting_sdk::c)

# Kotlin's System.loadLibrary("meeting_sdk_jni") searches java.library.path for this name.
set_target_properties(meeting_sdk_jni PROPERTIES OUTPUT_NAME "meeting_sdk_jni")
