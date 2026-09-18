# Concurrent Key-Value Server
[![Build and Test](https://github.com/HAR-N00R/Key_Value_Store/actions/workflows/ci.yml/badge.svg)](https://github.com/HAR-N00R/Key_Value_Store/actions/workflows/ci.yml)

A persistent key-value server written in C++20.

The project started as a basic local key-value store and was later expanded into a TCP server that can handle multiple clients at the same time.

Data is kept in an in-memory hash table for fast access and is also written to a binary log so it can be recovered after restarting the program.

## Features

- TCP client/server architecture
- Multiple concurrent clients
- Thread-safe key-value store
- Persistent binary storage
- Append-only log
- Recovery after restarting
- Delete tombstones
- Database compaction
- Length-prefixed network protocol
- Graceful server shutdown
- Handling for incomplete and oversized network frames
- Protection against disconnected clients
- Automated tests with CTest

## Supported Commands

```text
SET key value
GET key
EXISTS key
DELETE key
COMPACT
```

Example:

```text
SET username Div
GET username
EXISTS username
DELETE username
COMPACT
```

## Architecture

```text
Client
   |
   v
TCP Connection
   |
   v
Server
   |
   v
Command Parser
   |
   v
Key-Value Store
   |
   +---- unordered_map
   |
   +---- Binary Log File
```

The server accepts TCP connections and gives each connected client its own worker thread.

Requests use a 4-byte length prefix so the server knows how many bytes belong to each message.

The command parser converts the received text into commands that are passed to the key-value store.

## Storage

The current values are stored in:

```cpp
std::unordered_map<std::string, std::string>
```

Changes are also written to an append-only binary log.

Each record contains information similar to:

```text
operation
key size
key
value size
value
```

When the program starts, it reads the log and rebuilds the in-memory hash table.

If the last record was only partly written, the store can recover by keeping the valid records before it.

## Concurrency

Multiple clients can access the same key-value store.

The store uses:

```cpp
std::shared_mutex
```

Read operations such as `GET` and `EXISTS` can happen at the same time.

Write operations such as `SET`, `DELETE`, and `COMPACT` require exclusive access.

The server also tracks active client connections so they can be closed during shutdown.

## Compaction

Because the log is append-only, old values and deleted keys remain in the database file.

For example:

```text
SET score 10
SET score 20
SET score 30
DELETE name
```

Compaction rewrites the database using only the current live key-value pairs.

This keeps the database file from growing forever.

## Network Robustness

The server handles several networking problems, including:

- partial TCP reads
- partial TCP writes
- interrupted system calls
- incomplete frame headers
- incomplete frame payloads
- oversized frames
- clients disconnecting unexpectedly
- clients disconnecting while the server is sending data

A bad client connection should only affect that client and not stop the entire server.

## Testing

The project has automated tests for:

- key-value operations
- persistence and recovery
- command parsing
- TCP framing
- fragmented network messages
- concurrent clients
- concurrent writes
- compaction under concurrency
- graceful shutdown
- malformed clients
- oversized and incomplete frames

Run all tests with:

```bash
ctest --test-dir build --output-on-failure
```

The current test suite contains six CTest test groups.

## Project Structure

```text
Key_Value_Store/
├── CMakeLists.txt
├── README.md
│
├── src/
│   ├── main.cpp
│   │
│   ├── KeyValueStore/
│   │   ├── KeyValueStore.cpp
│   │   └── KeyValueStore.h
│   │
│   ├── Network/
│   │   ├── Socket.cpp
│   │   └── Socket.h
│   │
│   ├── Protocol/
│   │   ├── Command.h
│   │   ├── CommandParser.cpp
│   │   └── CommandParser.h
│   │
│   └── Server/
│       ├── Server.cpp
│       └── Server.h
│
└── tests/
    ├── KeyValueStoreTests.cpp
    ├── CommandParserTests.cpp
    ├── NetworkFramingTest.cpp
    ├── ServerConcurrencyTests.cpp
    ├── ServerShutdownTests.cpp
    └── ServerRobustnessTests.cpp
```

## Requirements

- C++20
- CMake
- Clang or GCC
- POSIX sockets

The project was developed and tested on macOS.

## Build

From the project folder:

```bash
cmake -S . -B build
cmake --build build
```

## Run

Start the server with:

```bash
./build/kvstore
```

The server stores its persistent data in:

```text
data.db
```

## Run Tests

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Limitations

This is a learning project and is not meant to be a production database.

Some limitations are:

- one database file
- one worker thread per client
- no authentication
- no transactions
- no checksums
- no automatic compaction
- full log replay during startup
- fixed key, value, and network frame limits
- networking code is currently designed for macOS/POSIX sockets

## What I Learned

This project helped me learn how a storage engine and a network server work together.

I worked with file persistence, binary data, TCP sockets, message framing, threads, mutexes, resource management, error handling, graceful shutdown, and automated testing.