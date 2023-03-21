# HARTEBEEST
# Author: Sukjoon Oh, sjoon@kaist.ac.kr

project(HARTEBEEST)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/extern)

# Test Target
set(DEMO_TARGET demo.run)
set(LIB_HARTEBEEST libhartebeest)

# Demo
add_executable(
    ${DEMO_TARGET}
    ${CMAKE_SOURCE_DIR}/test/demo2.cpp 
)
set_target_properties(
    ${DEMO_TARGET} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# Lib
add_library(
    ${LIB_HARTEBEEST} SHARED ${CMAKE_SOURCE_DIR}/src/rdma-conf.cpp
)
set_target_properties(
    ${LIB_HARTEBEEST} 
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)
target_compile_options(
    ${LIB_HARTEBEEST} PRIVATE -Wall -Werror
)

# Essentials
target_link_libraries(${DEMO_TARGET}        PUBLIC ibverbs)
target_link_libraries(${LIB_HARTEBEEST}     PUBLIC ibverbs)