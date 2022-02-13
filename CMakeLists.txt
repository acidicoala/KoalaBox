cmake_minimum_required(VERSION 3.21)

include(KoalaBox.cmake)

configure_globals(".")

project(KoalaBox VERSION 1.0.0)

configure_build_config()

configure_version_resource("A box of koality tools")

configure_library(
    SHARED

    ${KOALABOX_SOURCES}
)

configure_precompile_headers(${SRC_DIR}/koalabox/pch.hpp)

configure_output_name(KoalaBox)

configure_include_directories()

configure_dependencies(spdlog)
