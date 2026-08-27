#include <array>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include "../src/KeyValueStore.h"

namespace
{
    const std::array<std::size_t, 3> OPERATION_COUNTS = {
        1000,
        10000,
        100000
    };

    double elapsedSeconds(
        const std::chrono::steady_clock::time_point& start,
        const std::chrono::steady_clock::time_point& end
    ) {
        return std::chrono::duration<double>(end - start).count();
    }

    double operationsPerSecond(std::size_t operations, double seconds) {
        if (seconds == 0.0) {
            return 0.0;
        }

        return static_cast<double>(operations) / seconds;
    }

    double bytesToMegabytes(std::uintmax_t bytes) {
        return static_cast<double>(bytes) / (1024.0 * 1024.0);
    }

    void printDivider() {
        std::cout << std::string(60, '=') << std::endl;
    }

    void printBenchmarkHeader(const std::string& title) {
        printDivider();
        std::cout << title << std::endl;
        printDivider();
    }

    void setBenchmark(std::size_t operationCount) {
        const std::string databasePath = "benchmark_set_" + std::to_string(operationCount) + ".db";

        std::filesystem::remove(databasePath);
        double seconds = 0.0;
        std::uintmax_t fileSize = 0;

        {
            KeyValueStore store(databasePath);
            const auto start = std::chrono::steady_clock::now();
            for (std::size_t i = 0; i < operationCount; ++i) {
                store.setValue("key" + std::to_string(i),"value" + std::to_string(i));
            }
            const auto end = std::chrono::steady_clock::now();
            seconds = elapsedSeconds(start, end);
            fileSize = std::filesystem::file_size(databasePath);
        }

        std::cout << "Operations: " << operationCount << std::endl;
        std::cout << "Time: " << std::fixed << std::setprecision(4) << seconds << " seconds" << std::endl;
        std::cout << "Throughput: " << std::fixed << std::setprecision(2) <<
            operationsPerSecond(operationCount, seconds) << " ops/sec" << std::endl;
        std::cout << "Database size: " << std::fixed << std::setprecision(4) << bytesToMegabytes(fileSize) <<
            " MB" << std::endl;
        std::filesystem::remove(databasePath);
    }

    void getBenchmark(std::size_t operationCount) {
        const std::string databasePath = "benchmark_get_" + std::to_string(operationCount) + ".db";
        std::filesystem::remove(databasePath);
        double seconds = 0.0;
        std::size_t checksum = 0;
        {
            KeyValueStore store(databasePath);

            for (std::size_t i = 0; i < operationCount; ++i) {
                store.setValue("key" + std::to_string(i),"value" + std::to_string(i));
            }
            const auto start = std::chrono::steady_clock::now();
            for (std::size_t i = 0; i < operationCount; ++i) {
                const std::string value = store.getValue("key" + std::to_string(i));
                checksum += value.size();
            }
            const auto end = std::chrono::steady_clock::now();
            seconds = elapsedSeconds(start, end);
        }

        std::cout << "Operations: " << operationCount << std::endl;
        std::cout << "Time: " << std::fixed << std::setprecision(4) << seconds << " seconds" << std::endl;

        std::cout << "Throughput: " << std::fixed << std::setprecision(2) <<
            operationsPerSecond(operationCount, seconds) << " ops/sec" << std::endl;
        std::cout << "Checksum: " << checksum << std::endl;
        std::filesystem::remove(databasePath);
    }

    void deleteBenchmark(std::size_t operationCount) {
        const std::string databasePath = "benchmark_delete_" + std::to_string(operationCount) + ".db";

        std::filesystem::remove(databasePath);

        double seconds = 0.0;
        std::size_t deletedCount = 0;

        {
            KeyValueStore store(databasePath);

            for (std::size_t i = 0; i < operationCount; ++i) {
                store.setValue("key" + std::to_string(i),"value" + std::to_string(i));
            }

            const auto start = std::chrono::steady_clock::now();

            for (std::size_t i = 0; i < operationCount; ++i) {
                if (store.removeKey("key" + std::to_string(i))) {
                    ++deletedCount;
                }
            }
            const auto end = std::chrono::steady_clock::now();
            seconds = elapsedSeconds(start, end);
        }
        std::cout << "Operations: " << operationCount << std::endl;
        std::cout << "Time: " << std::fixed << std::setprecision(4) << seconds << " seconds" << std::endl;
        std::cout << "Throughput: " << std::fixed << std::setprecision(2) <<
            operationsPerSecond(operationCount, seconds) << " ops/sec" << std::endl;
        std::cout << "Keys deleted: " << deletedCount << std::endl;
        std::filesystem::remove(databasePath);
    }
    void recoveryBenchmark(std::size_t operationCount) {
        const std::string databasePath = "benchmark_recovery_" + std::to_string(operationCount) + ".db";
        std::filesystem::remove(databasePath);
        {
            KeyValueStore store(databasePath);
            for (std::size_t i = 0; i < operationCount; ++i) {
                store.setValue("key" + std::to_string(i),"value" + std::to_string(i));
            }
        }
        double seconds = 0.0;
        std::string lastValue;
        {
            const auto start = std::chrono::steady_clock::now();
            KeyValueStore recoveredStore(databasePath);
            const auto end = std::chrono::steady_clock::now();
            seconds = elapsedSeconds(start, end);
            lastValue = recoveredStore.getValue("key" + std::to_string(operationCount - 1));
        }
        std::cout << "Records replayed: " << operationCount << std::endl;
        std::cout << "Startup time: " << std::fixed << std::setprecision(4) << seconds << " seconds" << std::endl;
        std::cout << "Last value: " << lastValue << std::endl;
        std::filesystem::remove(databasePath);
    }

