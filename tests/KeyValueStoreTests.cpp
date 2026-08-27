#include <cassert>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <cstdint>
#include "../src/KeyValueStore.h"

void basicOperationTests() {
    const std::string testFilePath = "test_basic.db";
    std::filesystem::remove(testFilePath);
    {
        KeyValueStore kvs(testFilePath);
        // intial set and verify test
        kvs.setValue("test", "basic");
        assert(kvs.getValue("test") == "basic");

        //override test
        kvs.setValue("test2", "10");
        kvs.setValue("test2", "20");
        assert(kvs.getValue("test2") == "20");

        // missing key test
        bool exception = false;
        try {
            kvs.getValue("missing");
        }
        catch (const std::out_of_range&) {
            exception = true;
        }
        assert(exception);

        //exists test
        kvs.setValue("x", "100");
        assert(kvs.exists("x"));
        assert(!kvs.exists("y"));

        //delete test
        kvs.setValue("delete", "123");
        assert(kvs.exists("delete"));
        kvs.removeKey("delete");
        assert(!kvs.exists("delete"));

        auto oldSize = std::filesystem::file_size(testFilePath);
        bool removed = kvs.removeKey("nonexistent");
        assert(!removed);
        auto newSize = std::filesystem::file_size(testFilePath);
        assert(newSize == oldSize);
    }
    std::filesystem::remove(testFilePath);
}

void persistenceTests() {
    const std::string testFilePath = "test_persistence.db";
    std::filesystem::remove(testFilePath);
    {
        KeyValueStore kvs(testFilePath);
        kvs.setValue("test", "persistence");
        kvs.setValue("empty", "");
    }
    {
        KeyValueStore kvs(testFilePath);

        //Persistence test
        assert(kvs.exists("test"));
        assert(kvs.getValue("test") == "persistence");
        assert(kvs.exists("empty"));
        assert(kvs.getValue("empty") == "");


        kvs.removeKey("test");
        kvs.setValue("delete", "persistence");
    }
    {
        KeyValueStore kvs(testFilePath);
        assert(!kvs.exists("test"));

        assert(kvs.exists("delete"));
        assert(kvs.getValue("delete") == "persistence");
    }
    std::filesystem::remove(testFilePath);
}

void unusualValueTests() {
    const std::string testFilePath = "test_unusual.db";
    std::filesystem::remove(testFilePath);
    {
        KeyValueStore kvs(testFilePath);
        kvs.setValue("tab\ttest", "unusual\ttab\nwow");
        kvs.setValue("hello\nworld", "world hello");
    }
    {
        KeyValueStore kvs(testFilePath);
        assert(kvs.exists("tab\ttest"));
        assert(kvs.getValue("tab\ttest") == "unusual\ttab\nwow");

        assert(kvs.exists("hello\nworld"));
        assert(kvs.getValue("hello\nworld") == "world hello");
    }
    std::filesystem::remove(testFilePath);
}

void compactionTests() {
    const std::string testFilePath = "test_compaction.db";
    std::filesystem::remove(testFilePath);
    {
        KeyValueStore kvs(testFilePath);

        kvs.setValue("test", "10");
        kvs.setValue("test", "12");
        kvs.setValue("test", "14");
        kvs.setValue("test", "16");
        kvs.setValue("test", "18");
        kvs.setValue("test", "20");

        kvs.setValue("Name", "M1");
        kvs.setValue("Name", "M2");

        kvs.setValue("delete", "M0");
        kvs.removeKey("delete");
    }
    auto oldSize = std::filesystem::file_size(testFilePath);
    {
        KeyValueStore kvs(testFilePath);
        kvs.compact();
    }
    auto newSize = std::filesystem::file_size(testFilePath);
    assert(newSize < oldSize);
    {
        KeyValueStore kvs(testFilePath);
        assert(kvs.exists("test"));
        assert(kvs.getValue("test") == "20");
        assert(kvs.exists("Name"));
        assert(kvs.getValue("Name") == "M2");
        assert(!kvs.exists("delete"));
    }
    std::filesystem::remove(testFilePath);
}

void crashRecoveryTest() {
    const std::string testFilePath = "test_recovery.db";
    std::filesystem::remove(testFilePath);
    {
        KeyValueStore kvs(testFilePath);
        kvs.setValue("test", "10");
        kvs.setValue("test1", "12");
        kvs.setValue("test2", "14");
    }
    std::uintmax_t size = std::filesystem::file_size(testFilePath);
    std::uintmax_t byteToRemove = size - 1;
    std::filesystem::resize_file(testFilePath, byteToRemove);
    {
        KeyValueStore kvs(testFilePath);
        assert(kvs.exists("test"));
        assert(kvs.exists("test1"));
        assert(!kvs.exists("test2"));

        kvs.setValue("test3", "16");
    }
    {
        KeyValueStore kvs(testFilePath);
        assert(kvs.exists("test"));
        assert(kvs.getValue("test") == "10");
        assert(kvs.exists("test1"));
        assert(kvs.getValue("test1") == "12");
        assert(kvs.exists("test3"));
        assert(kvs.getValue("test3") == "16");
    }
    std::filesystem::remove(testFilePath);
}

void invalidOperationCorruptionTest() {
    const std::string testFilePath = "test_invalid_operation.db";
    std::filesystem::remove(testFilePath);
    {
        KeyValueStore kvs(testFilePath);
        kvs.setValue("test", "10");
    }
    {
        std::fstream file(testFilePath, std::ios::binary | std::ios::in | std::ios::out);
        assert(file.is_open());
        std::uint8_t invalidOperation = 99;
        file.seekp(0);
        file.write(reinterpret_cast<const char*>(&invalidOperation), sizeof(invalidOperation));
        assert(file);
    }

    auto sizeBefore = std::filesystem::file_size(testFilePath);
    bool exception = false;
    try {
        KeyValueStore kvs(testFilePath);
    }
    catch (const std::runtime_error&) {
        exception = true;
    }

    assert(exception);
    auto sizeAfter = std::filesystem::file_size(testFilePath);
    assert(sizeBefore == sizeAfter);

    std::filesystem::remove(testFilePath);
}

void oversizedLengthCorruptionTest() {
    const std::string testFilePath = "test_oversized_length.db";
    std::filesystem::remove(testFilePath);
    {
        KeyValueStore kvs(testFilePath);
        kvs.setValue("test", "10");
    }
    {
        std::fstream file(testFilePath, std::ios::binary | std::ios::in | std::ios::out);
        assert(file.is_open());
        std::uint64_t corruptedKeySize = 1024ULL * 1024 * 1024; // 1 GB
        file.seekp(sizeof(std::uint8_t));
        file.write(
            reinterpret_cast<const char*>(&corruptedKeySize),
            sizeof(corruptedKeySize)
        );
        assert(file);
    }
    auto sizeBefore = std::filesystem::file_size(testFilePath);
    bool exception = false;
    try {
        KeyValueStore kvs(testFilePath);
    }
    catch (const std::runtime_error&) {
        exception = true;
    }
    assert(exception);
    auto sizeAfter = std::filesystem::file_size(testFilePath);
    assert(sizeBefore == sizeAfter);

    std::filesystem::remove(testFilePath);
}

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Welcome to Key Value Store Testing" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    basicOperationTests();
    persistenceTests();
    unusualValueTests();
    compactionTests();
    crashRecoveryTest();
    invalidOperationCorruptionTest();
    oversizedLengthCorruptionTest();
    std::cout << "All tests passed.\n";
}