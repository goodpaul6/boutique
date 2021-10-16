cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(boutique)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Core language
set(SOURCES
    main.cpp
    socket.cpp
    io_context.cpp
    io_helpers.cpp)

add_executable(boutique ${SOURCES})
