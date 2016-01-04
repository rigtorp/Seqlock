cmake_minimum_required(VERSION 3.2)
project(Seqlock)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall")

add_executable(fuzz fuzz.cpp)
target_link_libraries(fuzz -lpthread)