/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** main
*/

#include "main.hpp"

int main(void) {
    try
    {
        client::logging::Logger::init();
        Boot boot;
        boot.core();
        client::logging::Logger::shutdown();
    }
    catch(const std::exception& e)
    {
        // client::logging::Logger::error("Exception caught: {}", e.what());
        return EXIT_FAILURE;
    }
    return 0;
}