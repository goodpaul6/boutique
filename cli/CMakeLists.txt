set(SOURCES
    main.cpp)

add_executable(cli ${SOURCES})

target_link_libraries(cli PRIVATE core io protocol)
