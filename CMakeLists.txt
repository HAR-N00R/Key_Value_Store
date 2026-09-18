cmake_minimum_required(VERSION 3.20)

project(PersistentKeyValueStore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(kvstore_lib
        src/KeyValueStore/KeyValueStore.cpp
)

target_include_directories(kvstore_lib PUBLIC src)

add_executable(kvstore
        src/main.cpp
        src/Server/Server.cpp
        src/Protocol/CommandParser.cpp
        src/Network/Socket.cpp
)

target_link_libraries(kvstore PRIVATE kvstore_lib)

add_executable(kvstore_tests
        tests/KeyValueStoreTests.cpp
)

target_link_libraries(kvstore_tests PRIVATE kvstore_lib)

add_executable(command_parser_tests
        tests/CommandParserTests.cpp
        src/Protocol/CommandParser.cpp
)

target_include_directories(command_parser_tests PRIVATE src)

add_executable(network_framing_tests
        tests/NetworkFramingTest.cpp
        src/Network/Socket.cpp
        src/Server/Server.cpp
        src/Protocol/CommandParser.cpp
)

target_link_libraries(network_framing_tests PRIVATE kvstore_lib)

add_executable(concurrency_tests
        tests/ServerConcurrencyTests.cpp
        src/Network/Socket.cpp
        src/Server/Server.cpp
        src/Protocol/CommandParser.cpp
)

target_link_libraries(concurrency_tests PRIVATE kvstore_lib)

add_executable(server_shutdown_tests
        tests/ServerShutdownTests.cpp
        src/Network/Socket.cpp
        src/Server/Server.cpp
        src/Protocol/CommandParser.cpp
)

target_link_libraries(server_shutdown_tests PRIVATE kvstore_lib)

add_executable(server_robustness_tests
        tests/ServerRobustnessTests.cpp
        src/Network/Socket.cpp
        src/Server/Server.cpp
        src/Protocol/CommandParser.cpp
)

target_link_libraries(server_robustness_tests PRIVATE kvstore_lib)

enable_testing()

add_test(
        NAME KeyValueStoreTests
        COMMAND kvstore_tests
)

add_test(
        NAME CommandParserTests
        COMMAND command_parser_tests
)

add_test(
        NAME NetworkFramingTests
        COMMAND network_framing_tests
)

add_test(
        NAME ServerConcurrencyTests
        COMMAND concurrency_tests
)

add_test(
        NAME ServerShutdownTests
        COMMAND server_shutdown_tests
)

add_test(
        NAME ServerRobustnessTests
        COMMAND server_robustness_tests
)

set_tests_properties(
        NetworkFramingTests
        ServerConcurrencyTests
        ServerShutdownTests
        ServerRobustnessTests
        PROPERTIES TIMEOUT 150
)