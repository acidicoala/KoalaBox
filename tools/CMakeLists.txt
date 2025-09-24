cmake_minimum_required(VERSION 3.24)

project(KoalaBoxTools LANGUAGES CXX)

# https://github.com/jarro2783/cxxopts
CPMAddPackage("gh:jarro2783/cxxopts@3.3.1")

# https://github.com/pantor/inja
set(nlohmann_json_DIR ${json_BINARY_DIR}) # needed for find_package(nlohmann_json) to succeed
CPMAddPackage(
    URI "gh:pantor/inja@3.4.0"
    OPTIONS
    "INJA_USE_EMBEDDED_JSON OFF"
    "INJA_INSTALL OFF"
    "INJA_EXPORT OFF"
    "BUILD_TESTING OFF"
)

### Windows exports generator

add_executable(windows_exports_generator src/exports_generator/windows_exports_generator.cpp)
target_link_libraries(windows_exports_generator PRIVATE KoalaBox)

### Linux exports generator

add_executable(linux_exports_generator src/exports_generator/linux_exports_generator.cpp)
target_link_libraries(linux_exports_generator PRIVATE KoalaBox cxxopts inja)

### Sync
add_executable(sync src/sync/config.cpp src/sync/config.hpp src/sync/sync.cpp)
target_link_libraries(sync PRIVATE KoalaBox inja)
