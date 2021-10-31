cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(boutique)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(core)
add_subdirectory(io)
add_subdirectory(protocol)
add_subdirectory(server)
add_subdirectory(test_client)
