cmake_minimum_required(VERSION 2.8.9)
project (userspace_dma)
add_executable(userspace_dma userspace_dma_test.cpp)
target_include_directories(userspace_dma PUBLIC common)
