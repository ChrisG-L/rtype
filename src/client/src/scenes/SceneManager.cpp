/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** SceneManager
*/

#include "scenes/SceneManager.hpp"

SceneManager::SceneManager()
    : _pendingScene(nullptr)
    , _pendingAction(PendingAction::None)
{
}

SceneManager::~SceneManager()
{
    // Clear the stack properly
    while (!_sceneStack.empty()) {
        _sceneStack.pop();
    }
}

void SceneManager::setContext(const GameContext& ctx)
{
    _context = ctx;
    // Update context for all scenes in the stack
    std::stack<std::unique_ptr<IScene>> tempStack;
    while (!_sceneStack.empty()) {
        auto& scene = _sceneStack.top();
        scene->setContext(_context);
        tempStack.push(std::move(_sceneStack.top()));
        _sceneStack.pop();
    }
    while (!tempStack.empty()) {
        _sceneStack.push(std::move(tempStack.top()));
        tempStack.pop();
    }
}

void SceneManager::initScene(IScene* scene)
{
    if (scene) {
        scene->setSceneManager(this);
        scene->setContext(_context);
    }
}

IScene* SceneManager::currentScene() const
{
    if (_sceneStack.empty()) {
        return nullptr;
    }
    return _sceneStack.top().get();
}

void SceneManager::changeScene(std::unique_ptr<IScene> newScene)
{
    _pendingScene = std::move(newScene);
    _pendingAction = PendingAction::Change;
}

void SceneManager::pushScene(std::unique_ptr<IScene> scene)
{
    _pendingScene = std::move(scene);
    _pendingAction = PendingAction::Push;
}

void SceneManager::popScene()
{
    _pendingAction = PendingAction::Pop;
}

bool SceneManager::hasOverlay() const
{
    return _sceneStack.size() > 1;
}

size_t SceneManager::sceneCount() const
{
    return _sceneStack.size();
}

void SceneManager::applyPendingActions()
{
    if (_pendingAction == PendingAction::None) {
        return;
    }

    switch (_pendingAction) {
        case PendingAction::Change:
            // Clear entire stack and push new scene
            while (!_sceneStack.empty()) {
                _sceneStack.pop();
            }
            if (_pendingScene) {
                initScene(_pendingScene.get());
                _sceneStack.push(std::move(_pendingScene));
            }
            break;

        case PendingAction::Push:
            // Push new scene on top
            if (_pendingScene) {
                initScene(_pendingScene.get());
                _sceneStack.push(std::move(_pendingScene));
            }
            break;

        case PendingAction::Pop:
            // Pop top scene
            if (!_sceneStack.empty()) {
                _sceneStack.pop();
            }
            break;

        case PendingAction::None:
            break;
    }

    _pendingScene = nullptr;
    _pendingAction = PendingAction::None;
}

void SceneManager::handleEvent(const events::Event& event)
{
    IScene* scene = currentScene();
    if (scene) {
        scene->handleEvent(event);
    }
}

void SceneManager::update(float deltatime)
{
    applyPendingActions();
    IScene* scene = currentScene();
    if (scene) {
        scene->update(deltatime);
    }
}

void SceneManager::render()
{
    // Render all scenes from bottom to top for proper layering
    // First, collect scenes in order
    std::vector<IScene*> scenesToRender;
    std::stack<std::unique_ptr<IScene>> tempStack;

    while (!_sceneStack.empty()) {
        scenesToRender.push_back(_sceneStack.top().get());
        tempStack.push(std::move(_sceneStack.top()));
        _sceneStack.pop();
    }

    // Restore stack
    while (!tempStack.empty()) {
        _sceneStack.push(std::move(tempStack.top()));
        tempStack.pop();
    }

    // Render from bottom to top (reverse order since we collected top-first)
    for (auto it = scenesToRender.rbegin(); it != scenesToRender.rend(); ++it) {
        if (*it) {
            (*it)->render();
        }
    }
}
