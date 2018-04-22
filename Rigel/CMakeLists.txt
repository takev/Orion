
cmake_minimum_required (VERSION 2.6)
project (RSON)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

add_definitions(-std=c++17 -Wall -Wstrict-null-sentinel -Weffc++ -Wold-style-cast -Woverloaded-virtual )
include_directories("${PROJECT_SOURCE_DIR}")

enable_testing()

add_executable(RSONEncodeTests RSONEncodeTests.cpp)
target_link_libraries(RSONEncodeTests ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(RSONEncodeTests RSONEncodeTests)

add_executable(RSONDecodeTests RSONDecodeTests.cpp)
target_link_libraries(RSONDecodeTests ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(RSONDecodeTests RSONDecodeTests)


