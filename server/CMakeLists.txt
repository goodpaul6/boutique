set(SOURCES
    main.cpp
    server.cpp
    client_handler.cpp
    dict.cpp)

add_executable(boutique ${SOURCES})

target_link_libraries(boutique PRIVATE core io)
