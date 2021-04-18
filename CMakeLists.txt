cmake_minimum_required(VERSION 3.19)
project(benchmark)

# Default to release builds
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -march=native -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g") # -fsanitize=thread -fsanitize=address
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-std=c++20)

add_subdirectory(thirdparty)
add_subdirectory(convenience)
add_subdirectory(reduction)
add_subdirectory(hash_functions)
add_subdirectory(learned_models)
add_subdirectory(src)
