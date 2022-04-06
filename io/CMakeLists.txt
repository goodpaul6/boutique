set(SOURCES
    socket.cpp
    context.cpp
    helpers.cpp
    timer.cpp
    unix_utils.cpp)

add_library(io ${SOURCES})
