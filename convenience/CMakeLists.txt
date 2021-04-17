cmake_minimum_required(VERSION 3.19)
project("convenience" VERSION 1.0
        DESCRIPTION "A header only c++ library that exposes convenience functionality necessary in the
        benchmarking/database area"
        HOMEPAGE_URL "https://github.com/andreaskipf/hashing")

# Declare library & directories to include. See http://mariobadr.com/creating-a-header-only-library-with-cmake.html for more info/install instructions
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )
target_sources(${PROJECT_NAME} INTERFACE convenience.hpp include/)

# Require c++20 for compilation
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)