/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** SceneManager
*/

#ifndef SCENEMANAGER_HPP_
#define SCENEMANAGER_HPP_

#include "scenes/IScene.hpp"
#include "../core/IRenderer.hpp"
#include <memory>

class SceneManager
{
    public:
        SceneManager();
        ~SceneManager();
        void setContext(const GameContext& ctx);
        void changeScene(std::unique_ptr<IScene> newScene);
        void handleEvent(const events::Event& event);
        void update(float deltatime);
        void render();

    private:
        std::unique_ptr<IScene> _currentScene;
        std::shared_ptr<core::IRenderer> _renderer;
        GameContext _context;
};

#endif /* !SCENEMANAGER_HPP_ */
