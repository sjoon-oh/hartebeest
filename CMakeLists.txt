# HARTEBEEST
# Author: Sukjoon Oh, sjoon@kaist.ac.kr

project(HARTEBEEST)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE true)

include_directories(${CMAKE_SOURCE_DIR}/extern)

message("Processing HARTEBEEST CMakeLists.txt")

message("-- HARTEBEEST Library --")
set(hartebeest_lib      hartebeest)
file(GLOB hartebeest_lib_src 
    CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/src/*.c
    ${CMAKE_SOURCE_DIR}/src/*.cc 
    ${CMAKE_SOURCE_DIR}/includes/*.hh)

add_library(${hartebeest_lib} SHARED ${hartebeest_lib_src})

set_target_properties(
    ${hartebeest_lib} 
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib
)
target_link_libraries(${hartebeest_lib} PUBLIC ibverbs pthread memcached)

#
# Test Binaries
message("-- HARTEBEEST Test Binaries --")
set(hartebeest_demo     run-demo)
add_executable(
    ${hartebeest_demo}
    ${CMAKE_SOURCE_DIR}/test/demo.cc
)
set_target_properties(
    ${hartebeest_demo} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin
)
target_link_libraries(${hartebeest_demo} PUBLIC ${hartebeest_lib})


set(configurator_test     config-test)
add_executable(
    ${configurator_test}
    ${CMAKE_SOURCE_DIR}/test/config-test.cc
)
set_target_properties(
    ${configurator_test} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin
)
target_link_libraries(${configurator_test} PUBLIC ${hartebeest_lib})


set(allocator_test     alloc-test)
add_executable(
    ${allocator_test}
    ${CMAKE_SOURCE_DIR}/test/alloc-test.cc
)
set_target_properties(
    ${allocator_test} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin
)
target_link_libraries(${allocator_test} PUBLIC ${hartebeest_lib})


set(core_test     core-test)
add_executable(
    ${core_test}
    ${CMAKE_SOURCE_DIR}/test/core-test.cc
)
set_target_properties(
    ${core_test} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin
)
target_link_libraries(${core_test} PUBLIC ${hartebeest_lib})


set(memc_test     memc-test)
add_executable(
    ${memc_test}
    ${CMAKE_SOURCE_DIR}/test/memc-test.cc
)
set_target_properties(
    ${memc_test} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin
)
target_link_libraries(${memc_test} PUBLIC ${hartebeest_lib})