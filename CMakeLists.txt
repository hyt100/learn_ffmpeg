cmake_minimum_required(VERSION 3.0)
project(examples)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(util/)
include_directories(util_ff/)
link_directories(thirdparty/)

add_subdirectory(util)
add_subdirectory(util_ff)
add_subdirectory(tools)
add_subdirectory(src EXCLUDE_FROM_ALL)