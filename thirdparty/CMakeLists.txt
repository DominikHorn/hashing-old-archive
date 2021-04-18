cmake_minimum_required(VERSION 3.19)
project("thirdparty" VERSION 1.0
        DESCRIPTION "Various third party libraries used by our project")

# Declare library & directories to include. See http://mariobadr.com/creating-a-header-only-library-with-cmake.html for more info/install instructions
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )

add_subdirectory(cxxopts)