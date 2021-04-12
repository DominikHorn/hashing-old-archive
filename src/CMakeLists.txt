set(CMAKE_MINIMUM_REQUIRED_VERSION 3.5.2)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(collisions collisions.cpp)
target_include_directories(collisions PRIVATE
        ${PROJECT_SOURCE_DIR}/convenience
        ${PROJECT_SOURCE_DIR}/hash_functions
        )

add_executable(throughput throughput.cpp)
target_include_directories(throughput PRIVATE
        ${PROJECT_SOURCE_DIR}/convenience
        ${PROJECT_SOURCE_DIR}/hash_functions
        )

add_executable(test test.cpp)
target_include_directories(test PRIVATE
        ${PROJECT_SOURCE_DIR}/convenience
        ${PROJECT_SOURCE_DIR}/hash_functions
        )