/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Command
*/

#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include <string>
#include <vector>

struct Command {
    std::string type;
    std::vector<std::string> args;
};

#endif /* !COMMAND_HPP_ */
