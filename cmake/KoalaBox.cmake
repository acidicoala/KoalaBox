# Sets the variable ${VAR} with val_for_32 on 32-bit build
# and appends 64 to val_for_32 on 64-bit build, unless it an optional argument
# is provided.
function(set_32_and_64 VAR val_for_32)
    if (DEFINED ARGV2)
        set(val_for_64 ${ARGV2})
    else ()
        set(val_for_64 "${val_for_32}64")
    endif ()

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(${VAR} ${val_for_64} PARENT_SCOPE)
    else ()
        set(${VAR} ${val_for_32} PARENT_SCOPE)
    endif ()
endfunction()

## Generate version resource file
function(configure_version_resource FILE_DESC)
    set(DLL_VERSION_FILE_DESC ${FILE_DESC})
    set(DLL_VERSION_PRODUCT_NAME ${CMAKE_PROJECT_NAME})
    set(DLL_VERSION_INTERNAL_NAME ${CMAKE_PROJECT_NAME})

    set(VERSION_RESOURCE "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
    set(VERSION_RESOURCE ${VERSION_RESOURCE} PARENT_SCOPE)

    configure_file(KoalaBox/res/version.gen.rc ${VERSION_RESOURCE})
endfunction()

function(configure_build_config)
    set(BUILD_CONFIG_HEADER "${CMAKE_CURRENT_BINARY_DIR}/build_config.h")

    configure_file(KoalaBox/res/build_config.gen.h ${BUILD_CONFIG_HEADER})

    foreach (EXTRA_CONFIG IN LISTS ARGN)
        set(GENERATED_EXTRA_CONFIG ${CMAKE_CURRENT_BINARY_DIR}/${EXTRA_CONFIG}.h)

        file(TOUCH ${GENERATED_EXTRA_CONFIG})
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/${EXTRA_CONFIG}.gen.h ${GENERATED_EXTRA_CONFIG})
        file(APPEND ${BUILD_CONFIG_HEADER} "#include <${EXTRA_CONFIG}.h>\n")
    endforeach ()
endfunction()


function(configure_output_name OUT_NAME)
    set_target_properties(
        ${CMAKE_PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_NAME "${OUT_NAME}"
    )
endfunction()

function(configure_include_directories)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${ARGN}")
endfunction()

function(link_to_koalabox)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE KoalaBox)
endfunction()

function(configure_linker_exports)
    cmake_parse_arguments(
        ARG "UNDECORATE" "FORWARDED_DLL;INPUT_SOURCES_DIR" "INPUT_DLLS;DEP_SOURCES" ${ARGN}
    )

    string(JOIN "|" JOINED_INPUT_DLLS ${ARG_INPUT_DLLS})

    set(GENERATED_LINKER_EXPORTS "${CMAKE_CURRENT_BINARY_DIR}/linker_exports.h")
    set(GENERATED_LINKER_EXPORTS "${GENERATED_LINKER_EXPORTS}" PARENT_SCOPE)

    # Make the linker_exports header available before build
    file(TOUCH ${GENERATED_LINKER_EXPORTS})

    add_custom_command(
        OUTPUT ${GENERATED_LINKER_EXPORTS}
        COMMAND exports_generator # Executable path
        ${ARG_UNDECORATE} # Undecorate boolean
        ${ARG_FORWARDED_DLL} # Forwarded DLL path
        "\"${JOINED_INPUT_DLLS}\"" # Input DLLs
        "${GENERATED_LINKER_EXPORTS}" # Output header
        "${ARG_INPUT_SOURCES_DIR}" # Input sources
        DEPENDS exports_generator
        ${ARG_INPUT_DLLS}
        ${ARG_DEP_SOURCES}
    )
endfunction()
