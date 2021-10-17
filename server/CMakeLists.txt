set(SOURCES
    main.cpp)

add_executable(boutique ${SOURCES})

target_link_libraries(boutique PRIVATE core io)
