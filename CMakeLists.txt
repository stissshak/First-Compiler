# MPL compiler — CMake build.
#
#   cmake -B build                       # configure (Debug by default)
#   cmake --build build                  # -> bin/comp
#   ctest --test-dir build               # run the self-checking test suite
#   cmake --build build --target run     # build + run the structs example
#
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake -B build -DENABLE_SANITIZERS=ON   # ASan + UBSan build of the compiler

cmake_minimum_required(VERSION 3.20)
project(mpl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clang-tidy / IDE tooling

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(ENABLE_SANITIZERS "Build the compiler with AddressSanitizer + UBSan" OFF)

file(GLOB SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(comp ${SOURCES})
target_include_directories(comp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/inc")
target_compile_options(comp PRIVATE -Wall -Wextra -pedantic -Wno-unused-result)

# the binary lands in <repo>/bin/comp
set_target_properties(comp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# convenience: build + run an example (replaces the old `make run`)
add_custom_target(run
    COMMAND comp "${CMAKE_CURRENT_SOURCE_DIR}/examples/structs.mpl" -o "$<TARGET_FILE_DIR:comp>/structs"
    COMMAND "$<TARGET_FILE_DIR:comp>/structs"
    DEPENDS comp
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL)

if(ENABLE_SANITIZERS)
    target_compile_options(comp PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer -g)
    target_link_options(comp PRIVATE -fsanitize=address,undefined)
endif()

# `ctest` drives the existing runner against the freshly built compiler
enable_testing()
add_test(NAME suite
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.sh")
set_tests_properties(suite PROPERTIES
    ENVIRONMENT "COMP=$<TARGET_FILE:comp>"
    FAIL_REGULAR_EXPRESSION "FAIL")
