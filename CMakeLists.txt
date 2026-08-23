cmake_minimum_required(VERSION 4.1)
project(Key_Value_Store)

set(CMAKE_CXX_STANDARD 20)

add_executable(Key_Value_Store main.cpp
        KeyValueStore.cpp
        KeyValueStore.h)
