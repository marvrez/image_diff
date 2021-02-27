cmake_minimum_required(VERSION 3.12)
project(image_diff)

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
