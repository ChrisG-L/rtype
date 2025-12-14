/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** main
*/

#include "main.hpp"
#include "accessibility/AccessibilityConfig.hpp"

int main(void) {
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