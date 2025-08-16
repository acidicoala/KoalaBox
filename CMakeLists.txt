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
    include/koalabox/patcher.hpp
    include/koalabox/paths.hpp
    include/koalabox/util.hpp
    include/koalabox/win_util.hpp
    include/koalabox/zip.hpp
)

add_library(KoalaBox OBJECT
    src/cache.cpp
    src/core.cpp
    src/crypto.cpp
    src/dll_monitor.cpp
    src/globals.cpp
    src/logger.cpp
    src/hook.cpp
    src/http_client.cpp
    src/io.cpp
    src/ipc.cpp
    src/loader.cpp
    src/patcher.cpp
    src/paths.cpp
    src/util.cpp
    src/win_util.cpp
    src/zip.cpp
)

# Configure variables

include(GNUInstallDirs) # I think this should be after add_library?

# Configure dependencies

kb_add_package(miniz richgel999/miniz c883286f1a6443720e7705450f59e579a4bbb8e2)

set(CURL_USE_LIBPSL OFF) # We don't need support for Public Suffix List
set(CPR_USE_SYSTEM_LIB_PSL ON) # CPR requires meson if not using system PSL
set(ENABLE_CURL_MANUAL OFF)
set(BUILD_LIBCURL_DOCS OFF)
set(BUILD_EXAMPLES OFF)
kb_add_package(cpr libcpr/cpr 1.12.0)

## https://github.com/nlohmann/json
kb_add_package(nlohmann_json nlohmann/json v3.12.0)

## https://github.com/stevemk14ebr/PolyHook_2_0
set(POLYHOOK_FEATURE_INLINENTD OFF)
set(POLYHOOK_FEATURE_EXCEPTION OFF)
kb_add_package(PolyHook_2 stevemk14ebr/PolyHook_2_0 298d56210b9d9e66cde8f96481d6053925c6ae15)

## https://github.com/gabime/spdlog
if($<CONFIG:Debug>)
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
else()
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()
kb_add_package(spdlog gabime/spdlog v1.15.3)

## WinReg - https://github.com/GiovanniDicanio/WinReg
kb_add_package(WinReg acidicoala/WinReg b86786c6845a4a03a2e66a456cd65ec77069a7c4)

# Setup KoalaBox exports
target_include_directories(
    KoalaBox PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include;${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:include>"
)

set(PCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
target_precompile_headers(KoalaBox PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${PCH_PATH}>")

install(
    TARGETS KoalaBox DESTINATION ${LIB_INSTALL_DIR}
    INCLUDES ${KOALABOX_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}
)

add_subdirectory(tools)
