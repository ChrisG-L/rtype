/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** LoginScene
*/

#ifndef LOGINSCENE_HPP_
#define LOGINSCENE_HPP_

#include "scenes/IScene.hpp"
#include <SFML/Window/Event.hpp>
#include "graphics/IWindow.hpp"
#include <iostream>

class LoginScene : public IScene
{
public:
    LoginScene();
    ~LoginScene() override = default;

    void handleEvent(const sf::Event &event) override;
    void update() override;
    void render(std::shared_ptr<graphics::IWindow>  window) override;
};

#endif /* !LOGINSCENE_HPP_ */
