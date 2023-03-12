cmake_minimum_required(VERSION 3.4.0)
project(libdbase LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)

set(SRC_CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/build")
list(APPEND CMAKE_MODULE_PATH ${SRC_CMAKE_MODULE_PATH})

add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(cli)

