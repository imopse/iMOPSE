cmake_minimum_required(VERSION 3.26)
project(paretoAnalyzer)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE PARETO_SOURCES "src/*.h" "src/*.cpp")

add_executable(paretoAnalyzer ${PARETO_SOURCES})
