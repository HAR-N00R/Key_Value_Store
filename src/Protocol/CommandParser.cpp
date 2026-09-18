#include "CommandParser.h"
#include <sstream>
#include <algorithm>
#include <cctype>

Command CommandParser::parse(const std::string& input) const {
    std::stringstream ss(input);
    std::string command;
    std::string key;
    std::string value;
    std::getline(ss, command, ' ');
    std::getline(ss, key, ' ');
    std::streampos pos = ss.tellg();
    if (pos >= 0) {
        value = ss.str().substr(pos);
    }

    std::transform(command.begin(), command.end(), command.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::tolower(c));
    });

    CommandType type;
    if (command == "set" && !key.empty() && !value.empty()) {
        type = CommandType::Set;
    }
    else if (command == "get" && !key.empty() && value.empty()) {
        type = CommandType::Get;
    }
    else if (command == "exists" && !key.empty() && value.empty()) {
        type = CommandType::Exists;
    }
    else if (command == "delete" && !key.empty() && value.empty()) {
        type = CommandType::Delete;
    }
    else if (command == "compact" && key.empty() && value.empty()) {
        type = CommandType::Compact;
    }
    else {
        type = CommandType::Unknown;
        return {type,"",""};
    }
    return {type,key,value};
}
