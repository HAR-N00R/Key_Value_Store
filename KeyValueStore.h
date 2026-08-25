#ifndef KEYVALUESTORE_H
#define KEYVALUESTORE_H
#include <unordered_map>
#include <string>
#include <cstdint>


class KeyValueStore {
    private:
    std::string filepath;
    std::unordered_map<std::string, std::string> keyValues;

    enum class Operation : std::uint8_t {
        Set = 1,
        Delete = 2
    };

    // File I/O
    void loadFromFile();
    void appendFile(Operation operation, const std::string& key, const std::string& value) const;

    public:
    KeyValueStore(const std::string& path);
    std::string getValue(const std::string& key) const;
    void setValue(const std::string& key, const std::string& value);
    bool removeKey(const std::string& key);
    bool exists(const std::string& key) const;

};


#endif