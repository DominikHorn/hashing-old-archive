set(CMAKE_MINIMUM_REQUIRED_VERSION 3.19)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(test test.cpp)
target_link_libraries(test convenience hashing)

add_executable(collisions collisions.cpp)
target_link_libraries(collisions convenience hashing)

add_executable(throughput throughput.cpp)
target_link_libraries(throughput convenience hashing)
