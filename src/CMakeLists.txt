add_executable(collisions collisions.cpp)
target_include_directories(collisions PRIVATE
        ${PROJECT_SOURCE_DIR}/convenience
        ${PROJECT_SOURCE_DIR}/hash_functions
        )

add_executable(test test.cpp)
target_include_directories(test PRIVATE
        ${PROJECT_SOURCE_DIR}/convenience
        ${PROJECT_SOURCE_DIR}/hash_functions
        )