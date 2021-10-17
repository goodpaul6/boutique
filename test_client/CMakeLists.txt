set(SOURCES
    main.cpp)

add_executable(test_client ${SOURCES})

target_link_libraries(test_client PRIVATE core io)
