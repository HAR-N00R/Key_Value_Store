#include "KeyValueStore.h"
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <iostream>

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
    if (!exists(key)) {
        return false;
    }
    appendFile(Operation::Delete,key,"");
    return keyValues.erase(key);
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

    while (true) {

        std::streampos pos = file.tellg();

        if (!file.read(reinterpret_cast<char*>(&operationValue), sizeof(operationValue))) {
            break;
        }
        Operation operation = static_cast<Operation>(operationValue);
        if (operation != Operation::Set && operation != Operation::Delete) {
            throw std::runtime_error("Corrupted file: invalid operation");
        }

        if (!file.read(reinterpret_cast<char*>(&keySize), sizeof(keySize))) {
            std::filesystem::resize_file(filepath, pos);
            break;
        }
        if (keySize > maxKeySize) {
            throw std::runtime_error("key exceeds maximum size");
        }
        key.resize(keySize);
        if (!file.read(key.data(), keySize)) {
            std::filesystem::resize_file(filepath, pos);
            break;
        }

        if (!file.read(reinterpret_cast<char*>(&valueSize), sizeof(valueSize))) {
            std::filesystem::resize_file(filepath, pos);
            break;
        }
        if (valueSize > maxValueSize) {
            throw std::runtime_error("Value exceeds maximum size");
        }
        if (operation == Operation::Delete && valueSize != 0) {
            throw std::runtime_error("Corrupted Record: oversized operation");
        }
        value.resize(valueSize);
        if (!file.read(value.data(), valueSize)) {
            std::filesystem::resize_file(filepath, pos);
            break;
        }

        if (operation == Operation::Set) {
            keyValues[key] = value;
        }
        else if (operation == Operation::Delete) {
            keyValues.erase(key);
        }
    }
}

void KeyValueStore::appendFile(Operation operation, const std::string& key, const std::string& value) const {
    std::ofstream file(filepath, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to save file");
    }
    writeToFile(file, operation, key, value);
}

void KeyValueStore::writeToFile(std::ofstream& file, Operation operation, const std::string& key,
    const std::string& value) const {

    std::uint8_t operationValue = static_cast<std::uint8_t>(operation);
    std::uint64_t keySize = key.size();
    std::uint64_t valueSize = value.size();

    if (keySize > maxKeySize) {
        throw std::runtime_error("key size exceeds maximum size");
    }
    if (valueSize > maxValueSize) {
        throw std::runtime_error("value size exceeds maximum size");
    }

    file.write(reinterpret_cast<char*>(&operationValue), sizeof(operationValue));
    file.write(reinterpret_cast<char*>(&keySize), sizeof(keySize));
    file.write(key.data(), keySize);
    file.write(reinterpret_cast<char*>(&valueSize), sizeof(valueSize));
    file.write(value.data(), valueSize);

    if (!file) {
        throw std::runtime_error("Failed to write to file");
    }
}

void KeyValueStore::compact() const {
    std::string tempPath = filepath + ".tmp";
    std::string backUpPath = filepath + ".old";
    std::ofstream file(tempPath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to save file");
    }
    for (const auto& [key, value] : keyValues) {
        writeToFile(file,Operation::Set,key,value);
    }
    if (!file) {
        throw std::runtime_error("Failed to write to file");
    }
    file.close();

    std::string fileName = filepath;
    std::filesystem::rename(filepath, backUpPath);
    std::filesystem::rename(tempPath, fileName);
    std::filesystem::remove(backUpPath);

}