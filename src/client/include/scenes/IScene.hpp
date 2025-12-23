/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** IScene
*/

#ifndef ISCENE_HPP_
#define ISCENE_HPP_

#include "graphics/IWindow.hpp"
#include "network/UDPClient.hpp"
#include "network/TCPClient.hpp"
#include "events/Event.hpp"
#include <memory>
#include <string>

class SceneManager;

struct GameContext {
    std::shared_ptr<graphics::IWindow> window;
    std::shared_ptr<client::network::UDPClient> udpClient;
    std::shared_ptr<client::network::TCPClient> tcpClient;
    std::string sessionToken;
};

class IScene
{
public:
    virtual ~IScene() = default;

    virtual void handleEvent(const events::Event& event) = 0;
    virtual void update(float deltatime) = 0;
    virtual void render() = 0;

    void setSceneManager(SceneManager* manager) { _sceneManager = manager; }
    void setContext(const GameContext& ctx) { _context = ctx; }

protected:
    SceneManager* _sceneManager = nullptr;
    GameContext _context;
};

#endif /* !ISCENE_HPP_ */
