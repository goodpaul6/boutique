set(SOURCES
    main.cpp
    server.cpp
    client_handler.cpp)

add_executable(server ${SOURCES})

target_link_libraries(server PRIVATE core db io protocol)
