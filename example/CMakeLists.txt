cmake_minimum_required(VERSION 2.10.0)

project(example)

set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -fPIC")

find_package(Eigen3 REQUIRED)


include_directories(
    ${EIGEN3_INCLUDE_DIRS}
    ../include
)


add_executable(${PROJECT_NAME} src/example.cpp)

