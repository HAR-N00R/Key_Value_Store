#include <cassert>
#include <iostream>
#include "./Protocol/CommandParser.h"

void parserTest() {
    {
        const std::string testRequest = "Set Alpha 12349876";
        CommandParser parser;
        Command command = parser.parse(testRequest);
        assert(command.type == CommandType::Set);
        assert(command.key == "Alpha");
        assert(command.value == "12349876");
    }
    {
        const std::string testRequest = "get Alpha";
        CommandParser parser;
        Command command = parser.parse(testRequest);
        assert(command.type == CommandType::Get);
        assert(command.key == "Alpha");
    }
    {
        const std::string testRequest = "EXISTS Alpha";
        CommandParser parser;
        Command command = parser.parse(testRequest);
        assert(command.type == CommandType::Exists);
        assert(command.key == "Alpha");
    }
    {
        const std::string testRequest = "DeLete Alpha";
        CommandParser parser;
        Command command = parser.parse(testRequest);
        assert(command.type == CommandType::Delete);
        assert(command.key == "Alpha");
    }
    {
        const std::string testRequest = "comPaCt";
        CommandParser parser;
        Command command = parser.parse(testRequest);
        assert(command.type == CommandType::Compact);
        assert(command.key.empty());
        assert(command.value.empty());
    }
    {
        const std::string testRequest = "Alpha";
        CommandParser parser;
        Command command = parser.parse(testRequest);
        assert(command.type == CommandType::Unknown);
        assert(command.key.empty());
        assert(command.value.empty());
    }
    {
        const std::string testRequest = "set Alpha 123 321 123";
        CommandParser parser;
        Command command = parser.parse(testRequest);
        assert(command.type == CommandType::Set);
        assert(command.key == "Alpha");
        assert(command.value == "123 321 123");
    }
    {
        const std::string testRequest = "GET";
        CommandParser parser;
        Command command = parser.parse(testRequest);
        assert(command.type == CommandType::Unknown);
        assert(command.key.empty());
        assert(command.value.empty());
    }
    {
        const std::string testRequest = "SET Alpha";
        CommandParser parser;
        Command command = parser.parse(testRequest);
        assert(command.type == CommandType::Unknown);
        assert(command.key.empty());
        assert(command.value.empty());
    }
    {
        const std::string testRequest = "get Alpha hello world";
        CommandParser parser;
        Command command = parser.parse(testRequest);
        assert(command.type == CommandType::Unknown);
        assert(command.key.empty());
        assert(command.value.empty());
    }
}

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Welcome to Command Parser Testing" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    parserTest();
    std::cout << "All tests passed.\n";
}
