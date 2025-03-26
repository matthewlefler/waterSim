execute_process(COMMAND bash -c "source /opt/intel/oneapi/setvars.sh --force")

# complier and compiler options
set(CMAKE_CXX_COMPILER /opt/intel/oneapi/compiler/2025.0/bin/icpx)

cmake_minimum_required(VERSION 3.10)

project(waterSim CXX)

find_package(Poco REQUIRED Net)

# amd gpu specific compile/link command arguments
set(COMPILE_FLAGS_AMD "-fsycl -fast -fsycl-targets=amdgcn-amd-amdhsa -Xsycl-target-backend --offload-arch=gfx1102")
set(LINK_FLAGS_AMD "-fsycl -fast -fsycl-targets=amdgcn-amd-amdhsa -Xsycl-target-backend --offload-arch=gfx1102")

# regular specific compile/link command arguments
set(COMPILE_FLAGS "-fsycl -fast")
set(LINK_FLAGS "-fsycl -fast")

# amd gpu specific executables 
add_executable(save_to_file_amd src/save_to_file.cpp)

set_target_properties(save_to_file_amd PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS_AMD})
set_target_properties(save_to_file_amd PROPERTIES LINK_FLAGS ${LINK_FLAGS_AMD})

target_link_libraries(save_to_file_amd ${CMAKE_SOURCE_DIR}/libraries/libPocoNet.a)

# add_executable(main_amd src/main.cpp)

# set_target_properties(main_amd PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS_AMD})
# set_target_properties(main_amd PROPERTIES LINK_FLAGS ${LINK_FLAGS_AMD})

# target_link_libraries(main_amd Poco::Net)

# regular executables
add_executable(save_to_file src/save_to_file.cpp)

set_target_properties(save_to_file PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS})
set_target_properties(save_to_file PROPERTIES LINK_FLAGS ${LINK_FLAGS})

target_link_libraries(save_to_file ${CMAKE_SOURCE_DIR}/libraries/libPocoNet.a)

# add_executable(main src/main.cpp)

# set_target_properties(main PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS})
# set_target_properties(main PROPERTIES LINK_FLAGS ${LINK_FLAGS})

# target_link_libraries(main Poco::Net)