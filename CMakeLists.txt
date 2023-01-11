cmake_minimum_required(VERSION 3.22)

project(KoalaBox
    LANGUAGES CXX
    DESCRIPTION "A box of koality tools")

# Configure variables

include(GNUInstallDirs)

# Configure dependencies

set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC Runtime Library")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build DLL instead of static library")

## Cpr - https://github.com/libcpr/cpr/
add_subdirectory(dependencies/cpr EXCLUDE_FROM_ALL)

## Json - https://github.com/nlohmann/json
add_subdirectory(dependencies/json EXCLUDE_FROM_ALL)

## Polyhook2 - https://github.com/stevemk14ebr/PolyHook_2_0
set(POLYHOOK_FEATURE_DETOURS ON)
set(POLYHOOK_FEATURE_INLINENTD OFF)
set(POLYHOOK_FEATURE_EXCEPTION OFF)
set(POLYHOOK_BUILD_DLL ON)
add_subdirectory(dependencies/PolyHook_2_0 EXCLUDE_FROM_ALL)

## Spdlog - https://github.com/gabime/spdlog
add_subdirectory(dependencies/spdlog EXCLUDE_FROM_ALL)

set(KOALABOX_HEADERS
    include/koalabox/cache.hpp
    include/koalabox/core.hpp
    include/koalabox/dll_monitor.hpp
    include/koalabox/logger.hpp
    include/koalabox/hook.hpp
    include/koalabox/http_client.hpp
    include/koalabox/io.hpp
    include/koalabox/ipc.hpp
    include/koalabox/loader.hpp
    include/koalabox/patcher.hpp
    include/koalabox/util.hpp
    include/koalabox/win_util.hpp
    )

set(KOALABOX_SOURCES
    src/koalabox/cache.cpp
    src/koalabox/core.cpp
    src/koalabox/dll_monitor.cpp
    src/koalabox/logger.cpp
    src/koalabox/hook.cpp
    src/koalabox/http_client.cpp
    src/koalabox/io.cpp
    src/koalabox/ipc.cpp
    src/koalabox/loader.cpp
    src/koalabox/ntapi.hpp
    src/koalabox/patcher.cpp
    src/koalabox/util.cpp
    src/koalabox/win_util.cpp
    )

add_library(KoalaBox OBJECT ${KOALABOX_HEADERS} ${KOALABOX_SOURCES})

target_link_libraries(KoalaBox PUBLIC cpr::cpr)
target_link_libraries(KoalaBox PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(KoalaBox PUBLIC PolyHook_2)
target_link_libraries(KoalaBox PUBLIC spdlog::spdlog)

target_include_directories(KoalaBox PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include;${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:include>")

set(PCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/koalabox/pch.hpp)
target_precompile_headers(KoalaBox PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${PCH_PATH}>")

install(
    TARGETS KoalaBox DESTINATION lib
    INCLUDES ${KOALABOX_HEADERS} DESTINATION include
)

# Exports Generator

add_executable(exports_generator src/exports_generator/exports_generator.cpp)

target_link_libraries(exports_generator PUBLIC KoalaBox)
