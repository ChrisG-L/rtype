/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** SceneManager
*/

#include "scenes/SceneManager.hpp"

SceneManager::SceneManager()
    : _currentScene(nullptr)
{
}

SceneManager::~SceneManager()
{
}

void SceneManager::setContext(const GameContext& ctx)
{
    _context = ctx;
    if (_currentScene) {
        _currentScene->setContext(_context);
    }
}

void SceneManager::changeScene(std::unique_ptr<IScene> newScene)
{
    _currentScene = std::move(newScene);
    if (_currentScene) {
        _currentScene->setSceneManager(this);
        _currentScene->setContext(_context);
    }
}

void SceneManager::handleEvent(const events::Event& event) {
    if (_currentScene)  {
        _currentScene->handleEvent(event);
    }
}

void SceneManager::update(float _deltatime)
{
    if (_currentScene) {
        _currentScene->update(_deltatime);
    }
}

void SceneManager::render()
{
    if (_currentScene)
        _currentScene->render();
}
