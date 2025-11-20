/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** SceneManger
*/

#ifndef SCENEMANAGER_HPP_
#define SCENEMANAGER_HPP_

#include "scenes/IScene.hpp"
#include <memory>

class SceneManager
{
public:
    SceneManager();
    ~SceneManager();
    void changeScene(std::unique_ptr<IScene> newScene);
    void handleEvent(const sf::Event &event);
    void update();
    void render(graphics::IWindow *window);

protected:
private:
    std::unique_ptr<IScene> _currentScene;
};

#endif /* !SCENEMANAGER_HPP_ */