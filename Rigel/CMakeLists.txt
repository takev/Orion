
cmake_minimum_required (VERSION 2.6)
project (Rigel)

add_definitions(-std=c++17)
add_definitions(-Wall -Wstrict-null-sentinel -Weffc++ -Wold-style-cast -Woverloaded-virtual)
add_definitions(-march=broadwell)
include_directories("${PROJECT_SOURCE_DIR}")

find_package(Boost REQUIRED COMPONENTS system unit_test_framework)
add_library(OrionRigelLibrary SHA512.cpp RunLoop.cpp Application.cpp)

set(ORION_RIGEL_LIBRARIES OrionRigelLibrary ${Boost_SYSTEM_LIBRARY})
set(ORION_RIGEL_TEST_LIBRARIES ${ORION_RIGEL_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_executable(RigelHostServer RigelHostServer.cpp)
target_link_libraries(RigelHostServer ${ORION_RIGEL_LIBRARIES})

enable_testing()

add_executable(RSONEncodeTests RSONEncodeTests.cpp)
target_link_libraries(RSONEncodeTests ${ORION_RIGEL_TEST_LIBRARIES})
add_test(RSONEncodeTests RSONEncodeTests)

add_executable(RSONDecodeTests RSONDecodeTests.cpp)
target_link_libraries(RSONDecodeTests ${ORION_RIGEL_TEST_LIBRARIES})
add_test(RSONDecodeTests RSONDecodeTests)

add_executable(BigIntTests BigIntTests.cpp)
target_link_libraries(BigIntTests ${ORION_RIGEL_TEST_LIBRARIES})
add_test(BigIntTests BigIntTests)

add_executable(SHA512Tests SHA512Tests.cpp)
target_link_libraries(SHA512Tests ${ORION_RIGEL_TEST_LIBRARIES})
add_test(SHA512Tests SHA512Tests)

add_executable(DiffieHellmanTests DiffieHellmanTests.cpp)
target_link_libraries(DiffieHellmanTests ${ORION_RIGEL_TEST_LIBRARIES})
add_test(DiffieHellmanTests DiffieHellmanTests)

add_executable(AES128Tests AES128Tests.cpp)
target_link_libraries(AES128Tests ${ORION_RIGEL_TEST_LIBRARIES})
add_test(AES128Tests AES128Tests)

