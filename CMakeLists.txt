cmake_minimum_required(VERSION 3.24)

include(cmake/KoalaBox.cmake)

project(
    KoalaBox LANGUAGES CXX
    DESCRIPTION "A box of koality tools"
)

set(KOALABOX_SOURCES
    src/cache.cpp
    src/crypto.cpp
    src/dll_monitor.cpp
    src/globals.cpp
    src/logger.cpp
    src/hook.cpp
    src/http_client.cpp
    src/io.cpp
    src/ipc.cpp
    src/loader.cpp
    src/parser.cpp
    src/patcher.cpp
    src/path.cpp
    src/paths.cpp
    src/str.cpp
    src/util.cpp
    src/win.cpp
    src/zip.cpp
)

add_library(KoalaBox OBJECT ${KOALABOX_SOURCES})
target_compile_options(KoalaBox PUBLIC /std:c++latest)

### Setup KoalaBox exports

target_include_directories(KoalaBox PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_include_directories(KoalaBox PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")

set(PCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
target_precompile_headers(KoalaBox PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${PCH_PATH}>")

add_subdirectory(tools)

### Configure dependencies

# Note: Dependencies should be linked to KoalaBox using PUBLIC scope
# to avoid inconsistent flags/definitions.

## https://github.com/richgel999/miniz
CPMAddPackage("gh:richgel999/miniz#c883286f1a6443720e7705450f59e579a4bbb8e2")
target_link_libraries(KoalaBox PUBLIC miniz)

CPMAddPackage(
    URI "gh:libcpr/cpr#1.12.0"
    OPTIONS
    "CURL_USE_LIBPSL OFF"
    "CPR_USE_SYSTEM_LIB_PSL ON"
    "ENABLE_CURL_MANUAL OFF"
    "BUILD_LIBCURL_DOCS OFF"
    "BUILD_EXAMPLES OFF"
)
target_link_libraries(KoalaBox PUBLIC cpr)

## https://github.com/nlohmann/json
CPMAddPackage(
    URI "gh:nlohmann/json@3.12.0"
    OPTIONS "JSON_Diagnostics ON"
)
target_link_libraries(KoalaBox PUBLIC nlohmann_json)

## https://github.com/stevemk14ebr/PolyHook_2_0
CPMAddPackage(
    URI "gh:stevemk14ebr/PolyHook_2_0#298d56210b9d9e66cde8f96481d6053925c6ae15"
    OPTIONS
    "POLYHOOK_FEATURE_INLINENTD OFF"
    "POLYHOOK_FEATURE_EXCEPTION OFF"
)
target_link_libraries(KoalaBox PUBLIC PolyHook_2)

## https://github.com/gabime/spdlog
target_compile_definitions(KoalaBox PUBLIC
    $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE>
    $<$<NOT:$<CONFIG:Debug>>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
)
CPMAddPackage(
    URI "gh:gabime/spdlog@1.15.3"
    OPTIONS
    "SPDLOG_WCHAR_SUPPORT ON"
    "SPDLOG_WCHAR_FILENAMES ON"
    "SPDLOG_WCHAR_CONSOLE ON"
)
target_link_libraries(KoalaBox PUBLIC spdlog)

## https://github.com/nsumner/cpp-tree-sitter
# The latest commit includes a fix for MSVC, which is not present in the latest release
CPMAddPackage("gh:nsumner/cpp-tree-sitter#9cd7f3c9de9fc6d19bc982c6772fcd7a39e5797a")

# Grammars need to be installed in a special way
# Downloads a tree-sitter grammar from github
# and makes it available as a cmake library target.
add_grammar_from_repo(
    tree-sitter-cpp # Library name
    https://github.com/tree-sitter/tree-sitter-cpp.git # Repository URL
    0.23.4 # Version tag
)
target_link_libraries(KoalaBox PUBLIC cpp-tree-sitter tree-sitter-cpp)

## https://github.com/nemtrif/utfcpp
CPMAddPackage("gh:nemtrif/utfcpp@4.0.6")
target_compile_definitions(KoalaBox PUBLIC UTF_CPP_CPLUSPLUS=202002L)
target_link_libraries(KoalaBox PUBLIC utf8cpp)

## https://github.com/microsoft/wil
CPMAddPackage(
    URI "gh:microsoft/wil@1.0.250325.1"
    OPTIONS
    "WIL_BUILD_TESTS OFF"
    "WIL_BUILD_PACKAGING OFF"
)
target_link_libraries(KoalaBox PUBLIC WIL)
