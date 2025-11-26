/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** LoginScene
*/

#include "scenes/LoginScene.hpp"
#include "graphics/Graphics.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/GameScene.hpp"
#include "core/Logger.hpp"
#include "utils/Vecs.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Text.hpp>
#include <algorithm>
#include <memory>
#include <spdlog/logger.h>
#include <vector>

LoginScene::LoginScene(std::shared_ptr<core::IRenderer> renderer): _renderer(renderer), _loginAssets{}
{
    client::logging::Logger::getSceneLogger()->debug("LoginScene created");
    graphic::GraphicTexture bgMenuT("assets/login/loginMenuBg.jpg", Vec2f({0, 0}), Vec2f({1, 1}));
    graphic::GraphicTexture inputT("assets/login/loginInput.jpg", Vec2f({0, 0}), Vec2f({1, 1}));
    _loginAssets.push_back(bgMenuT);
    _loginAssets.push_back(inputT);
    _renderer->initialize(std::move(_loginAssets));
}

void LoginScene::handleEvent(const sf::Event &event)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Space) {
            client::logging::Logger::getSceneLogger()->info("Switching to GameScene");
            if (_sceneManager) {
                _sceneManager->changeScene(std::make_unique<GameScene>());
            }
        }
    }
    // if (const auto* keyPressed = event.getIf<sf::Event::TextEntered>()){
    //     if (event.getIf<sf::>()){
    //         s += static_cast<char>(event.text.unicode);
    //     } else {
    //         // Time to consider sf::String or some other unicode-capable string
    //     }
    // }
}

void LoginScene::update(float deltatime)
{
    _renderer->update(deltatime);
}

void LoginScene::render()
{
    _renderer->render();
}
