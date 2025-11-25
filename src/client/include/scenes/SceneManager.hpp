/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** SceneManager
*/

#ifndef SCENEMANAGER_HPP_
#define SCENEMANAGER_HPP_

#include "scenes/IScene.hpp"
#include "network/TCPClient.hpp"
#include "../implementations/sfml/SFMLRenderer.hpp"
#include <memory>

class SceneManager
{
    public:
        SceneManager();
        ~SceneManager();
        void changeScene(std::unique_ptr<IScene> newScene);
        void handleEvent(const sf::Event &event);
        void update();
        void render(std::shared_ptr<graphics::IWindow>  window);

        void setTCPClient(std::shared_ptr<client::network::TCPClient> tcpClient);

    private:
        std::unique_ptr<IScene> _currentScene;
        std::shared_ptr<core::IRenderer> _renderer;
        std::shared_ptr<client::network::TCPClient> _tcpClient = nullptr;
};

#endif /* !SCENEMANAGER_HPP_ */