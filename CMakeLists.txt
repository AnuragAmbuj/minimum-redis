cmake_minimum_required(VERSION 4.0)
project(MinimalRedis)

set(CMAKE_CXX_STANDARD 20)

# Find GoogleTest
find_package(GTest REQUIRED)

# Enable testing
enable_testing()

# Coverage flags for GCC/Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

# Main executable
add_executable(MinimalRedis main.cpp resp.cpp db.cpp commands.cpp
        resp.h db.h commands.h redis_value.h
        net/networks.h)

# Test executable
add_executable(MinimalRedisTests
        tests/test_main.cpp
        tests/test_resp.cpp
        tests/test_database.cpp
        tests/test_commands.cpp
        resp.cpp db.cpp commands.cpp
        resp.h db.h commands.h redis_value.h
        net/networks.h)

target_link_libraries(MinimalRedisTests GTest::gtest GTest::gtest_main)

# Include source directory for test headers
target_include_directories(MinimalRedisTests PRIVATE ${CMAKE_SOURCE_DIR})

# Add test
add_test(NAME MinimalRedisTests COMMAND MinimalRedisTests)

# Coverage target
add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory coverage
        COMMAND lcov --capture --initial --directory ${CMAKE_BINARY_DIR} --output-file coverage/baseline.info --no-external
        COMMAND MinimalRedisTests
        COMMAND lcov --capture --directory ${CMAKE_BINARY_DIR} --output-file coverage/test.info --no-external
        COMMAND lcov --add-tracefile coverage/baseline.info --add-tracefile coverage/test.info --output-file coverage/total.info
        COMMAND lcov --remove coverage/total.info '/usr/*' '*/tests/*' '*/googletest/*' --output-file coverage/final.info
        COMMAND genhtml coverage/final.info --output-directory coverage/html
        COMMENT "Generating coverage report in coverage/html/index.html"
)
