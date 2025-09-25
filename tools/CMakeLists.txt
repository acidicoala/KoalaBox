cmake_minimum_required(VERSION 3.24)

project(KoalaBoxTools LANGUAGES CXX)

add_subdirectory(lib)

### Windows exports generator

add_executable(windows_exports_generator src/exports_generator/windows_exports_generator.cpp)
target_link_libraries(windows_exports_generator PRIVATE KoalaBoxTools)

### Linux exports generator

add_executable(linux_exports_generator src/exports_generator/linux_exports_generator.cpp)
target_link_libraries(linux_exports_generator PRIVATE KoalaBoxTools)

### Sync
add_executable(sync src/sync/config.cpp src/sync/config.hpp src/sync/sync.cpp)
target_link_libraries(sync PRIVATE KoalaBoxTools)
