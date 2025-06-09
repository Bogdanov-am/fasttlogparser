cmake_minimum_required(VERSION 3.15...3.29)
project(${SKBUILD_PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ version selection") 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_CXX_EXTENSIONS OFF) 

find_package(Python 3.10 COMPONENTS Interpreter Development REQUIRED)
add_subdirectory (thirdparty/pybind11)

pybind11_add_module(fasttlogparser 
    src/module.cpp
    src/MessageSeries.cpp)

    
target_compile_options(fasttlogparser PRIVATE -Wno-address-of-packed-member -Wno-cast-align)

target_include_directories(fasttlogparser PRIVATE thirdparty/c_library_v2)

install(TARGETS fasttlogparser DESTINATION .)