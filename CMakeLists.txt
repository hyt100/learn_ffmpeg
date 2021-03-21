cmake_minimum_required(VERSION 3.0)
project(examples)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(util/)

add_subdirectory(util)
add_subdirectory(src EXCLUDE_FROM_ALL)