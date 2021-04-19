cmake_minimum_required(VERSION 3.0)
project(examples)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(util/)
include_directories(util_ff/)
include_directories(thirdparty/include)
link_directories(thirdparty/lib)

add_subdirectory(util)
add_subdirectory(util_ff)
add_subdirectory(tools)
add_subdirectory(src EXCLUDE_FROM_ALL)