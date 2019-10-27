cmake_minimum_required(VERSION 3.0.0)

set (CMAKE_CXX_STANDARD 11)

project (zynqmp_userspace_dma_driver)

file(GLOB_RECURSE COMMON_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "lib/xilinx_drivers/*.c")
add_library( zynqmp_userspace_dma_driver_lib STATIC lib/zynqmp_userspace_dma_driver.cpp ${COMMON_SOURCES} )
target_include_directories(zynqmp_userspace_dma_driver_lib PUBLIC lib/xilinx_drivers/common)

add_executable(userspace_dma_test example/userspace_dma_driver_test.cpp)
target_link_libraries( userspace_dma_test zynqmp_userspace_dma_driver_lib )
