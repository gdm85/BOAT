cmake_minimum_required(VERSION 2.8)
project(BOAT)

set(CMAKE_CXX_COMPILER g++-5)
set(CMAKE_CXX_FLAGS "-Wall -fexceptions -std=c++14 -O3 -fopenmp")
set(LIBS ${Boost_LIBRARIES} optimization gp probabilistic utilities parameters
        pthread gsl nlopt boost_serialization boost_system boost_coroutine )

#include_directories(SYSTEM $ENV{EIGEN_DIR})
include_directories("../Eigen")

include_directories(${BOAT_SOURCE_DIR})
add_subdirectory(optimization)
add_subdirectory(utilities)
add_subdirectory(probabilistic)
add_subdirectory(parameters)
add_subdirectory(firmament)
