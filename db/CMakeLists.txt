set(SOURCES
    storage.cpp
    schema.cpp
    collection.cpp)

set(TEST_SOURCES
    test_main.cpp)

add_library(db ${SOURCES})

target_link_libraries(db PRIVATE core)

add_executable(test_db ${TEST_SOURCES})

target_link_libraries(test_db PRIVATE db)
