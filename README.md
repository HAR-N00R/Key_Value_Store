# Persistent Key-Value Store

This is a small persistent key-value store written in C++.

The program stores key-value pairs in memory for fast access, while also saving changes to a binary file so the data is still available after restarting the program.

The project supports basic commands such as:

```text
SET username Div
GET username
EXISTS username
DELETE username
COMPACT
QUIT
```

## Features

- Store key-value pairs
- Get values using a key
- Check if a key exists
- Delete keys
- Save data permanently to disk
- Binary file storage
- Append-only log
- Delete tombstones
- Recovery after an incomplete write
- Basic corruption detection
- Database compaction
- Automated tests
- Performance benchmarks

## How It Works

The program has two main parts for storing data.

```text
KeyValueStore
     |
     +---- unordered_map
     |        |
     |        +---- current data in memory
     |
     +---- binary log file
              |
              +---- saved history on disk
```

The `std::unordered_map` stores the current version of every key and value.

The database file stores all successful `SET` and `DELETE` operations.

For example:

```text
SET score 10
SET score 20
SET score 30
```

The file contains all three operations, but the map only keeps:

```text
score -> 30
```

## In-Memory Storage

The current data is stored using:

```cpp
std::unordered_map<std::string, std::string>
```

This gives average O(1) lookup, insertion, and deletion.

This means operations such as `GET` and `EXISTS` can usually be done very quickly without reading from disk.

## Binary File Format

The database uses a binary file instead of a normal text file.

Each record stores:

```text
operation
key size
key
value size
value
```

For example, a SET record is roughly:

```text
[SET][key size][key][value size][value]
```

A DELETE record is roughly:

```text
[DELETE][key size][key][0]
```

The program stores the lengths of the key and value before storing the actual data.

Because of this, values can contain spaces, tabs, newlines, and empty strings without breaking the file format.

## Append-Only Log

The program does not rewrite the whole database after every change.

Instead, new operations are added to the end of the file.

For example:

```text
SET a 10
SET b 20
SET a 30
DELETE b
```

All four operations stay in the file.

The current data in memory becomes:

```text
a -> 30
```

This makes writes simple, but the file becomes larger over time because old data stays in the log.

## Loading the Database

When the program starts, it reads the log from beginning to end.

Each operation is applied to the in-memory map.

For example:

```text
SET a 10
SET b 20
SET a 30
DELETE b
```

After replaying the log, the final state is:

```text
a -> 30
```

This allows the program to rebuild the database after restarting.

## Crash Recovery

The program can recover if the final database record was only partly written.

For example, the file may contain:

```text
complete record
complete record
partial record
```

The program keeps the complete records and removes the incomplete part at the end of the file.

It also checks for some forms of corrupted data, including:

- invalid operation values
- keys that are too large
- values that are too large
- invalid DELETE records

A corrupted record is not automatically deleted unless it looks like an incomplete final write.

## Compaction

Because the database uses an append-only log, old values and deleted keys stay in the file.

For example:

```text
SET score 10
SET score 20
SET score 30
SET name Alice
DELETE name
```

The current database only needs:

```text
score -> 30
```

The `COMPACT` command creates a new database file using only the current live data.

After compaction, the old values and tombstones are removed.

A temporary file is created first so the original database is not immediately overwritten.

## Time Complexity

| Operation | Average Time |
|---|---:|
| GET | O(1) |
| EXISTS | O(1) |
| SET | O(1) average + disk write |
| DELETE | O(1) average + disk write |
| Recovery | O(n) |
| Compaction | O(m) |

`n` is the number of records stored in the log.

`m` is the number of live keys currently stored in the database.

The hash table can have worse performance in the worst case if many keys cause hash collisions.

## Testing

The project includes automated tests for:

- SET and GET
- overwriting values
- missing keys
- EXISTS
- DELETE
- persistence after restart
- deleted keys after restart
- empty values
- tabs and newlines
- compaction
- crash recovery
- writing after recovery
- invalid operations
- corrupted record sizes

The tests use separate database files so they do not modify the normal `data.db` file.

## Benchmarks

I also created benchmarks to measure the storage engine.

Results from one local run:

| Benchmark | 1,000 Operations | 10,000 Operations | 100,000 Operations |
|---|---:|---:|---:|
| SET | 52,933 ops/sec | 60,018 ops/sec | 54,234 ops/sec |
| GET | 5.61M ops/sec | 5.33M ops/sec | 5.01M ops/sec |
| DELETE | 65,369 ops/sec | 59,790 ops/sec | 54,429 ops/sec |
| Recovery | 1.5 ms | 14.2 ms | 151.3 ms |

The exact results depend on the computer and build settings.

### Compaction Benchmark

I overwrote one key 100,000 times.

Before compaction:

```text
Historical records: 100,000
Live keys: 1
Database size: about 2.56 MB
Recovery time: about 128 ms
```

After compaction, only one current SET record was needed.

This showed how an append-only log can grow over time and how compaction can reduce both the database size and the amount of work needed during startup.

## Project Structure

```text
Key_Value_Store/
├── CMakeLists.txt
├── README.md
├── .gitignore
│
├── src/
│   ├── main.cpp
│   ├── KeyValueStore.cpp
│   └── KeyValueStore.h
│
├── tests/
│   └── KeyValueStoreTests.cpp
│
└── benchmarks/
    └── KeyValueStoreBenchmark.cpp
```

## Requirements

- C++20
- CMake
- A C++ compiler such as Clang or GCC

## Build

From the project folder:

```bash
cmake -S . -B build
cmake --build build
```

## Run

Run the main program:

```bash
./build/Key_Value_Store
```

The program creates a file called:

```text
data.db
```

This file stores the database log.

## Run Tests

```bash
./build/KeyValueStoreTests
```

## Run Benchmarks

```bash
./build/KeyValueStoreBenchmark
```

For better benchmark results, use a Release build:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
./build-release/KeyValueStoreBenchmark
```

## Limitations

This project is not meant to be a production database.

Some current limitations are:

- single database file
- single-threaded
- no multiple clients
- no transactions
- no checksums
- no file format versioning
- no network support
- no automatic compaction
- fixed key and value size limits
- full log replay is required when the program starts
- no guarantee that every successful stream write has already reached physical storage

## Future Improvements

Possible future improvements include:

- automatic compaction
- checksums
- stronger crash protection
- thread safety
- multiple clients
- TCP networking
- client/server architecture
- better indexing
- file format versioning

A future version could turn this project into a networked key-value server:

```text
Client
   |
   v
TCP Server
   |
   v
Command Parser
   |
   v
KeyValueStore
   |
   v
Database File
```

## Why I Built This

I built this project to understand more than just basic C++ programs.

It helped me learn how data can be stored in memory and on disk, how an append-only log works, how a database can recover after an incomplete write, how compaction removes old data, and how automated tests and benchmarks can be used to check a storage engine.