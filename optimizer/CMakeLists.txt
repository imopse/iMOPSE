cmake_minimum_required(VERSION 3.16)
project(iMOPSE)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
     "src/*.h" "src/*.hpp" "src/*.cpp")

add_executable(imopse ${SOURCES})
target_include_directories(imopse PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")

set(GPHH_DIR "${CMAKE_CURRENT_LIST_DIR}/../additions/gphh-msrcpsp")

file(GLOB_RECURSE GPHH_ALL CONFIGURE_DEPENDS
     "${GPHH_DIR}/src/*.h" "${GPHH_DIR}/src/*.hpp" "${GPHH_DIR}/src/*.cpp")

set(GPHH_MAIN "")
foreach(f ${GPHH_ALL})
  if(f MATCHES ".*/main\\.cpp$")
    set(GPHH_MAIN ${f})
  endif()
endforeach()

set(GPHH_SOURCES ${GPHH_ALL})
list(FILTER GPHH_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(gphh_msrcpsp STATIC ${GPHH_SOURCES})
target_include_directories(gphh_msrcpsp PUBLIC
  "${GPHH_DIR}"
  "${GPHH_DIR}/src"
  "${GPHH_DIR}/src/domain"
  "${GPHH_DIR}/include"
)

target_link_libraries(imopse PRIVATE gphh_msrcpsp)

if (GPHH_MAIN)
  add_executable(gphh_runner ${GPHH_MAIN})
  target_link_libraries(gphh_runner PRIVATE gphh_msrcpsp)
endif()
