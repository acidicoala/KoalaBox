include_guard()

set(CMAKE_CXX_STANDARD 23 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC Runtime Library")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build DLL instead of static library")

set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/build/.cache" CACHE STRING "CPM.cmake source cache")
include("${CMAKE_CURRENT_LIST_DIR}/get_cpm.cmake")

function(kb_add_package LIB USER_REPO TAG)
    CPMAddPackage(
        NAME ${LIB}
        GIT_REPOSITORY "https://github.com/${USER_REPO}.git"
        GIT_TAG ${TAG}
    )
    target_link_libraries(KoalaBox PUBLIC ${LIB})
endfunction()

# Sets the variable ${VAR} with val_for_32 on 32-bit build
# and appends 64 to val_for_32 on 64-bit build, unless it an optional argument
# is provided.
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

    foreach(EXTRA_CONFIG IN LISTS ARGN)
        set(GENERATED_EXTRA_CONFIG ${CMAKE_CURRENT_BINARY_DIR}/${EXTRA_CONFIG}.h)

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
        ARG "UNDECORATE" "TARGET;FORWARDED_DLL;INPUT_SOURCES_DIR;DLL_FILES_GLOB" "" ${ARGN}
    )

    set(GENERATED_LINKER_EXPORTS "${CMAKE_CURRENT_BINARY_DIR}/linker_exports.h")
    set(GENERATED_LINKER_EXPORTS "${GENERATED_LINKER_EXPORTS}" PARENT_SCOPE)

    # Make the linker_exports header available before build
    file(TOUCH ${GENERATED_LINKER_EXPORTS})

    add_custom_target(generate_linker_exports ALL
        COMMENT "Generate linkers exports for export address table"
        COMMAND exports_generator # Executable path
        "${ARG_UNDECORATE}" # Undecorate boolean
        "${ARG_FORWARDED_DLL}" # Forwarded DLL path
        "${ARG_DLL_FILES_GLOB}" # Input DLLs
        "${GENERATED_LINKER_EXPORTS}" # Output header
        "${ARG_INPUT_SOURCES_DIR}" # Input sources

        DEPENDS exports_generator
    )

    add_dependencies("${ARG_TARGET}" generate_linker_exports)
endfunction()

function(install_python)
    # https://www.python.org/downloads/release/python-3112/
    set(MY_PYTHON_VERSION 3.11.2)
    set_32_and_64(MY_PYTHON_INSTALLER python-${MY_PYTHON_VERSION}.exe python-${MY_PYTHON_VERSION}-amd64.exe)
    set_32_and_64(MY_PYTHON_INSTALLER_EXPECTED_MD5 2123016702bbb45688baedc3695852f4 4331ca54d9eacdbe6e97d6ea63526e57)

    set(MY_PYTHON_INSTALLER_PATH "${CMAKE_BINARY_DIR}/python-installer/${MY_PYTHON_INSTALLER}")

    # Download installer if necessary
    set(MY_PYTHON_INSTALLER_URL "https://www.python.org/ftp/python/${MY_PYTHON_VERSION}/${MY_PYTHON_INSTALLER}")

    MESSAGE(STATUS "Downloading python installer: ${MY_PYTHON_INSTALLER_URL}")

    file(
        DOWNLOAD ${MY_PYTHON_INSTALLER_URL} "${MY_PYTHON_INSTALLER_PATH}"
        EXPECTED_HASH MD5=${MY_PYTHON_INSTALLER_EXPECTED_MD5}
        SHOW_PROGRESS
    )

    MESSAGE(STATUS "Installing python: ${MY_PYTHON_INSTALLER_PATH}")

    cmake_path(CONVERT "${CMAKE_BINARY_DIR}/python" TO_NATIVE_PATH_LIST MY_PYTHON_INSTALL_DIR NORMALIZE)

    # Install python
    execute_process(
        COMMAND "${MY_PYTHON_INSTALLER_PATH}" /quiet
        InstallAllUsers=0
        "TargetDir=${MY_PYTHON_INSTALL_DIR}"
        AssociateFiles=0
        PrependPath=0
        AppendPath=0
        Shortcuts=0
        Include_doc=0
        Include_debug=1
        Include_dev=1
        Include_exe=0
        Include_launcher=0
        Include_lib=1
        Include_pip=1
        Include_tcltk=1
        Include_test=0
        COMMAND_ERROR_IS_FATAL ANY
        COMMAND_ECHO STDOUT
    )

    set(Python_ROOT_DIR CACHE STRING "${MY_PYTHON_INSTALLER_PATH}")

    find_package(Python REQUIRED COMPONENTS Development)
endfunction()
