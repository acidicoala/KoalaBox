cmake_minimum_required(VERSION 3.21)

include(KoalaBox.cmake)

configure_globals(${CMAKE_SOURCE_DIR})

project(KoalaBox VERSION 1.0.0)

configure_build_config()

configure_version_resource("A box of koality tools")

configure_exports_generator()

configure_library(
    SHARED

    ${KOALABOX_SRC_DIR}/koalabox/dll_monitor/dll_monitor.cpp
    ${KOALABOX_SRC_DIR}/koalabox/hook/hook.cpp
    ${KOALABOX_SRC_DIR}/koalabox/loader/loader.cpp
    ${KOALABOX_SRC_DIR}/koalabox/logger/logger.cpp
    ${KOALABOX_SRC_DIR}/koalabox/util/util.cpp
    ${KOALABOX_SRC_DIR}/koalabox/win_util/win_util.cpp
)

configure_precompile_headers(${SRC_DIR}/koalabox/pch.hpp)

configure_output_name(KoalaBox)

configure_include_directories()

configure_dependencies(
    KoalaBox
    nlohmann_json
    PolyHook_2
    spdlog
)
