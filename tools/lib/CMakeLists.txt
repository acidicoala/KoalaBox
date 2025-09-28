cmake_minimum_required(VERSION 3.24)

project(KoalaBoxToolsLib LANGUAGES CXX)

set(KOALABOX_TOOLS_HEADERS
    include/koalabox_tools/cmd.hpp
    include/koalabox_tools/parser.hpp
)

set(KOALABOX_TOOLS_SOURCES
    src/cmd.cpp
    src/parser.cpp
)

add_library(KoalaBoxTools OBJECT ${KOALABOX_TOOLS_HEADERS} ${KOALABOX_TOOLS_SOURCES})
target_include_directories(KoalaBoxTools PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")

## https://github.com/jarro2783/cxxopts
CPMAddPackage("gh:jarro2783/cxxopts@3.3.1")

## https://github.com/pantor/inja
set(nlohmann_json_DIR ${json_BINARY_DIR}) # needed for find_package(nlohmann_json) to succeed
CPMAddPackage(
    URI "gh:pantor/inja@3.4.0"
    OPTIONS
    "INJA_USE_EMBEDDED_JSON OFF"
    "INJA_INSTALL OFF"
    "INJA_EXPORT OFF"
    "BUILD_TESTING OFF"
)

## https://github.com/boostorg/preprocessor
CPMAddPackage("gh:boostorg/preprocessor#boost-1.89.0")

## https://github.com/bshoshany/thread-pool
CPMAddPackage(
    NAME BS_thread_pool
    GITHUB_REPOSITORY bshoshany/thread-pool
    VERSION 5.0.0
    EXCLUDE_FROM_ALL
    SYSTEM
)
add_library(BS_thread_pool INTERFACE)
target_include_directories(BS_thread_pool INTERFACE ${BS_thread_pool_SOURCE_DIR}/include)

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

target_link_libraries(KoalaBoxTools PUBLIC
    KoalaBox $<TARGET_OBJECTS:KoalaBox>
    boost_preprocessor
    cpp-tree-sitter tree-sitter-cpp
    BS_thread_pool
    cxxopts
    inja
)
