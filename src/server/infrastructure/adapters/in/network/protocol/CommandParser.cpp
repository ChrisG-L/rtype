/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** CommandParser
*/

#include "infrastructure/adapters/in/network/protocol/CommandParser.hpp"

namespace infrastructure::adapters::in::network::protocol {
    Command CommandParser::parse([[maybe_unused]] const std::string &rawData) {
        Command cmd;
        // std::stringstream ss(rawData);
        // std::string t;
        // char del = '|';
        // std::vector<std::string> sep_command;
        // while (getline(ss, t, del))
        //     sep_command.push_back(t);
        // if (!sep_command.empty()) {
        //     cmd.type = sep_command[0];
        //     cmd.args = sep_command;
        // }
        return cmd;
    }
}
