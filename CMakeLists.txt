cmake_minimum_required(VERSION 3.20)

project(PersistentKeyValueStore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(kvstore_lib
        src/KeyValueStore.cpp
)

target_include_directories(kvstore_lib PUBLIC src)

add_executable(kvstore
        src/main.cpp
)

target_link_libraries(kvstore PRIVATE kvstore_lib)

add_executable(kvstore_tests
        tests/KeyValueStoreTests.cpp
)

target_link_libraries(kvstore_tests PRIVATE kvstore_lib)

add_executable(kvstore_benchmark
        benchmarks/KeyValueStoreBenchmark.cpp
)

target_link_libraries(kvstore_benchmark PRIVATE kvstore_lib)

enable_testing()

add_test(
        NAME KeyValueStoreTests
        COMMAND kvstore_tests
)