cmake_minimum_required(VERSION 2.8.9)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm64)

set(tools /data2/cdk/cdk-19.3/)
set(CMAKE_C_COMPILER ${tools}/bin/cdk-arm64-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/cdk-arm64-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)





project (userspace_dma)

file(GLOB_RECURSE COMMON_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "common/*.c")
add_executable(userspace_dma userspace_dma_test.cpp xzdma.c xzdma_selftest.c ${COMMON_SOURCES})
target_include_directories(userspace_dma PUBLIC common)


