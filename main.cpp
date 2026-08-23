#include <iostream>
#include <stdexcept>
#include "KeyValueStore.h"

void parsing(std::string input, std::string& command, std::string& key, std::string& value) {

    std::size_t space = input.find(' ');
    if (space != std::string::npos) {
        command = input.substr(0, space);
        input.erase(0, space + 1);
    }
    else {
        command = input;
        return;
    }
    if (input.empty()) {
        throw std::runtime_error("Key missing");
    }
    space = input.find(' ');
    if (space != std::string::npos) {
        key = input.substr(0, space);
        input.erase(0, space + 1);
    }
    else {
        key = input;
        return;
    }
    if (!input.empty()) {
        value = input;
        input.clear();
    }
}

void commandParser(KeyValueStore& kvs) {
    while (true) {
        std::string input;

        std::string command;
        std::string key;
        std::string value;
        std::cout << "Input command: ";
        getline(std::cin, input);
        if (input == "quit" || input == "QUIT") {
            return;
        }
        try {
            parsing(input, command, key, value);
            if (key.empty()) {
                throw std::runtime_error("Invalid command");
            }
            if ((command != "set" && command != "SET") && !value.empty()) {
                throw std::runtime_error("Invalid command");
            }

            if (command == "set" || command == "SET") {
                kvs.setValue(key, value);
            }
            else if (command == "get" || command == "GET") {
                try {
                    std::string value = kvs.getValue(key);
                    std::cout << "Key: " << key << std::endl;
                    std::cout << "Value: " << value << std::endl;
                }
                catch (const std::out_of_range& e) {
                    std::cout << "Key doesn't exist" << std::endl;
                }
            }
            else if (command == "exists" || command == "EXISTS") {
                if (kvs.exists(key)) {
                    std::cout << "Key exists" << std::endl;
                }
                else {
                    std::cout << "Key doesn't exist" << std::endl;
                }
            }
            else if (command == "delete" || command == "DELETE") {
                if (kvs.removeKey(key)) {
                    std::cout << "Key deleted" << std::endl;
                }
                else {
                    std::cout << "Unable to delete" << std::endl;
                }
            }
            else {
                std::cout << "Invalid Command" << std::endl;
            }
        }
        catch (const std::runtime_error& error) {
            std::cout << "Invalid command" << std::endl;
        }
    }
}

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Welcome to Key Value Store" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    KeyValueStore kvs;

    commandParser(kvs);
    return 0;
}