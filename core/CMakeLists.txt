set(SOURCES
    logger.cpp
    streambuf.cpp)

add_library(core ${SOURCES})

target_include_directories(core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
