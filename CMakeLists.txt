cmake_minimum_required(VERSION 3.20)
project(serial_protocol_stack LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------- GoogleTest -------
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)
enable_testing()

# ------- Physical Layer -------
add_library(physical
    src/physical/SocketPhysicalLayer.cpp
)
target_include_directories(physical PUBLIC src/physical)

# ------- Tests -------
add_executable(proto_tests
    tests/PhysicalLayerTest.cpp
)
target_link_libraries(proto_tests PRIVATE physical GTest::gtest_main)
gtest_discover_tests(proto_tests)
