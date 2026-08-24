#include "KeyValueStore.h"
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cstdint>


std::string KeyValueStore::getValue(const std::string& key) const {
    return keyValues.at(key);
}

void KeyValueStore::setValue(const std::string& key, const std::string& value) {
    keyValues[key] = value;
}

bool KeyValueStore::removeKey(const std::string& key) {
    return keyValues.erase(key);
}

bool KeyValueStore::exists(const std::string& key) const {
    return keyValues.contains(key);
}

void KeyValueStore::loadFromFile(const std::string& filename) {
    keyValues.clear();
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::ofstream newFile(filename, std::ios::binary);
        if (!newFile.is_open()) {
            throw std::runtime_error("Unable to create new file");
        }
        newFile.close();
        return;
    }
    std::uint64_t keySize;
    std::string key;
    std::uint64_t valueSize;
    std::string value;

    while (file.read(reinterpret_cast<char*>(&keySize), sizeof(keySize))) {
        key.resize(keySize);
        if (!file.read(key.data(), keySize)) {
            throw std::runtime_error("Corrupted file");
        }
        if (!file.read(reinterpret_cast<char*>(&valueSize), sizeof(valueSize))) {
            throw std::runtime_error("Corrupted file");
        }
        value.resize(valueSize);
        if (!file.read(value.data(), valueSize)) {
            throw std::runtime_error("Corrupted file");
        }
        setValue(key, value);
    }

    /*
    std::string line;
    while (std::getline(file, line)) {
        std::string key;
        std::string value;
        importHelper(line, key, value);
        setValue(key, value);
    }
    */
}

void KeyValueStore::saveToFile(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to save file");
    }
    for (const auto& [key, value] : keyValues) {
        std::uint64_t keySize =key.size();
        std::uint64_t valueSize =value.size();
        file.write(reinterpret_cast<char*>(&keySize), sizeof(keySize));
        file.write(key.data(), keySize);
        file.write(reinterpret_cast<char*>(&valueSize), sizeof(valueSize));
        file.write(value.data(), valueSize);
    }
}