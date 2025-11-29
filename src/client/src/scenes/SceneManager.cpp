/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
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