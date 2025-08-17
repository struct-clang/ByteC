cmake_minimum_required(VERSION 3.10)
project(bytec)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

file(GLOB_RECURSE SRC_FILES src/*.cpp)

include_directories(includes)

add_executable(bytec ${SRC_FILES})

install(TARGETS bytec DESTINATION bin)
