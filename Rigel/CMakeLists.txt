
cmake_minimum_required (VERSION 2.6)
project (Rigel)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

add_definitions(-std=c++17)
add_definitions(-Wall -Wstrict-null-sentinel -Weffc++ -Wold-style-cast -Woverloaded-virtual)
add_definitions(-march=broadwell)
include_directories("${PROJECT_SOURCE_DIR}")

enable_testing()

add_executable(RSONEncodeTests RSONEncodeTests.cpp)
target_link_libraries(RSONEncodeTests ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(RSONEncodeTests RSONEncodeTests)

add_executable(RSONDecodeTests RSONDecodeTests.cpp)
target_link_libraries(RSONDecodeTests ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(RSONDecodeTests RSONDecodeTests)

add_executable(BigIntTests BigIntTests.cpp)
target_link_libraries(BigIntTests ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(BigIntTests BigIntTests)

add_executable(SHA512Tests SHA512Tests.cpp SHA512.cpp)
target_link_libraries(SHA512Tests ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(SHA512Tests SHA512Tests)

add_executable(DiffieHellmanTests DiffieHellmanTests.cpp SHA512.cpp)
target_link_libraries(DiffieHellmanTests ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(DiffieHellmanTests DiffieHellmanTests)
