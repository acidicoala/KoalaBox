cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# Configure vcpkg triplet

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
if (${CMAKE_GENERATOR_PLATFORM} STREQUAL "Win32")
    set(VCPKG_TARGET_TRIPLET x86-windows-static CACHE STRING "VCPKG Target Triplet to use")
elseif (${CMAKE_GENERATOR_PLATFORM} STREQUAL "x64")
    set(VCPKG_TARGET_TRIPLET x64-windows-static CACHE STRING "VCPKG Target Triplet to use")
else ()
    message(FATAL_ERROR "Unexpected argument for the platform generator")
endif ()

# Configure project

project(KoalaBox)

add_library(
        KoalaBox
        ${SRC_DIR}/koalabox.cpp
        ${SRC_DIR}/logger/logger.cpp
        ${SRC_DIR}/win_util/win_util.cpp
        ${SRC_DIR}/util/util.cpp
)

target_include_directories(
        ${CMAKE_PROJECT_NAME} PRIVATE
        ${SRC_DIR}
)

# Link dependencies
find_package(spdlog CONFIG REQUIRED)

target_link_libraries(
        ${CMAKE_PROJECT_NAME} PRIVATE
        spdlog::spdlog
)
