#include "KeyValueStore.h"
#include <fstream>
#include <stdexcept>

KeyValueStore::KeyValueStore(const std::string& path) : filepath(path) {
    loadFromFile();
}

std::string KeyValueStore::getValue(const std::string& key) const {
    return keyValues.at(key);
}

void KeyValueStore::setValue(const std::string& key, const std::string& value) {
    appendFile(Operation::Set, key, value);
    keyValues[key] = value;
}

bool KeyValueStore::removeKey(const std::string& key) {
    if (exists(key)) {
        appendFile(Operation::Delete,key,"");
        return keyValues.erase(key);
    }
    else {
        throw std::runtime_error("Key not found");
    }
}

bool KeyValueStore::exists(const std::string& key) const {
    return keyValues.contains(key);
}

void KeyValueStore::loadFromFile() {
    keyValues.clear();
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::ofstream newFile(filepath, std::ios::binary);
        if (!newFile.is_open()) {
            throw std::runtime_error("Unable to create new file");
        }
        newFile.close();
        return;
    }
    std::uint8_t operationValue;
    std::uint64_t keySize;
    std::string key;
    std::uint64_t valueSize;
    std::string value;

    while (file.read(reinterpret_cast<char*>(&operationValue), sizeof(operationValue))) {
        if (!file.read(reinterpret_cast<char*>(&keySize), sizeof(keySize))) {
            throw std::runtime_error("Corrupted file");
        }
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
        Operation operation = static_cast<Operation>(operationValue);

        if (operation == Operation::Set) {
            keyValues[key] = value;
        }
        if (operation == Operation::Delete) {
            keyValues.erase(key);
        }
    }
}

void KeyValueStore::appendFile(Operation operation, const std::string& key, const std::string& value) const {
    std::ofstream file(filepath, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to save file");
    }

    std::uint8_t operationValue = static_cast<std::uint8_t>(operation);
    std::uint64_t keySize = key.size();
    std::uint64_t valueSize = value.size();

    file.write(reinterpret_cast<char*>(&operationValue), sizeof(operationValue));
    file.write(reinterpret_cast<char*>(&keySize), sizeof(keySize));
    file.write(key.data(), keySize);
    file.write(reinterpret_cast<char*>(&valueSize), sizeof(valueSize));
    file.write(value.data(), valueSize);

    if (!file) {
        throw std::runtime_error("Failed to write to file");
    }
}