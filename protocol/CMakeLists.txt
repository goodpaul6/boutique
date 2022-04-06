set(SOURCES
    binary_protocol.cpp)

set(TEST_SOURCES
    test_main.cpp)

add_library(protocol ${SOURCES})

target_link_libraries(protocol PRIVATE core)

add_executable(test_protocol ${TEST_SOURCES})

target_link_libraries(test_protocol PRIVATE protocol)
