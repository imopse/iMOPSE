cmake_minimum_required (VERSION 2.8.11)
project (Myproject_all)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

# Set compiler flags and options.
# enable the Visual Studio warning level to 4
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

# Sub-directories where more CMakeLists.txt exist
add_subdirectory (optimizer)
add_subdirectory (paretoAnalyzer)