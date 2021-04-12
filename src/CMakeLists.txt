set(CMAKE_MINIMUM_REQUIRED_VERSION 3.5.2)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO: this really belongs to hashing.hpp (simd code necessary for meowhash)
# TODO: see if other hash functions also need these flags (xxh3!)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

add_executable(collisions collisions.cpp)
target_include_directories(collisions PRIVATE
        ${PROJECT_SOURCE_DIR}/hash_functions
        )

add_executable(throughput throughput.cpp)
target_include_directories(throughput PRIVATE
        ${PROJECT_SOURCE_DIR}/hash_functions
        )

add_executable(test test.cpp)
target_include_directories(test PRIVATE
        ${PROJECT_SOURCE_DIR}/hash_functions
        )