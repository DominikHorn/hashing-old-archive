file(GLOB SRCS CONFIGURE_DEPENDS "*.cpp")
add_executable(main ${SRCS})
target_include_directories(main PRIVATE
        ${PROJECT_SOURCE_DIR}/src/include
        ${PROJECT_SOURCE_DIR}/convenience
        ${PROJECT_SOURCE_DIR}/hash_functions
        )