
cmake_minimum_required (VERSION 2.6)
project (CBSON)

add_definitions(-std=c++17)
include_directories("${PROJECT_SOURCE_DIR}")

add_executable(CBSON main.cxx)


