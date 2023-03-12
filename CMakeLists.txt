cmake_minimum_required(VERSION 3.24)

project(
    KoalaBox LANGUAGES CXX
    DESCRIPTION "A box of koality tools"
)

# Configure variables

include(GNUInstallDirs)

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
    include/koalabox/patcher.hpp
    include/koalabox/paths.hpp
    include/koalabox/util.hpp
    include/koalabox/win_util.hpp
)

set(
    KOALABOX_SOURCES
    src/koalabox/cache.cpp
    src/koalabox/core.cpp
    src/koalabox/crypto.cpp
    src/koalabox/dll_monitor.cpp
    src/koalabox/globals.cpp
    src/koalabox/logger.cpp
    src/koalabox/hook.cpp
    src/koalabox/http_client.cpp
    src/koalabox/io.cpp
    src/koalabox/ipc.cpp
    src/koalabox/loader.cpp
    src/koalabox/ntapi.hpp
    src/koalabox/patcher.cpp
    src/koalabox/paths.cpp
    src/koalabox/util.cpp
    src/koalabox/win_util.cpp
)

add_library(KoalaBox OBJECT ${KOALABOX_HEADERS} ${KOALABOX_SOURCES})

# Configure dependencies

include(FetchContent)

function(fetch_library LIB USER_REPO TAG)
    FetchContent_Declare(${LIB} GIT_REPOSITORY "https://github.com/${USER_REPO}" GIT_TAG ${TAG})
    FetchContent_MakeAvailable(${LIB})
    target_link_libraries(KoalaBox PUBLIC ${LIB})
endfunction()

## Cpr - https://github.com/libcpr/cpr/
fetch_library(cpr libcpr/cpr 1.10.1)

## Json - https://github.com/nlohmann/json
fetch_library(nlohmann_json nlohmann/json v3.11.2)

## Polyhook2 - https://github.com/stevemk14ebr/PolyHook_2_0
set(POLYHOOK_FEATURE_INLINENTD OFF)
set(POLYHOOK_FEATURE_EXCEPTION OFF)
fetch_library(PolyHook_2 stevemk14ebr/PolyHook_2_0 71d273463a3c4e30ec0a4031c4b477b85ea773fb)

## Spdlog - https://github.com/gabime/spdlog
fetch_library(spdlog gabime/spdlog v1.11.0)

## Miniz - https://github.com/richgel999/miniz
fetch_library(miniz richgel999/miniz 3.0.2)

## WinReg - https://github.com/GiovanniDicanio/WinReg
fetch_library(WinReg acidicoala/WinReg b86786c6845a4a03a2e66a456cd65ec77069a7c4)

# Setup KoalaBox exports
target_include_directories(
    KoalaBox PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include;${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:include>"
)

set(PCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/koalabox/pch.hpp)
target_precompile_headers(KoalaBox PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${PCH_PATH}>")

install(
    TARGETS KoalaBox DESTINATION ${LIB_INSTALL_DIR}
    INCLUDES ${KOALABOX_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}
)

# Exports Generator

add_executable(exports_generator src/exports_generator/exports_generator.cpp)

target_link_libraries(exports_generator PUBLIC KoalaBox)
