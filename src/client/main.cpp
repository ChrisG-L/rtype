/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** main
*/

#include "main.hpp"
#include "accessibility/AccessibilityConfig.hpp"

#ifdef _WIN32
    #include <windows.h>
#endif

int main(void) {
    #ifdef _WIN32
        // Configure la console Windows pour utiliser UTF-8
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #endif
    try
    {
        client::logging::Logger::init();

        auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
        if (accessConfig.loadFromFile("assets/accessibility.cfg")) {
            client::logging::Logger::getBootLogger()->info("Accessibility config loaded");
        } else {
            client::logging::Logger::getBootLogger()->info("Using default accessibility settings");
            accessConfig.saveToFile("assets/accessibility.cfg");
        }

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