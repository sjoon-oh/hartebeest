# HARTEBEEST
# Author: Sukjoon Oh, sjoon@kaist.ac.kr

project(HARTEBEEST)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/extern)

# Test Target
set(DM_TEST_TARGET hartebeest-dm-test.run)
set(PM_TEST_TARGET hartebeest-pm-test.run)
set(RC_TEST_TARGET hartebeest-rc-test.run)

# Device Manager Test
add_executable(
    ${DM_TEST_TARGET} 
    ${CMAKE_SOURCE_DIR}/test/dm-test.cpp
    )
set_target_properties(
    ${DM_TEST_TARGET} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# Protection Domain Test
add_executable(
    ${PM_TEST_TARGET} 
    ${CMAKE_SOURCE_DIR}/test/pm-test.cpp 
    )
set_target_properties(
    ${PM_TEST_TARGET} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# Walkie Talkie Test
add_executable(
    ${RC_TEST_TARGET}
    ${CMAKE_SOURCE_DIR}/test/rc-test.cpp 
)
set_target_properties(
    ${RC_TEST_TARGET} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

target_link_libraries(${DM_TEST_TARGET} PUBLIC ibverbs)
target_link_libraries(${PM_TEST_TARGET} PUBLIC ibverbs)
target_link_libraries(${RC_TEST_TARGET} PUBLIC ibverbs)