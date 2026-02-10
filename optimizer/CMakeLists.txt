cmake_minimum_required(VERSION 3.16)
project(iMOPSE)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
     "src/*.h" "src/*.hpp" "src/*.cpp")

add_executable(imopse ${SOURCES})
target_include_directories(imopse PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")
