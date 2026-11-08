cmake_minimum_required(VERSION 3.20)
project(embedded_practice C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(googletest)

add_library(embedkit 
    lib/bits.c
    lib/ringbuf.c
    lib/fsm.c
    lib/frame_parser.c
    lib/debounce.c
)
target_include_directories(embedkit PUBLIC lib)
target_compile_options(embedkit PRIVATE -Wall -Wextra -Wconversion)

enable_testing()
file(GLOB TESTS tests/*.cpp)
add_executable(unit_tests ${TESTS})
target_link_libraries(unit_tests embedkit GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(unit_tests)
