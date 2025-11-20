/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** IScene
*/

#ifndef ISCENE_HPP_
#define ISCENE_HPP_

#include "graphics/IWindow.hpp"
#include <SFML/Window/Event.hpp>

class SceneManager;

class IScene
{
public:
    virtual ~IScene() = default;

    virtual void handleEvent(const sf::Event &event) = 0;
    virtual void update() = 0;
    virtual void render(graphics::IWindow *window) = 0;

    void setSceneManager(SceneManager* manager) { _sceneManager = manager; }

protected:
    SceneManager* _sceneManager = nullptr;
};

#endif /* !ISCENE_HPP_ */
