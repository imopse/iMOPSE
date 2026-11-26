include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.8.1
)

FetchContent_MakeAvailable(Catch2)

file(GLOB_RECURSE TEST_SOURCES "src/*.h" "src/*.cpp")

add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE imopse Catch2::Catch2WithMain)
