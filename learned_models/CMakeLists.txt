cmake_minimum_required(VERSION 3.19)
project("learned_models" VERSION 1.0
        DESCRIPTION "A header only c++ library that exposes various learned models that may be used as hash functions.
        Primarily intended to be used for benchmarking learned methods against classical hashes for small, fixed-size keys."
        HOMEPAGE_URL "https://github.com/andreaskipf/hashing")

# Declare library & directories to include. See http://mariobadr.com/creating-a-header-only-library-with-cmake.html for more info/install instructions
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )

# Link other code from this repo
#target_link_libraries(${PROJECT_NAME} INTERFACE convenience)
#target_link_libraries(${PROJECT_NAME} INTERFACE thirdparty)

# Make IDE friendly
target_sources(${PROJECT_NAME} INTERFACE learned_models.hpp)

# Require c++20 for compilation
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)