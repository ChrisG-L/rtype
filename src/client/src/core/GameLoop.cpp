/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** GameLoop
*/

#include "core/GameLoop.hpp"
#include "events/Event.hpp"
#include "scenes/LoginScene.hpp"
#include "scenes/ConnectionScene.hpp"
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

        // Check initial connection status
        updateConnectionState();

        if (_connectionState.isFullyConnected()) {
            // Already connected, go to LoginScene
            _sceneManager->changeScene(std::make_unique<LoginScene>());
            _wasConnected = true;
        } else {
            // Not connected, show ConnectionScene
            _sceneManager->changeScene(std::make_unique<ConnectionScene>(
                ConnectionSceneMode::InitialConnection
            ));
            _wasConnected = false;
        }
    }

    GameLoop::~GameLoop()
    {
    }

    void GameLoop::updateConnectionState()
    {
        if (_tcpClient) {
            if (_tcpClient->isConnected()) {
                _connectionState.tcp = ConnectionStatus::Connected;
            } else if (_tcpClient->isConnecting()) {
                _connectionState.tcp = ConnectionStatus::Connecting;
            } else {
                _connectionState.tcp = ConnectionStatus::Disconnected;
            }
        }
        if (_udpClient) {
            if (_udpClient->isConnected()) {
                _connectionState.udp = ConnectionStatus::Connected;
            } else if (_udpClient->isConnecting()) {
                _connectionState.udp = ConnectionStatus::Connecting;
            } else {
                _connectionState.udp = ConnectionStatus::Disconnected;
            }
        }
    }

    bool GameLoop::isFullyConnected() const
    {
        return _connectionState.isFullyConnected();
    }

    void GameLoop::checkConnectionStatus()
    {
        updateConnectionState();

        bool currentlyConnected = isFullyConnected();

        // Detect transition from connected to disconnected
        if (_wasConnected && !currentlyConnected && !_connectionOverlayActive) {
            // Lost connection - push reconnection overlay
            _sceneManager->pushScene(std::make_unique<ConnectionScene>(
                ConnectionSceneMode::Reconnection
            ));
            _connectionOverlayActive = true;
        }

        // Detect transition from disconnected to connected (overlay should auto-pop)
        if (!_wasConnected && currentlyConnected && _connectionOverlayActive) {
            // Connection restored - the ConnectionScene will pop itself
            _connectionOverlayActive = false;
        }

        // Track if overlay was popped by the scene itself
        if (_connectionOverlayActive && _sceneManager->sceneCount() == 1) {
            _connectionOverlayActive = false;
        }

        _wasConnected = currentlyConnected;
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

            // Check connection status and handle reconnection
            checkConnectionStatus();

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
