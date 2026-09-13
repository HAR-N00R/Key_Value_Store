#ifndef KEYVALUESTORE_H
#define KEYVALUESTORE_H
#include <unordered_map>
#include <string>
#include <fstream>
#include <cstdint>
#include <shared_mutex>


class KeyValueStore {
    private:
    std::string filepath;
    std::unordered_map<std::string, std::string> keyValues;
    mutable std::shared_mutex storeMutex;

    enum class Operation : std::uint8_t {
        Set = 1,
        Delete = 2
    };
    static constexpr std::uint64_t maxKeySize =  1024 * 1024;
    static constexpr std::uint64_t maxValueSize = 16U * 1024 * 1024;

    // File I/O
    void loadFromFile();
    void appendFile(Operation operation, const std::string& key, const std::string& value) const;
    void writeToFile(std::ofstream& file, Operation operation, const std::string& key, const std::string& value) const;

    public:
    explicit KeyValueStore(const std::string& path);
    std::string getValue(const std::string& key) const;
    void setValue(const std::string& key, const std::string& value);
    bool removeKey(const std::string& key);
    bool exists(const std::string& key) const;
    void compact();


};


#endif