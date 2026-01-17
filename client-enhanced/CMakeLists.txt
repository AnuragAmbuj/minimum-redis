cmake_minimum_required(VERSION 3.16)
project(MinimalRedisEnhancedTUI)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

# Enhanced TUI client executable
add_executable(redis-enhanced-tui main.cpp)

# Link libraries
target_link_libraries(redis-enhanced-tui
    Threads::Threads
)

# Include directories
target_include_directories(redis-enhanced-tui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)