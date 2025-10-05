cmake_minimum_required(VERSION 3.24)

project(KoalaBoxToolsLib LANGUAGES CXX)

set(KOALABOX_TOOLS_HEADERS
    include/koalabox_tools/cmd.hpp
    include/koalabox_tools/module.hpp
    include/koalabox_tools/parser.hpp
    include/koalabox_tools/zip.hpp
)

set(KOALABOX_TOOLS_SOURCES
    src/cmd.cpp
    src/module.cpp
    src/parser.cpp
    src/zip.cpp
)

if(WIN32)
    list(APPEND KOALABOX_TOOLS_SOURCES src/module_win.cpp)
else()
    list(APPEND KOALABOX_TOOLS_SOURCES src/module_linux.cpp)
endif()

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

## https://github.com/richgel999/miniz
target_compile_definitions(KoalaBoxTools PUBLIC MINIZ_NO_DEFLATE_APIS)
CPMAddPackage("gh:richgel999/miniz#c883286f1a6443720e7705450f59e579a4bbb8e2")

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

if(WIN32)
    ## https://github.com/trailofbits/pe-parse
    CPMAddPackage(
        URI "gh:trailofbits/pe-parse@2.1.1"
        OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "BUILD_COMMAND_LINE_TOOLS OFF"
        "PEPARSE_LIBRARY_WARNINGS OFF"
    )
    target_link_libraries(KoalaBoxTools PUBLIC pe-parse)
endif()

## Steamworks downloader needs this to fix linux binaries
## https://github.com/serge1/ELFIO
CPMAddPackage("gh:serge1/ELFIO#Release_3.12")
target_link_libraries(KoalaBoxTools PUBLIC elfio)

target_link_libraries(KoalaBoxTools PUBLIC
    KoalaBox $<TARGET_OBJECTS:KoalaBox>
    boost_preprocessor
    BS_thread_pool
    cpp-tree-sitter tree-sitter-cpp
    cxxopts
    inja
    miniz
)
