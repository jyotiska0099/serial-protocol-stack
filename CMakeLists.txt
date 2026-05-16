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

# ------- Framing Layer -------
add_library(framing
    src/framing/FramingLayer.cpp
)
target_include_directories(framing PUBLIC src/framing)
target_link_libraries(framing PUBLIC physical)

# ------- CRC Layer -------
add_library(crc
    src/crc/CrcLayer.cpp
)
target_include_directories(crc PUBLIC src/crc)

# ------- Session Layer -------
add_library(session
    src/session/SessionLayer.cpp
)
target_include_directories(session PUBLIC src/session)
target_link_libraries(session PUBLIC framing crc)

# ------- Application Layer -------
add_library(application
    src/application/ApplicationLayer.cpp
)
target_include_directories(application PUBLIC src/application)
target_link_libraries(application PUBLIC session)

# ------- Tests -------
add_executable(proto_tests
    tests/PhysicalLayerTest.cpp
    tests/FramingLayerTest.cpp
    tests/CrcLayerTest.cpp
    tests/SessionLayerTest.cpp
    tests/ApplicationLayerTest.cpp
)
target_link_libraries(proto_tests PRIVATE
    physical framing crc session application
    GTest::gtest_main
)
gtest_discover_tests(proto_tests)
