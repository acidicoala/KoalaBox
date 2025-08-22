cmake_minimum_required(VERSION 3.24)

include(cmake/KoalaBox.cmake)

project(
    KoalaBox LANGUAGES CXX
    DESCRIPTION "A box of koality tools"
)

set(
    KOALABOX_HEADERS
    include/koalabox/cache.hpp
    include/koalabox/config.hpp
    include/koalabox/core.hpp
    include/koalabox/crypto.hpp
    include/koalabox/dll_monitor.hpp
    include/koalabox/globals.hpp
    include/koalabox/logger.hpp
    include/koalabox/hook.hpp
    include/koalabox/http_client.hpp
    include/koalabox/io.hpp
    include/koalabox/ipc.hpp
    include/koalabox/loader.hpp
    include/koalabox/parser.hpp
    include/koalabox/patcher.hpp
    include/koalabox/paths.hpp
    include/koalabox/str.hpp
    include/koalabox/util.hpp
    include/koalabox/win_util.hpp
    include/koalabox/zip.hpp
)

add_library(KoalaBox OBJECT
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
    src/paths.cpp
    src/str.cpp
    src/util.cpp
    src/win_util.cpp
    src/zip.cpp
)

### Configure dependencies

## Miniz - https://github.com/richgel999/miniz
CPMAddPackage("gh:richgel999/miniz#c883286f1a6443720e7705450f59e579a4bbb8e2")
target_link_libraries(KoalaBox PRIVATE miniz)

CPMAddPackage(
    URI "gh:libcpr/cpr#1.12.0"
    OPTIONS
    "CURL_USE_LIBPSL OFF"
    "CPR_USE_SYSTEM_LIB_PSL ON"
    "ENABLE_CURL_MANUAL OFF"
    "BUILD_LIBCURL_DOCS OFF"
    "BUILD_EXAMPLES OFF"
)
target_link_libraries(KoalaBox PRIVATE cpr)

## JSON - https://github.com/nlohmann/json
CPMAddPackage(
    URI "gh:nlohmann/json@3.12.0"
    OPTIONS "JSON_Diagnostics ON"
)
target_link_libraries(KoalaBox PUBLIC nlohmann_json)

## PolyHook 2 - https://github.com/stevemk14ebr/PolyHook_2_0
CPMAddPackage(
    URI "gh:stevemk14ebr/PolyHook_2_0#298d56210b9d9e66cde8f96481d6053925c6ae15"
    OPTIONS
    "POLYHOOK_FEATURE_INLINENTD OFF"
    "POLYHOOK_FEATURE_EXCEPTION OFF"
)
target_link_libraries(KoalaBox PRIVATE PolyHook_2)

## Logging - https://github.com/gabime/spdlog
target_compile_definitions(KoalaBox PUBLIC
    $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE>
    $<$<NOT:$<CONFIG:Debug>>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
)
CPMAddPackage("gh:gabime/spdlog@1.15.3")
target_link_libraries(KoalaBox PUBLIC spdlog)

## WinReg - https://github.com/GiovanniDicanio/WinReg
CPMAddPackage("gh:acidicoala/WinReg#b86786c6845a4a03a2e66a456cd65ec77069a7c4")
target_link_libraries(KoalaBox PRIVATE WinReg)

## C++ parser - https://github.com/nsumner/cpp-tree-sitter
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

### Setup KoalaBox exports

target_include_directories(KoalaBox PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_include_directories(KoalaBox PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")

set(PCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
target_precompile_headers(KoalaBox PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${PCH_PATH}>")

add_subdirectory(tools)
