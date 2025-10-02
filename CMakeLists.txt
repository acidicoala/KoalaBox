cmake_minimum_required(VERSION 3.24)

include(cmake/KoalaBox.cmake)

project(
    KoalaBox LANGUAGES CXX
    DESCRIPTION "A box of koality tools"
)

set(KOALABOX_HEADERS
    include/koalabox/cache.hpp
    include/koalabox/core.hpp
    include/koalabox/config.hpp
    include/koalabox/crypto.hpp
    include/koalabox/globals.hpp
    include/koalabox/hook.hpp
    include/koalabox/http_client.hpp
    include/koalabox/io.hpp
    include/koalabox/ipc.hpp
    include/koalabox/logger.hpp
    include/koalabox/lib.hpp
    include/koalabox/lib_monitor.hpp
    include/koalabox/patcher.hpp
    include/koalabox/path.hpp
    include/koalabox/paths.hpp
    include/koalabox/platform.hpp
    include/koalabox/str.hpp
    include/koalabox/util.hpp
    include/koalabox/zip.hpp
)

set(KOALABOX_SOURCES
    src/cache.cpp
    src/crypto.cpp
    src/globals.cpp
    src/logger.cpp
    src/hook.cpp
    src/http_client.cpp
    src/io.cpp
    src/lib.cpp
    src/lib_monitor.cpp
    src/patcher.cpp
    src/path.cpp
    src/paths.cpp
    src/str.cpp
    src/util.cpp
    src/zip.cpp
)

add_library(KoalaBox OBJECT ${KOALABOX_HEADERS} ${KOALABOX_SOURCES})

if(WIN32)
    target_sources(KoalaBox PRIVATE
        include/koalabox/win.hpp
        src/ipc_win.cpp
        src/lib_monitor_win.cpp
        src/lib_win.cpp
        src/path_win.cpp
        src/paths_win.cpp
        src/str_win.cpp
        src/util_win.cpp
        src/win.cpp
    )
elseif(LINUX)
    target_sources(KoalaBox PRIVATE
        src/lib_linux.cpp
        src/lib_monitor_linux.cpp
        src/path_linux.cpp
        src/paths_linux.cpp
        src/str_linux.cpp
        src/util_linux.cpp
    )

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

    include_directories(${GTK3_INCLUDE_DIRS})
endif()

### Setup KoalaBox exports

target_include_directories(KoalaBox PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")


set(PCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
target_precompile_headers(KoalaBox PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${PCH_PATH}>")

### Configure dependencies

# Note: Dependencies should be linked to KoalaBox using PUBLIC scope
# to avoid inconsistent flags/definitions.

## https://github.com/serge1/ELFIO
CPMAddPackage("gh:serge1/ELFIO#Release_3.12")
target_link_libraries(KoalaBox PUBLIC elfio)

## https://github.com/p-ranav/glob
CPMAddPackage("gh:p-ranav/glob#e796e3ca94e0c514296425d9884e2198c41699ee")
target_link_libraries(KoalaBox PUBLIC Glob)

## https://github.com/nlohmann/json
CPMAddPackage(
    URI "gh:nlohmann/json@3.12.0"
    OPTIONS "JSON_Diagnostics ON"
)
target_link_libraries(KoalaBox PUBLIC nlohmann_json::nlohmann_json)

## https://github.com/richgel999/miniz
target_compile_definitions(KoalaBox PUBLIC MINIZ_NO_DEFLATE_APIS)
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

## https://github.com/stevemk14ebr/PolyHook_2_0
CPMAddPackage(
    NAME PolyHook_2_0
    GIT_REPOSITORY "https://github.com/stevemk14ebr/PolyHook_2_0.git"
    GIT_TAG "09a882ef40b821b32a51770785a8e05373073e84"
    GIT_SUBMODULES "*"
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
set(SPDLOG_OPTIONS "SPDLOG_USE_STD_FORMAT ON")
if(WIN32)
    list(APPEND SPDLOG_OPTIONS
        "SPDLOG_WCHAR_SUPPORT ON"
        "SPDLOG_WCHAR_FILENAMES ON"
        "SPDLOG_WCHAR_CONSOLE ON"
    )
endif()
CPMAddPackage(
    URI "gh:gabime/spdlog@1.15.3"
    OPTIONS ${SPDLOG_OPTIONS}
)
target_link_libraries(KoalaBox PUBLIC spdlog)

## https://github.com/nemtrif/utfcpp
CPMAddPackage("gh:nemtrif/utfcpp@4.0.6")
target_compile_definitions(KoalaBox PUBLIC UTF_CPP_CPLUSPLUS=202002L)
target_link_libraries(KoalaBox PUBLIC utf8cpp)

if(WIN32)
    ## https://github.com/microsoft/wil
    CPMAddPackage(
        URI "gh:microsoft/wil@1.0.250325.1"
        OPTIONS
        "WIL_BUILD_TESTS OFF"
        "WIL_BUILD_PACKAGING OFF"
    )
    target_link_libraries(KoalaBox PUBLIC WIL)

    # https://github.com/winsiderss/phnt
    CPMAddPackage("gh:winsiderss/phnt#fc1f96ee976635f51faa89896d1d805eb0586350")
    target_link_libraries(KoalaBox PUBLIC phnt)
endif()

add_subdirectory(tools)
