cmake_minimum_required(VERSION 3.16)
project(MinimalRedisTUI)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

# Unified TUI client executable
add_executable(redis-tui redis_tui.cpp redis_client.cpp)

# Link libraries
target_link_libraries(redis-tui
    Threads::Threads
)

# Include directories
target_include_directories(redis-tui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)