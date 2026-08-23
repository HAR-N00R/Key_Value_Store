#include <iostream>
#include <limits>
#include <stdexcept>
#include "KeyValueStore.h"

void separator() {
    std::cout << std::string(60, '=') << std::endl;
}

template <typename datatype>
void inputValidator(datatype& input) {
    std::cout << "Input : ";
    std::cin >> input;
    while (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "invalid input, Please reenter" << std::endl << "Input: ";
        std::cin >> input;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << std::string(60, '=') << std::endl;
}

void printMessage(const std::string& message) {
    separator();
    std::cout << message << std::endl;
    separator();
}

int menu() {
    while (true) {
        std::cout << "Please select one of the following operations:" << std::endl;
        std::cout << "1. Create new key value pair" << std::endl;
        std::cout << "2. Check if key value pair exists" << std::endl;
        std::cout << "3. Look up key value pair" << std::endl;
        std::cout << "4. Delete key value pair" << std::endl;
        std::cout << "5. Quit" << std::endl;

        int choice;
        inputValidator(choice);

        if (choice >= 1 && choice <= 5) {
            return choice;
        }
        std::cout << "Invalid Choice" << std::endl;
        separator();
    }
}

void addKeyValue(KeyValueStore& kvs) {
    std::cout << "Enter the key and value to be stored" << std::endl;
    std::string key;
    std::string value;
    std::cout << "Key: ";
    getline(std::cin, key);
    std::cout << "Value: ";
    getline(std::cin, value);
    if (!key.empty()) {
        if (kvs.exists(key)) {
            printMessage("Key successfully updated");
        }
        else {
            printMessage("Key successfully created and stored");
        }
        kvs.setValue(key, value);
    }
    else {
        printMessage("Key cannot be empty");
    }
}

void keyExists(const KeyValueStore& kvs) {
    std::cout << "Enter the key to check" << std::endl;
    std::string key;
    std::cout << "Key: ";
    getline(std::cin, key);
    if (!key.empty()) {
        if (kvs.exists(key)) {
            printMessage("Key exists");
            return;
        }
        printMessage("Key doesn't exist");
    }
    else {
        printMessage("Key cannot be empty");
    }
}

void keyLookup(const KeyValueStore& kvs) {
    std::cout << "Enter the key to look up" << std::endl;
    std::string key;
    std::cout << "Key: ";
    getline(std::cin, key);
    if (!key.empty()) {
        try {
            std::string value = "Value: " + kvs.getValue(key);
            printMessage(value);
        }
        catch (const std::out_of_range& e) {
            printMessage("Key doesn't exist");
        }
    }
    else {
        printMessage("Key cannot be empty");
    }
}

void deleteKeyValue(KeyValueStore& kvs) {
    std::cout << "Enter the key to delete" << std::endl;
    std::string key;
    std::cout << "Key: ";
    getline(std::cin, key);
    if (!key.empty()) {
        if (kvs.removeKey(key)) {
            printMessage("Key Value pair deleted");
            return;
        }
        printMessage("Unable to delete");
    }
    else {
        printMessage("Key cannot be empty");
    }
}


int main() {
    printMessage("Welcome to Key Value Store");

    KeyValueStore kvs;

    while (true) {
        switch (menu()) {
        case 1:
            addKeyValue(kvs);
            break;
        case 2:
            keyExists(kvs);
            break;
        case 3:
            keyLookup(kvs);
            break;
        case 4:
            deleteKeyValue(kvs);
            break;
        case 5:
            return 0;
        }
    }
}