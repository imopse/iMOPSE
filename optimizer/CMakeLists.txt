cmake_minimum_required(VERSION 3.16)
project(iMOPSE)

set(CMAKE_CXX_STANDARD 17)

option(BUILD_TESTS "Build tests instead of production executable" OFF)

file(GLOB_RECURSE SOURCES "src/*.h" "src/*.cpp")

include_directories(src)

if (BUILD_TESTS)
    message(STATUS "Building TEST configuration")

    list(FILTER SOURCES EXCLUDE REGEX "/main\\.cpp$")

    add_library(imopse ${SOURCES})

    target_include_directories(imopse PUBLIC src)

    add_compile_definitions(BUILDTEST)

    add_subdirectory(tests)
else()
    message(STATUS "Building PRODUCTION configuration")

    add_executable(imopse ${SOURCES})
endif()
