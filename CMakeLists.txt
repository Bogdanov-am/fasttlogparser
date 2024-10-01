cmake_minimum_required(VERSION 3.15...3.29)
project(${SKBUILD_PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ version selection")  # or 11, 14, 17, 20
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # optional, ensure standard is supported
set(CMAKE_CXX_EXTENSIONS OFF)  # optional, keep compiler extensions off

find_package(Python 3.11 COMPONENTS Interpreter Development REQUIRED)
add_subdirectory (thirdparty/pybind11)

pybind11_add_module(fasttlogparser 
    src/module.cpp
    src/MessageSeries.cpp)

target_include_directories(fasttlogparser PRIVATE thirdparty/c_library_v2)

install(TARGETS fasttlogparser DESTINATION .)