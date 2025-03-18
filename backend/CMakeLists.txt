# complier and compiler options
set(CMAKE_CXX_COMPILER /opt/intel/oneapi/compiler/2025.0/bin/icpx)

cmake_minimum_required(VERSION 3.10)

project(waterSim CXX)

set(CMAKE_BUILD_TYPE "debug")

set(COMPILE_FLAGS "-fsycl -g")
set(LINK_FLAGS "-fsycl -g")

add_executable(main src/save_to_file.cpp)

set_target_properties(main PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS})
set_target_properties(main PROPERTIES LINK_FLAGS ${LINK_FLAGS})

find_package(Poco REQUIRED Net)
target_link_libraries(main Poco::Net)