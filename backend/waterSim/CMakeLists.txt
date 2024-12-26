# complier and compiler options
set(CMAKE_CXX_COMPILER icpx)

cmake_minimum_required(VERSION 3.10)

project(waterSim CXX)


set(COMPILE_FLAGS "-fsycl")
set(LINK_FLAGS "-fsycl")

add_executable(main src/main.cpp)

set_target_properties(main PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS})
set_target_properties(main PROPERTIES LINK_FLAGS ${LINK_FLAGS})

