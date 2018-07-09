cmake_minimum_required(VERSION 3.7)
project(stream)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O0 -lrt -D_REENTERANT" )

find_package (Threads)

set(SOURCE_FILES main-c-singlethreads.c main.h)
add_executable(stream ${SOURCE_FILES})
target_link_libraries (stream ${CMAKE_THREAD_LIBS_INIT})