cmake_minimum_required(VERSION 3.24)

project(koalabox-tools LANGUAGES CXX)

include(../cmake/get_cpm.cmake)

### Install glob library

CPMAddPackage(
    NAME Glob
    GIT_REPOSITORY https://github.com/p-ranav/glob.git
    GIT_TAG e796e3ca94e0c514296425d9884e2198c41699ee
)

### Exports Generator executable

add_executable(exports_generator exports_generator.cpp)
target_link_libraries(exports_generator PUBLIC
    KoalaBox
    Glob
)