cmake_minimum_required(VERSION 3.24)

project(KoalaBoxToolsLib LANGUAGES CXX)

# https://github.com/jarro2783/cxxopts
CPMAddPackage("gh:jarro2783/cxxopts@3.3.1")

# https://github.com/pantor/inja
set(nlohmann_json_DIR ${json_BINARY_DIR}) # needed for find_package(nlohmann_json) to succeed
CPMAddPackage(
    URI "gh:pantor/inja@3.4.0"
    OPTIONS
    "INJA_USE_EMBEDDED_JSON OFF"
    "INJA_INSTALL OFF"
    "INJA_EXPORT OFF"
    "BUILD_TESTING OFF"
)

# https://github.com/boostorg/preprocessor
CPMAddPackage("gh:boostorg/preprocessor#boost-1.89.0")

set(KOALABOX_TOOLS_HEADERS
    include/koalabox_tools/cmd.hpp
)

set(KOALABOX_TOOLS_SOURCES
    src/cmd.cpp
)

add_library(KoalaBoxTools OBJECT ${KOALABOX_TOOLS_HEADERS} ${KOALABOX_TOOLS_SOURCES})
target_include_directories(KoalaBoxTools PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")

target_link_libraries(KoalaBoxTools PUBLIC
    KoalaBox $<TARGET_OBJECTS:KoalaBox>
    cxxopts
    inja
    boost_preprocessor
)
