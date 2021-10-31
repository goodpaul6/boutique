set(SOURCES
    binary_protocol.cpp)

add_library(protocol ${SOURCES})

target_link_libraries(protocol PRIVATE core)
