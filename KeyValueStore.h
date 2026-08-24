#ifndef KEYVALUESTORE_H
#define KEYVALUESTORE_H
#include <unordered_map>
#include <string>



class KeyValueStore {
    private:
    std::unordered_map<std::string, std::string> keyValues;

    public:
    std::string getValue(const std::string& key) const;
    void setValue(const std::string& key, const std::string& value);
    bool removeKey(const std::string& key);
    bool exists(const std::string& key) const;

    // File I/O
    void loadFromFile(const std::string& filename);
    void saveToFile(const std::string& filename) const;
};


#endif