cmake_minimum_required(VERSION 3.12)
project(image_diff)

enable_testing()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


add_executable(
    image_diff
    src/main.cpp
    src/image.cpp
    src/error_metrics.cpp
)

# Testing
# enable_testing() # Already added above for clarity, ensure it's processed early.

add_executable(
    image_diff_tests
    tests/test_runner.cpp
    src/image.cpp
    src/error_metrics.cpp
)
# Ensure test_runner can find headers from src/
target_include_directories(image_diff_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_test(
    NAME image_diff_tests_run
    COMMAND image_diff_tests
)
