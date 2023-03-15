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
set(JSON_TEST_TARGET hartebeest-json-test.run)
set(FILE_TEST_TARGET hartebeest-file-test.run)
set(COMM_TEST_TARGET hartebeest-comm-test.run)
set(DEMO_TARGET demo.run)

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

# JSON Test
add_executable(
    ${JSON_TEST_TARGET}
    ${CMAKE_SOURCE_DIR}/test/json-test.cpp 
)
set_target_properties(
    ${JSON_TEST_TARGET} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# File Test
add_executable(
    ${FILE_TEST_TARGET}
    ${CMAKE_SOURCE_DIR}/test/file-test.cpp 
)
set_target_properties(
    ${FILE_TEST_TARGET} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# Comm Test
add_executable(
    ${COMM_TEST_TARGET}
    ${CMAKE_SOURCE_DIR}/test/serv-test.cpp 
)
set_target_properties(
    ${COMM_TEST_TARGET} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# Demo
add_executable(
    ${DEMO_TARGET}
    ${CMAKE_SOURCE_DIR}/test/conn-test.cpp 
)
set_target_properties(
    ${DEMO_TARGET} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# Essentials
target_link_libraries(${DM_TEST_TARGET}     PUBLIC ibverbs)
target_link_libraries(${PM_TEST_TARGET}     PUBLIC ibverbs)
target_link_libraries(${RC_TEST_TARGET}     PUBLIC ibverbs)
target_link_libraries(${FILE_TEST_TARGET}   PUBLIC ibverbs)
target_link_libraries(${COMM_TEST_TARGET}   PUBLIC ibverbs)
target_link_libraries(${DEMO_TARGET}        PUBLIC ibverbs)