cmake_minimum_required(VERSION 3.25)
project(fh2recomp_android LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Runtime lives in recomp/runtime (prompt structure); JNI is a thin shell.
set(FH2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
set(FH2_RUNTIME_DIR "${FH2_ROOT}/recomp/runtime")
set(FH2_GEN_DIR "${FH2_ROOT}/recomp/generated")

# Generated recompiled PPC code is optional at APK-build time: the user
# supplies game files at runtime, and CI may skip codegen without PAT.
# If present, generated/fh2_sources.cmake is included and linked.
if(EXISTS "${FH2_GEN_DIR}/fh2_sources.cmake")
    include("${FH2_GEN_DIR}/fh2_sources.cmake")
    message(STATUS "FH2: using generated sources from ${FH2_GEN_DIR}")
else()
    set(FH2_GENERATED_SOURCES "")
    message(STATUS "FH2: no generated sources (boot/runtime-only build)")
endif()

add_library(fh2recomp SHARED
    fh2_jni.cpp
    ${FH2_RUNTIME_DIR}/memory.cpp
    ${FH2_RUNTIME_DIR}/filesystem.cpp
    ${FH2_RUNTIME_DIR}/input.cpp
    ${FH2_RUNTIME_DIR}/shader_cache.cpp
    ${FH2_RUNTIME_DIR}/renderer.cpp
    ${FH2_RUNTIME_DIR}/audio.cpp
    ${FH2_RUNTIME_DIR}/streaming.cpp
    ${FH2_RUNTIME_DIR}/d3d9_translator.cpp
    ${FH2_RUNTIME_DIR}/xenos_shader.cpp
    ${FH2_GENERATED_SOURCES}
)

target_include_directories(fh2recomp PRIVATE
    ${FH2_RUNTIME_DIR}
    ${FH2_GEN_DIR}
)

find_library(LOG_LIB log)
find_library(ANDROID_LIB android)
find_library(EGL_LIB EGL)
find_library(GLES_LIB GLESv3)
find_library(AAUDIO_LIB aaudio)

target_link_libraries(fh2recomp PRIVATE
    ${LOG_LIB}
    ${ANDROID_LIB}
    ${EGL_LIB}
    ${GLES_LIB}
    ${AAUDIO_LIB}
)

# Release tuning: -O3 + LTO + stripped symbols (debug keeps symbols).
target_compile_options(fh2recomp PRIVATE
    -O3
    -fvisibility=hidden
    -Wall -Wextra -Wno-unused-parameter
)
set_target_properties(fh2recomp PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET fh2recomp PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
