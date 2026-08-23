#include "KeyValueStore.h"


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
