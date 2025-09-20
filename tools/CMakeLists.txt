cmake_minimum_required(VERSION 3.24)

project(KoalaBoxTools LANGUAGES CXX)

### Exports Generator

add_executable(exports_generator src/exports_generator/exports_generator.cpp)
target_link_libraries(exports_generator PRIVATE KoalaBox)

### Sync

add_executable(sync
    src/sync/config.cpp
    src/sync/config.hpp
    src/sync/sync.cpp
)

# needed for find_package(nlohmann_json) to succeed
set(nlohmann_json_DIR ${json_BINARY_DIR})

### https://github.com/pantor/inja
CPMAddPackage(
    URI "gh:pantor/inja@3.4.0"
    OPTIONS
    "INJA_USE_EMBEDDED_JSON OFF"
    "INJA_INSTALL OFF"
    "INJA_EXPORT OFF"
    "BUILD_TESTING OFF"
)
target_link_libraries(sync PRIVATE KoalaBox inja)
