cmake_minimum_required(VERSION 3.2)
project(Seqlock)

set(CMAKE_EXPORT_COMPILE_COMMANDS "true")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall")

link_libraries(pthread)

add_executable(SeqlockExample SeqlockExample.cpp)
add_executable(SeqlockTest SeqlockTest.cpp)

enable_testing()
add_test(SeqlockTest SeqlockTest)
