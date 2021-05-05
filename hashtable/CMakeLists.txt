cmake_minimum_required(VERSION 3.19)
project("hashtable" VERSION 1.0
        DESCRIPTION "A header only c++ library that exposes various hashtable implementations meant to benchmark
                     the real world performance of different hashing schemas"
        HOMEPAGE_URL "https://github.com/andreaskipf/hashing")

# Declare library & directories to include. See http://mariobadr.com/creating-a-header-only-library-with-cmake.html for more info/install instructions
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )

# Link other code from this repo
target_link_libraries(${PROJECT_NAME} INTERFACE convenience reduction thirdparty)

# Make IDE friendly
target_sources(${PROJECT_NAME} INTERFACE hashtable.hpp include/)

# Require c++20 for compilation
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)