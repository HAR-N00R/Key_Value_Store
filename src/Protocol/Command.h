#ifndef COMMAND_H
#define COMMAND_H
#include <string>

enum class CommandType {
    Set,
    Get,
    Delete,
    Exists,
    Compact,
    Unknown
};
struct Command {
    CommandType type;
    std::string key;
    std::string value;
};

#endif //PERSISTENTKEYVALUESTORE_COMMAND_H