/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** main
*/

#include "main.hpp"

#ifdef _WIN32
    #include <windows.h>
#endif

int main(int argc, char* argv[]) {
    #ifdef _WIN32
        // Configure la console Windows pour utiliser UTF-8
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #endif
    try
    {
        client::logging::Logger::init();
        client::logging::Logger::getBootLogger()->info("Using default accessibility settings (loaded from server after login)");

        {
            Boot boot(argc, argv);
            boot.core();
        } // Boot détruit ici, avant shutdown du logger

        client::logging::Logger::shutdown();
    }
    catch(const std::exception& e)
    {
        // client::logging::Logger::error("Exception caught: {}", e.what());
        return EXIT_FAILURE;
    }
    return 0;
}