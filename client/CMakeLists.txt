cmake_minimum_required(VERSION 3.10)
project(MinimalRedisTUIClient)

set(CMAKE_CXX_STANDARD 20)

# Find ncurses
find_package(Curses REQUIRED)

# Find panel library (usually part of ncurses)
find_library(PANEL_LIBRARY NAMES panel libpanel)
if(NOT PANEL_LIBRARY)
  set(PANEL_LIBRARY "")
endif()

# Client executable
add_executable(redis-tui-client redis_tui_client.cpp)

# Link libraries
target_link_libraries(redis-tui-client ${CURSES_LIBRARIES} ${PANEL_LIBRARY})

# Include directories
target_include_directories(redis-tui-client PRIVATE ${CURSES_INCLUDE_DIRS})

# Set curses defines
target_compile_definitions(redis-tui-client PRIVATE ${CURSES_CFLAGS})