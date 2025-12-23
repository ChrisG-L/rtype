/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** GameLoop
*/

#include "core/GameLoop.hpp"
#include "events/Event.hpp"
#include "scenes/LoginScene.hpp"
#include <chrono>
#include <ctime>
#include <variant>

namespace core {
    GameLoop::GameLoop(
        std::shared_ptr<graphics::IWindow> window,
        graphics::IGraphicPlugin* _graphicPlugin,
        std::shared_ptr<client::network::UDPClient> udpClient,
        std::shared_ptr<client::network::TCPClient> tcpClient
    ): _deltatime(0.0f), _window(window), _renderer{}, _udpClient(udpClient), _tcpClient(tcpClient)
    {
        _renderer = _graphicPlugin->createRenderer(_window);

        _sceneManager = std::make_unique<SceneManager>();
        _sceneManager->setContext(GameContext{
            .window = _window,
            .udpClient = _udpClient,
            .tcpClient = _tcpClient
        });
        _sceneManager->changeScene(std::make_unique<LoginScene>());
    }

    GameLoop::~GameLoop()
    {
    }

    void GameLoop::run()
    {
        using Clock = std::chrono::high_resolution_clock;
        using Duration = std::chrono::duration<float>;
        auto previousTime = Clock::now();
        events::Event event;

        while (_window->isOpen()) {
            auto currentTime = Clock::now();
            float deltaTime = Duration(currentTime - previousTime).count();
            deltaTime = std::min(deltaTime, 0.1f);
            previousTime = currentTime;

            while (true) {
                event = _window->pollEvent();
                if (std::holds_alternative<events::None>(event))
                    break;
                _sceneManager->handleEvent(event);
            }
            _sceneManager->update(deltaTime);

            clear();
            _sceneManager->render();
            display();

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    void GameLoop::clear()
    {
        _window->clear();
    }

    void GameLoop::display()
    {
        _window->display();
    }

}
