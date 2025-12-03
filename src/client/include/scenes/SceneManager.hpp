/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** SceneManager
*/

#ifndef SCENEMANAGER_HPP_
#define SCENEMANAGER_HPP_

#include "scenes/IScene.hpp"
#include "../core/IRenderer.hpp"
#include "network/TCPClient.hpp"
#include <memory>

class SceneManager
{
    public:
        SceneManager();
        ~SceneManager();
        void changeScene(std::unique_ptr<IScene> newScene);
        void handleEvent(const events::Event& event);
        void update(float deltatime);
        void render();

        void setTCPClient(std::shared_ptr<client::network::TCPClient> tcpClient);

    private:
        std::unique_ptr<IScene> _currentScene;
        std::shared_ptr<core::IRenderer> _renderer;
        std::shared_ptr<client::network::TCPClient> _tcpClient = nullptr;
};

#endif /* !SCENEMANAGER_HPP_ */