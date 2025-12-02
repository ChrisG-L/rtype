/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** GameLoop
*/

#include "core/GameLoop.hpp"
#include "events/Event.hpp"
#include <variant>

namespace core {
    GameLoop::GameLoop(
        std::shared_ptr<graphics::IWindow> window,
        graphics::IGraphicPlugin* _graphicPlugin,
        std::shared_ptr<client::network::TCPClient> tcpClient,
        std::shared_ptr<client::network::UDPClient> udpClient
    ): _deltatime(0.0f), _window(window), _renderer{}, _tcpClient(tcpClient), _udpClient(udpClient)
    {
        _renderer = _graphicPlugin->createRenderer(_window); // DLOpe
        
        _sceneManager = std::make_unique<SceneManager>();
        _sceneManager->setTCPClient(_tcpClient);
        _sceneManager->changeScene(std::make_unique<LoginScene>(_renderer)); // avec le DLOpen
    }

    GameLoop::~GameLoop()
    {
    }

    void GameLoop::run()
    {
        events::Event event;
        // signal.connect([](const Event& event) {
        //     std::visit(overloaded {
        //         [](const KeyPressed& e) { /* handle key */ },
        //         [](auto&) { /* ignore others */ }
        //     }, event);
        // });
        std::cout << "inside RUN " << std::endl;

        while (_window->isOpen()) {
            _deltatime = 0.0;
            if (_deltatime > 0.1f) _deltatime = 0.1f;
            event = _window->pollEvent();
            if (std::holds_alternative<events::KeyPressed>(event)) {
                auto& key = std::get<events::KeyPressed>(event);
                if (key.key == events::Key::B) {
                    std::cout << "key: " << std::endl;
                    _udpClient->movePlayer(10, 15);
                }
            }

            // Update
            _sceneManager->update(_deltatime);

            // Render
            clear();
            _sceneManager->render();
            display();
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
