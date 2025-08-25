cmake_minimum_required(VERSION 3.24)

project(KoalaBoxTools LANGUAGES CXX)

### Exports Generator

add_executable(exports_generator exports_generator.cpp)

target_link_libraries(exports_generator PRIVATE KoalaBox)

## https://github.com/p-ranav/glob
CPMAddPackage("gh:p-ranav/glob#e796e3ca94e0c514296425d9884e2198c41699ee")
target_link_libraries(exports_generator PRIVATE Glob)

