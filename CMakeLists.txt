cmake_minimum_required(VERSION 3.5)
project(imupool_prj LANGUAGES CXX)
#set(CMAKE_VERBOSE_MAKEFILE ON)
add_definitions("-Wall -g")
add_definitions(-std=c++11)
add_executable(demo imupool.cc 
                    imupool.h
                    demo.cc  
                    demo.h )
target_link_libraries(demo pthread)