    void compactionBenchmark() {
        const std::string databasePath = "benchmark_compaction.db";
        constexpr std::size_t overwriteCount = 100000;
        std::filesystem::remove(databasePath);
        {
            KeyValueStore store(databasePath);
            for (std::size_t i = 0; i < overwriteCount; ++i) {
                store.setValue("score",std::to_string(i));
            }
        }
        const auto sizeBefore = std::filesystem::file_size(databasePath);
        double recoveryBeforeSeconds = 0.0;
        {
            const auto start = std::chrono::steady_clock::now();
            KeyValueStore store(databasePath);
            const auto end = std::chrono::steady_clock::now();
            recoveryBeforeSeconds = elapsedSeconds(start, end);
        }
        double compactionSeconds = 0.0;
        {
            KeyValueStore store(databasePath);
            const auto start = std::chrono::steady_clock::now();
            store.compact();
            const auto end = std::chrono::steady_clock::now();
            compactionSeconds = elapsedSeconds(start, end);
        }
        const auto sizeAfter = std::filesystem::file_size(databasePath);
        double recoveryAfterSeconds = 0.0;
        std::string finalValue;

        {
            const auto start = std::chrono::steady_clock::now();
            KeyValueStore store(databasePath);
            const auto end = std::chrono::steady_clock::now();
            recoveryAfterSeconds = elapsedSeconds(start, end);
            finalValue = store.getValue("score");
        }

        std::cout << "Historical SET records: " << overwriteCount << std::endl;

        std::cout << "Live keys after overwrites: 1" << std::endl;

        std::cout << "Database size before compaction: " << std::fixed << std::setprecision(4) <<
            bytesToMegabytes(sizeBefore) << " MB" << std::endl;

        std::cout << "Database size after compaction: " << std::fixed << std::setprecision(4)
            << bytesToMegabytes(sizeAfter) << " MB" << std::endl;

        std::cout << "Recovery time before compaction: " << std::fixed << std::setprecision(6) <<
            recoveryBeforeSeconds << " seconds" << std::endl;

        std::cout << "Compaction time: " << std::fixed << std::setprecision(6) << compactionSeconds << " seconds"
            << std::endl;

        std::cout << "Recovery time after compaction: " << std::fixed << std::setprecision(6) << recoveryAfterSeconds
            << " seconds" << std::endl;

        std::cout << "Final score value: " << finalValue << std::endl;

        const double sizeReduction = sizeBefore == 0 ? 0.0 : (1.0 - static_cast<double>(sizeAfter) /
            static_cast<double>(sizeBefore)) * 100.0;

        std::cout << "Storage reduction: " << std::fixed << std::setprecision(2) << sizeReduction << "%" << std::endl;

        if (recoveryAfterSeconds > 0.0) {
            std::cout << "Recovery speedup: " << std::fixed << std::setprecision(2)
                << recoveryBeforeSeconds / recoveryAfterSeconds << "x" << std::endl;
        }

        std::filesystem::remove(databasePath);
    }
}

int main() {
    std::cout << "Persistent Key-Value Store Benchmarks" << std::endl;
    printBenchmarkHeader("SET Benchmark");

    for (const std::size_t operationCount : OPERATION_COUNTS) {
        setBenchmark(operationCount);
        std::cout << std::endl;
    }

    printBenchmarkHeader("GET Benchmark");

    for (const std::size_t operationCount : OPERATION_COUNTS) {
        getBenchmark(operationCount);
        std::cout << std::endl;
    }

    printBenchmarkHeader("DELETE Benchmark");

    for (const std::size_t operationCount : OPERATION_COUNTS) {
        deleteBenchmark(operationCount);
        std::cout << std::endl;
    }

    printBenchmarkHeader("Recovery Benchmark");

    for (const std::size_t operationCount : OPERATION_COUNTS) {
        recoveryBenchmark(operationCount);
        std::cout << std::endl;
    }

    printBenchmarkHeader(
        "Stale Log / Compaction Benchmark"
    );

    compactionBenchmark();

    std::cout << std::endl;
    printDivider();

    return 0;
}
