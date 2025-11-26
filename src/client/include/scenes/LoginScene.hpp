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
#include "../core/IRenderer.hpp"
#include <iostream>

class LoginScene : public IScene
{
    public:
        LoginScene(std::shared_ptr<core::IRenderer>);
        ~LoginScene() override = default;

        void handleEvent(const sf::Event &event) override;
        void update(float deltatime) override;
        void render() override;
    private:
        std::shared_ptr<core::IRenderer> _renderer;
        GraphicAssets _loginAssets;
};

#endif /* !LOGINSCENE_HPP_ */
