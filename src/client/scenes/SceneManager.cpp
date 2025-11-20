/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** SceneManager
*/

#include "scenes/SceneManager.hpp"

SceneManager::SceneManager() : _currentScene(nullptr)
{
}

SceneManager::~SceneManager()
{
}

void SceneManager::changeScene(std::unique_ptr<IScene> newScene)
{
    _currentScene = std::move(newScene);
    if (_currentScene) {
        _currentScene->setSceneManager(this);
        _currentScene->setTCPClient(_tcpClient);
    }
}

void SceneManager::setTCPClient(std::shared_ptr<client::network::TCPClient> tcpClient)
{
    _tcpClient = tcpClient;
}

void SceneManager::handleEvent(const sf::Event &event)
{
    if (_currentScene)
        _currentScene->handleEvent(event);
}

void SceneManager::update()
{
    if (_currentScene)
        _currentScene->update();
}

void SceneManager::render(std::shared_ptr<graphics::IWindow>  window)
{
    if (_currentScene)
        _currentScene->render(window);
}