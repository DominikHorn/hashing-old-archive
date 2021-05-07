set(CMAKE_MINIMUM_REQUIRED_VERSION 3.19)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(VERBOSE)

add_executable(test test.cpp)
target_link_libraries(test convenience hashtable hashing reduction)

add_executable(throughput_hash throughput_hash.cpp)
target_link_libraries(throughput_hash convenience reduction hashing cxxopts)

add_executable(throughput_learned throughput_learned.cpp)
target_link_libraries(throughput_learned convenience reduction learned_models cxxopts)

add_executable(collisions_hash collisions_hash.cpp)
target_link_libraries(collisions_hash convenience reduction hashing cxxopts)

add_executable(collisions_learned collisions_learned.cpp)
target_link_libraries(collisions_learned convenience reduction learned_models cxxopts)

add_executable(hashtable_hash hashtable_hash.cpp)
target_link_libraries(hashtable_hash convenience hashtable reduction hashing cxxopts)

add_executable(hashtable_learned hashtable_learned.cpp)
target_link_libraries(hashtable_learned convenience hashtable reduction learned_models hashing cxxopts)
