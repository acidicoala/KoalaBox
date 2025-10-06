include_guard()

set(CMAKE_CXX_STANDARD 23 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC Runtime Library")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build DLL instead of static library")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:-Zc:preprocessor>")

set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/build/.cache" CACHE STRING "CPM.cmake source cache")
include("${CMAKE_CURRENT_LIST_DIR}/get_cpm.cmake")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG_BUILD)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(IS_64 TRUE)
else()
    set(IS_32 TRUE)
endif()

# Sets the variable ${VAR} with val_for_32 on 32-bit build
# and appends 64 to val_for_32 on 64-bit build, unless an optional argument is provided.
function(set_32_and_64 VAR val_for_32)
    if(DEFINED ARGV2)
        set(val_for_64 ${ARGV2})
    else()
        set(val_for_64 "${val_for_32}64")
    endif()

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(${VAR} ${val_for_64} PARENT_SCOPE)
    else()
        set(${VAR} ${val_for_32} PARENT_SCOPE)
    endif()
endfunction()

## Generate version resource file
function(configure_version_resource)
    cmake_parse_arguments(
        arg "" "TARGET;FILE_DESC;ORIG_NAME" "" ${ARGN}
    )
    set(DLL_FILE_DESC ${arg_FILE_DESC})
    set(DLL_ORIG_NAME ${arg_ORIG_NAME})
    set(VERSION_RESOURCE "${CMAKE_CURRENT_BINARY_DIR}/generated/version.rc")

    get_target_property(KOALABOX_SOURCE_DIR KoalaBox SOURCE_DIR)
    configure_file("${KOALABOX_SOURCE_DIR}/res/version.gen.rc" ${VERSION_RESOURCE})
    target_sources(${arg_TARGET} PRIVATE ${VERSION_RESOURCE})
endfunction()

function(configure_build_config)
    # Run a command to get the short commit hash
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_SHORT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Check if the current commit has tags
    execute_process(
        COMMAND git tag --points-at HEAD
        OUTPUT_VARIABLE GIT_TAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Set VERSION_SUFFIX based on presence of tags
    if(GIT_TAGS STREQUAL "")
        set(VERSION_SUFFIX "-dev[${GIT_SHORT_HASH}]")
    else()
        set(VERSION_SUFFIX "")
    endif()

    set(BUILD_CONFIG_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/build_config.h")

    get_target_property(KOALABOX_SOURCE_DIR KoalaBox SOURCE_DIR)
    configure_file("${KOALABOX_SOURCE_DIR}/res/build_config.gen.h" ${BUILD_CONFIG_HEADER})

    foreach(EXTRA_CONFIG IN LISTS ARGN)
        set(GENERATED_EXTRA_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/generated/${EXTRA_CONFIG}.h")

        file(TOUCH ${GENERATED_EXTRA_CONFIG})
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/${EXTRA_CONFIG}.gen.h ${GENERATED_EXTRA_CONFIG})
        file(APPEND ${BUILD_CONFIG_HEADER} "#include <${EXTRA_CONFIG}.h>\n")
    endforeach()
endfunction()


function(configure_output_name OUT_NAME)
    set_target_properties(
        ${CMAKE_PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_NAME "${OUT_NAME}"
    )
endfunction()

function(configure_include_directories)
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
        "${ARGN}")
endfunction()

function(configure_linker_exports)
    cmake_parse_arguments(
        ARG "" "TARGET;HEADER_NAME;FORWARDED_DLL_NAME;LIB_FILES_GLOB;SOURCES_INPUT_PATH" "" ${ARGN}
    )

    set(GENERATED_LINKER_EXPORTS "${CMAKE_CURRENT_BINARY_DIR}/generated/${ARG_HEADER_NAME}")

    # Make the linker_exports header available before build
    file(TOUCH ${GENERATED_LINKER_EXPORTS})

    set(GENERATE_LINKER_EXPORTS_TARGET "generate_linker_exports_for_${ARG_HEADER_NAME}")
    add_custom_target("${GENERATE_LINKER_EXPORTS_TARGET}" ALL
        COMMENT "Generate linker exports for export address table"
        COMMAND windows_exports_generator # Executable path
        --forwarded_dll_name "${ARG_FORWARDED_DLL_NAME}"
        --lib_files_glob "\"${ARG_LIB_FILES_GLOB}\""
        --output_file_path "\"${GENERATED_LINKER_EXPORTS}\""
        --sources_input_path "\"${ARG_SOURCES_INPUT_PATH}\""

        DEPENDS windows_exports_generator
    )

    target_sources(${ARG_TARGET} PRIVATE ${GENERATED_LINKER_EXPORTS})
    add_dependencies("${ARG_TARGET}" "${GENERATE_LINKER_EXPORTS_TARGET}")
endfunction()

function(configure_proxy_exports)
    cmake_parse_arguments(
        ARG "" "TARGET;INPUT_LIBS_GLOB;OUTPUT_NAME" "" ${ARGN}
    )

    set(GENERATED_PROXY_EXPORTS "${CMAKE_CURRENT_BINARY_DIR}/generated/${ARG_OUTPUT_NAME}")

    set(GENERATED_HEADER "${GENERATED_PROXY_EXPORTS}.hpp")
    set(GENERATED_SOURCE "${GENERATED_PROXY_EXPORTS}.cpp")

    # Make header and source available before build
    file(TOUCH ${GENERATED_HEADER})
    file(TOUCH ${GENERATED_SOURCE})

    set(GENERATE_PROXY_EXPORTS_TARGET "generate_${ARG_OUTPUT_NAME}")
    add_custom_target("${GENERATE_PROXY_EXPORTS_TARGET}" ALL
        COMMENT "Generate proxy exports"
        COMMAND linux_exports_generator # Executable path
        --input_libs_glob "'${ARG_INPUT_LIBS_GLOB}'"
        --output_path "${GENERATED_PROXY_EXPORTS}"

        DEPENDS linux_exports_generator
    )

    target_sources(${ARG_TARGET} PRIVATE ${GENERATED_HEADER} ${GENERATED_SOURCE})
    add_dependencies("${ARG_TARGET}" "${GENERATE_PROXY_EXPORTS_TARGET}")
endfunction()
