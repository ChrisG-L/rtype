/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** GameScene
*/

#ifndef GAMESCENE_HPP_
#define GAMESCENE_HPP_

#include "scenes/IScene.hpp"
#include <SFML/Window/Event.hpp>
#include "graphics/IWindow.hpp"
#include <iostream>

class GameScene : public IScene
{
public:
    GameScene();
    ~GameScene() override = default;

    void handleEvent(const sf::Event &event) override;
    void update() override;
    void render() override;
};
#endif /* !GAMESCENE_HPP_ */
