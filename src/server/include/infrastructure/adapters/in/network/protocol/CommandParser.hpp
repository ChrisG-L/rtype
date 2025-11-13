/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** CommandParser
*/

#ifndef COMMANDPARSER_HPP_
#define COMMANDPARSER_HPP_

#include <sstream>
#include <iostream>
#include "Command.hpp"

namespace infrastructure::adapters::in::network::protocol {
    class CommandParser {
        public:
            static Command parse(const std::string &rawData);
    };
}

#endif /* !COMMANDPARSER_HPP_ */
