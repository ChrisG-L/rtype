/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** main
*/


#include "infrastructure/boostrap/GameBootstrap.hpp"

int main(void) {
    using infrastructure::boostrap::GameBootstrap;
    try
    {
        GameBootstrap gameBoostrap;
        gameBoostrap.launch();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}