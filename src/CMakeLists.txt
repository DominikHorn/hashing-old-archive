set(CMAKE_MINIMUM_REQUIRED_VERSION 3.19)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(test test.cpp)
target_link_libraries(test convenience hashing cxxopts)

add_executable(collisions_hash collisions_hash.cpp)
target_link_libraries(collisions_hash convenience reduction hashing cxxopts)

add_executable(collisions_learned collisions_learned.cpp)
target_link_libraries(collisions_learned convenience reduction learned_models cxxopts)

add_executable(throughput throughput.cpp)
target_link_libraries(throughput convenience reduction hashing cxxopts)
