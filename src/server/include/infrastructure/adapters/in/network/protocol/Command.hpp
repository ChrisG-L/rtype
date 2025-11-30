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
#include <cstdint>

struct Command {
    uint16_t type;
    std::vector<uint8_t> buf;
};

#endif /* !COMMAND_HPP_ */
