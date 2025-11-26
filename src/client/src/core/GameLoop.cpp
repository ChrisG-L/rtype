/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** GameLoop
*/

#include "core/GameLoop.hpp"

namespace core {
    GameLoop::GameLoop(
        std::shared_ptr<graphics::IWindow> window,
        std::shared_ptr<client::network::TCPClient> tcpClient
    ): _deltatime(0.0f), _window(window), _renderer{}, _tcpClient(tcpClient)
    {
        // _renderer = std::make_shared<SFMLRenderer>(_window); // DLOpe
        
        _sceneManager = std::make_unique<SceneManager>();
        _sceneManager->setTCPClient(_tcpClient);
        // _sceneManager->changeScene(std::make_unique<LoginScene>(_renderer)); // avec le DLOpen
    }

    GameLoop::~GameLoop()
    {
    }

    void GameLoop::run()
    {
    
        while (_window->isOpen()) {
            _deltatime = 0.0;
            if (_deltatime > 0.1f) _deltatime = 0.1f;
            while (auto pEvent = _window->pollEvent()) {
                // if (pEvent->is<Closed>()) {
                    // std::cout << "closed!" << std::endl;
                // }
                // if (pEvent->is<evt::Event::>()) {
                //     _window->close();
                //     return;
                // }
                // _sceneManager->handleEvent(*pEvent.value());
            }

            // Update
            _sceneManager->update(_deltatime);

            // Render
            clear();
            _sceneManager->render(_window);
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
