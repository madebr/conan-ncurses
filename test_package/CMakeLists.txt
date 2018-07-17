cmake_minimum_required(VERSION 2.8.12)
project(PackageTest C CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(example example.c)
target_link_libraries(example ${CONAN_LIBS})

add_executable(example2 example.cpp)
target_link_libraries(example2 ${CONAN_LIBS})
