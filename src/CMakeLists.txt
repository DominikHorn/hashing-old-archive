file(GLOB SRCS CONFIGURE_DEPENDS "*.hpp" "*.cpp")
add_executable(main ${SRCS})

target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/src/include)

# Configure XXHASH
option(BUILD_SHARED_LIBS "Build shared libs" OFF) #optional
set(XXHASH_BUILD_ENABLE_INLINE_API OFF) #optional
set(XXHASH_BUILD_XXHSUM OFF) #optional
add_subdirectory(${CMAKE_SOURCE_DIR}/opensource_references/xxHash/cmake_unofficial ${CMAKE_SOURCE_DIR}/opensource_references/xxHash/cmake_unofficial/build EXCLUDE_FROM_ALL)
target_link_libraries(main PRIVATE xxHash::xxhash)