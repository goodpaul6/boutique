set(SOURCES
    socket.cpp
    context.cpp
    helpers.cpp
    timer.cpp
    unix_utils.cpp)

set(TEST_SOURCES
    test_main.cpp)

add_library(io ${SOURCES})

add_executable(test_io ${TEST_SOURCES})

target_link_libraries(test_io PRIVATE core io)
