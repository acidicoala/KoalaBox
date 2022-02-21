macro(configure_include_directories)
    set(EXTRA_INCLUDE_DIRECTORIES "${ARGN}")

    target_include_directories(
        ${CMAKE_PROJECT_NAME} PRIVATE
        ${SRC_DIR}
        ${KOALABOX_SRC_DIR}
        ${GEN_DIR}
        ${EXTRA_INCLUDE_DIRECTORIES}
    )
endmacro()

macro(configure_dependencies PROJECT)
    set(DEPENDENCIES "${ARGN}")

    foreach (DEP IN LISTS DEPENDENCIES)
        find_package(${DEP} CONFIG REQUIRED)

        target_link_libraries(${PROJECT} PRIVATE ${DEP}::${DEP})
    endforeach ()
endmacro()

macro(set_32_and_64 VAR val_for_32 val_for_64)
    if (IS_64_BIT)
        set(${VAR} ${val_for_64})
    else ()
        set(${VAR} ${val_for_32})
    endif ()
endmacro()

macro(configure_globals KOALABOX_DIR)
    # Resolve KoalaBox directory
    get_filename_component(KOALABOX_DIR "${KOALABOX_DIR}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")

    # configure c++
    set(CMAKE_CXX_STANDARD 20)

    # configure bitness
    string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" CMAKE_GENERATOR_PLATFORM)

    if (${CMAKE_GENERATOR_PLATFORM} STREQUAL "win32")
        set(IS_64_BIT FALSE)
    elseif (${CMAKE_GENERATOR_PLATFORM} STREQUAL "x64")
        set(IS_64_BIT TRUE)
    else ()
        message(FATAL_ERROR "Unexpected argument for the platform generator")
    endif ()


    # configure vcpkg
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
    set_32_and_64(VCPKG_TARGET_TRIPLET x86-windows-static x64-windows-static)

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # configure variables
    set(VERSION_SUFFIX $ENV{VERSION_SUFFIX})

    set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
    set(RES_DIR "${CMAKE_SOURCE_DIR}/res")
    set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    file(MAKE_DIRECTORY "${GEN_DIR}")


    set(KOALABOX_SRC_DIR "${KOALABOX_DIR}/src")
    set(KOALABOX_RES_DIR "${KOALABOX_DIR}/res")

    set(LINKER_EXPORTS ${GEN_DIR}/linker_exports.h)
endmacro()

macro(configure_exports_generator)
    add_executable(
        exports_generator
        ${KOALABOX_SRC_DIR}/exports_generator/exports_generator.cpp
        ${KOALABOX_SRC_DIR}/koalabox/loader/loader.cpp
        ${KOALABOX_SRC_DIR}/koalabox/util/util.cpp
        ${KOALABOX_SRC_DIR}/koalabox/win_util/win_util.cpp
        ${KOALABOX_SRC_DIR}/koalabox/koalabox.cpp
    )

    configure_build_config()

    target_include_directories(
        exports_generator PRIVATE
        ${KOALABOX_SRC_DIR}
        ${GEN_DIR}
    )

    configure_precompile_headers(exports_generator koalabox/pch.hpp)

    configure_dependencies(exports_generator spdlog)
endmacro()

macro(configure_linker_exports FORWARD_PREFIX INPUT_DLL_PATH INPUT_SOURCES_DIR)
    # Make the linker_exports header available before build
    file(TOUCH ${LINKER_EXPORTS})

    add_custom_command(
        OUTPUT ${LINKER_EXPORTS}
        COMMAND exports_generator # Executable path
        ${FORWARD_PREFIX} # Forwarded DLL path
        ${INPUT_DLL_PATH} # Input DLL
        ${LINKER_EXPORTS} # Output header
        "${INPUT_SOURCES_DIR}" # Input sources
        DEPENDS exports_generator
        ${INPUT_DLL_PATH}
        ${ARGN} # Extra source dependencies
    )
endmacro()


macro(configure_build_config)
    set(EXTRA_CONFIGS "${ARGN}")

    configure_file("${KOALABOX_RES_DIR}/build_config.gen.h" "${GEN_DIR}/build_config.h")

    foreach (EXTRA_CONFIG IN LISTS EXTRA_CONFIGS)
        set(GENERATED_EXTRA_CONFIG ${GEN_DIR}/${EXTRA_CONFIG}.h)

        file(TOUCH ${GENERATED_EXTRA_CONFIG})
        configure_file(${RES_DIR}/${EXTRA_CONFIG}.gen.h ${GENERATED_EXTRA_CONFIG})
        file(APPEND "${GEN_DIR}/build_config.h" "#include <${EXTRA_CONFIG}.h>\n")
    endforeach ()
endmacro()

macro(configure_version_resource FILE_DESC)
    ## Generate version resource file
    set(DLL_VERSION_FILE_DESC "${FILE_DESC}")
    set(DLL_VERSION_PRODUCT_NAME "${CMAKE_PROJECT_NAME}")
    set(DLL_VERSION_INTERNAL_NAME "${DLL_VERSION_PRODUCT_NAME}")
    configure_file(${KOALABOX_RES_DIR}/version.gen.rc ${GEN_DIR}/version.rc)
endmacro()

macro(configure_library TYPE)
    set(SOURCES "${ARGN}")

    add_library(
        ${CMAKE_PROJECT_NAME} ${TYPE}
        ${SOURCES}
        ${KOALABOX_SRC_DIR}/koalabox/koalabox.cpp

        ## Resources
        ${GEN_DIR}/version.rc
    )
endmacro()

macro(configure_output_name OUTPUT_NAME)
    set_target_properties(
        ${CMAKE_PROJECT_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_NAME "${OUTPUT_NAME}"
    )
endmacro()


macro(configure_precompile_headers PROJECT PCH_PATH)
    target_precompile_headers(
        ${PROJECT} PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${PCH_PATH}>"
    )
endmacro()
