/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Execute
*/

#include "Execute.hpp"

namespace infrastructure::adapters::in::network::execute {
    Execute::Execute(const Command& cmd): _cmd(cmd)
    {
        if (_cmd.type == "LOGIN") {
            // Login
        } else {
            std::cout << "COMMAND NOT FOUND!" << std::endl;
        }
    }
}


