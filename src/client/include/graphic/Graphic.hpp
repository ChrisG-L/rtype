/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Graphic
*/

#ifndef GRAPHIC_HPP_
#define GRAPHIC_HPP_

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <thread>
#include <iostream>

class Graphic {
    public:
        Graphic();
        void run();

    protected:
    private:
        sf::RenderWindow _window;
        void renderingThread(sf::RenderWindow* window);
};
    
    #endif /* !GRAPHIC_HPP_ */
    