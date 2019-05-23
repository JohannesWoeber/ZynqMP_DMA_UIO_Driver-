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


project (zynqmp_userspace_dma_driver)

file(GLOB_RECURSE COMMON_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "lib/xilinx_drivers/*.c")
add_library( zynqmp_userspace_dma_driver_lib STATIC lib/zynqmp_userspace_dma_driver.cpp ${COMMON_SOURCES} )
target_include_directories(zynqmp_userspace_dma_driver_lib PUBLIC lib/xilinx_drivers/common)
target_include_directories(zynqmp_userspace_dma_driver_lib PUBLIC lib/xilinx_drivers)

add_executable(userspace_dma_test example/userspace_dma_driver_test.cpp)
target_link_libraries( userspace_dma_test zynqmp_userspace_dma_driver_lib )


