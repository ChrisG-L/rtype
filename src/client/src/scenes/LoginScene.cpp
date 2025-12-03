/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** LoginScene
*/

#include "scenes/LoginScene.hpp"
#include "gameplay/Missile.hpp"
#include "core/Logger.hpp"
#include <string>

LoginScene::LoginScene(std::shared_ptr<core::IRenderer> renderer)
    : _renderer(renderer)
    , _textures{}
    , _elements{}
    , _entityManager{}
    , _fireCooldown(0.0f)
{
    client::logging::Logger::getSceneLogger()->debug("LoginScene created");

    graphic::GraphicTexture loginMenuT("assets/login/loginMenuBg.jpg", "loginMenu");
    graphic::GraphicElement loginuMenuE(loginMenuT, Vec2f({0, 0}), Vec2f({1, 1}), "loginMenu", graphic::Layer::Background);

    graphic::GraphicTexture spaceshipT("assets/spaceship/Ship1.png", "spaceship");
    graphic::GraphicElement spaceshipE(spaceshipT, Vec2f({100, 300}), Vec2f({1, 1}), "spaceship", graphic::Layer::Player);

    graphic::GraphicTexture missileT("assets/spaceship/missile.png", "missile");

    _textures.push_back(loginMenuT);
    _textures.push_back(missileT);
    _textures.push_back(spaceshipT);

    _elements.push_back(spaceshipE);
    _elements.push_back(loginuMenuE);

    _renderer->initialize(_textures, _elements);

    graphic::GraphicTexture* bgTexture = findTexture("loginMenu");
    graphic::GraphicElement* bgElement = findElement("loginMenu");

    if (bgTexture && bgElement) {
        float scaleX = 1920.0f / bgTexture->getSize().x;
        float scaleY = 1080.0f / bgTexture->getSize().y;
        bgElement->setScale({scaleX, scaleY});
    }
}

graphic::GraphicTexture* LoginScene::findTexture(const std::string& name)
{
    for (auto& asset : _textures) {
        auto& texture = std::get<graphic::GraphicTexture>(asset);
        if (texture.getName() == name)
            return &texture;
    }
    return nullptr;
}

graphic::GraphicElement* LoginScene::findElement(const std::string& name)
{
    for (auto& asset : _elements) {
        auto& elem = std::get<graphic::GraphicElement>(asset);
        if (elem.getName() == name)
            return &elem;
    }
    return nullptr;
}

void LoginScene::handleEvent(const events::Event& event)
{
    handleInput(event);
}

void LoginScene::handleInput(const events::Event& event)
{
    if (std::holds_alternative<events::KeyPressed>(event)) {
        auto& key = std::get<events::KeyPressed>(event);
        _pressedKeys.insert(key.key);
    }
    if (std::holds_alternative<events::KeyReleased>(event)) {
        auto& key = std::get<events::KeyReleased>(event);
        _pressedKeys.erase(key.key);
    }
}

void LoginScene::spawnMissile()
{
    auto* missileTexture = findTexture("missile");
    auto* spaceship = findElement("spaceship");

    if (!missileTexture || !spaceship)
        return;

    Vec2f spawnPos = spaceship->getPos();
    spawnPos.x += 50.0f;
    spawnPos.y += 15.0f;

    std::string missileName = "missile " + std::to_string(_entitiesIds);
    _entityManager.spawn<gameplay::Missile>(*missileTexture, missileName, spawnPos, 1000.0f);
    _entitiesIds += 1;
}

void LoginScene::update(float deltaTime)
{
    if (auto* spaceship = findElement("spaceship")) {
        Vec2f pos = spaceship->getPos();
        float movement = _moveSpeed * deltaTime;

        if (_pressedKeys.contains(events::Key::Up))
            pos.y -= movement;
        if (_pressedKeys.contains(events::Key::Down))
            pos.y += movement;
        if (_pressedKeys.contains(events::Key::Left))
            pos.x -= movement;
        if (_pressedKeys.contains(events::Key::Right))
            pos.x += movement;

        spaceship->setPos(pos);
    }

    if (_fireCooldown > 0)
        _fireCooldown -= deltaTime;

    if (_pressedKeys.contains(events::Key::Space) && _fireCooldown <= 0) {
        spawnMissile();
        _fireCooldown = FIRE_RATE;
    }

    _entityManager.update(deltaTime);
    _entityManager.cleanup(_entitiesIds);
    _entityManager.syncToGraphics(_elements);

    _renderer->update(deltaTime, _textures, _elements);
}

void LoginScene::render()
{
    _renderer->render();
}
