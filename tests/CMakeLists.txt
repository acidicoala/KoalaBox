## https://github.com/catchorg/Catch2
CPMAddPackage("gh:catchorg/Catch2@3.8.0")

add_executable(KoalaBoxTests re_test.cpp)

# Linking the KoalaBox OBJECT target into this executable already pulls in its object files along
# with its usage requirements; adding $<TARGET_OBJECTS:KoalaBox> on top would define them twice.
target_link_libraries(KoalaBoxTests PRIVATE
    KoalaBox
    Catch2::Catch2WithMain
)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(KoalaBoxTests)
