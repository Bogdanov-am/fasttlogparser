cmake_minimum_required(VERSION 3.15...3.29)
project(${SKBUILD_PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ version selection") 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_CXX_EXTENSIONS OFF) 

find_package(Python COMPONENTS Interpreter Development REQUIRED)

option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Coverage build enabled")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

#pybind11 lib
add_subdirectory (thirdparty/pybind11)
pybind11_add_module(fasttlogparser 
    src/fasttlogparser.cpp
    src/MessageSeries.cpp)
target_include_directories(fasttlogparser PRIVATE src)

#mavlink lib
set(MAVLINK_DIALECT ardupilotmega)
add_subdirectory (thirdparty/mavlink EXCLUDE_FROM_ALL)
add_dependencies(fasttlogparser mavlink)
target_include_directories(fasttlogparser PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/mavlink/include/mavlink/${MAVLINK_DIALECT})
target_include_directories(fasttlogparser PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/mavlink/include/mavlink)

if (NOT WIN32)  
    target_compile_options(fasttlogparser PRIVATE -Wno-address-of-packed-member -Wno-cast-align)  
endif()

install(TARGETS fasttlogparser DESTINATION fasttlogparser)
install(FILES  
    ${CMAKE_CURRENT_SOURCE_DIR}/src/__init__.py
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/fasttlogparser.pyi 
    DESTINATION fasttlogparser)