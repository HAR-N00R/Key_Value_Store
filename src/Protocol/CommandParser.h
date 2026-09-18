#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H
#include <string>
#include "Command.h"


class CommandParser {
    public:
    Command parse(const std::string& input) const;
};

#endif