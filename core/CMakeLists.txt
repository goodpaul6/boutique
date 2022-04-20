set(SOURCES
    logger.cpp
    streambuf.cpp
    const_buffer.cpp
    serialize.cpp
    time_tracker.cpp)

add_library(core ${SOURCES})

target_include_directories(core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
