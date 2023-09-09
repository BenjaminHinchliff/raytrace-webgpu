cmake_minimum_required(VERSION 3.7...3.27)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(TraceWebG
    VERSION 0.0.1
    DESCRIPTION "WebGPU Ray Tracing in C"
    LANGUAGES C CXX)

# add_compile_options(-fsanitize=address,undefined)
# add_link_options(-fsanitize=address,undefined)

set(FETCHCONTENT_QUIET FALSE CACHE BOOL "" FORCE)

include(cmake/CPM.cmake)

CPMAddPackage(
    NAME glfwpp
    OPTIONS "GLFWPP_BUILD_EXAMPLES OFF"
    GIT_REPOSITORY https://github.com/BenjaminHinchliff/glfwpp.git
    GIT_TAG 5c7452abf83deea756557a9ec7b50249fd04b912
    GIT_PROGRESS TRUE
)

CPMAddPackage(
    NAME depot_tools
    GIT_REPOSITORY https://chromium.googlesource.com/chromium/tools/depot_tools.git
    GIT_TAG 28116103e4df01890abfa5f1d15a003f69729d81
    GIT_PROGRESS TRUE
)

CPMAddPackage(
    NAME dawn
    DOWNLOAD_ONLY TRUE
    GIT_REPOSITORY https://dawn.googlesource.com/dawn.git
    GIT_TAG 97a50fb01224df6e5158657bb5ad6890f973cdd5
    GIT_SUBMODULES "docs"
    GIT_PROGRESS TRUE
)

if (dawn_ADDED)
    configure_file(${dawn_SOURCE_DIR}/scripts/standalone.gclient ${dawn_SOURCE_DIR}/.gclient COPYONLY)
    set(ENV{PATH} "${depot_tools_SOURCE_DIR}:$ENV{PATH}")
    execute_process(
        COMMAND gclient sync
        WORKING_DIRECTORY ${dawn_SOURCE_DIR}
    )

    # set(DAWN_BUILD_GEN_DIR ${dawn_BINARY_DIR}/gen)
    # set(DAWN_THIRD_PARTY_DIR ${dawn_BINARY_DIR}/dawn/third_party)

    # set(DAWN_ENABLE_ASAN OFF CACHE BOOL "" FORCE)
    # set(DAWN_ENABLE_TSAN OFF CACHE BOOL "" FORCE)
    # set(DAWN_ENABLE_MSAN OFF CACHE BOOL "" FORCE)
    # set(DAWN_ENABLE_UBSAN OFF CACHE BOOL "" FORCE)

    set(DAWN_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)

    # Turn off some Dawn backends we aren't going to be using anywhere
    set(DAWN_ENABLE_DESKTOP_GL OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_OPENGLES OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_NULL OFF CACHE BOOL "" FORCE)

    set(TINT_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(TINT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(TINT_BUILD_SAMPLES ON CACHE BOOL "" FORCE)

    # Turn off some Tint functionality we don't require
    set(TINT_BUILD_GLSL_WRITER OFF CACHE BOOL "" FORCE)
    set(TINT_BUILD_SPV_READER OFF CACHE BOOL "" FORCE)

    # if (NOT APPLE)
    #   set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    # endif()

    add_definitions(
      -Wno-deprecated-builtins    # Warning from Abseil
      -Wno-unknown-warning-option  # SPIRV-Tools
    )

    add_subdirectory(${dawn_SOURCE_DIR} ${dawn_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_subdirectory("src")
add_subdirectory("shaders")
