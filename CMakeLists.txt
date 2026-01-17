cmake_minimum_required(VERSION 4.0)
project(MinimalRedis)

set(CMAKE_CXX_STANDARD 20)

add_executable(MinimalRedis main.cpp resp.cpp db.cpp commands.cpp
        resp.h db.h commands.h redis_value.h
        net/networks.h)
