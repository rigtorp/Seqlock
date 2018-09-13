cmake_minimum_required(VERSION 3.2)
project(Seqlock CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS "true")
set(CMAKE_CXX_STANDARD 11)

add_library(Seqlock INTERFACE)
target_include_directories(Seqlock INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_compile_options(Seqlock INTERFACE -Wall -Wextra -Wpedantic -Werror)

find_package(Threads REQUIRED)

add_executable(SeqlockExample src/SeqlockExample.cpp)
target_link_libraries(SeqlockExample Seqlock Threads::Threads)

add_executable(SeqlockTest src/SeqlockTest.cpp)
target_link_libraries(SeqlockTest Seqlock Threads::Threads)

enable_testing()
add_test(SeqlockTest SeqlockTest)
