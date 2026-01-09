/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ColorblindShaderManager - Implementation
*/

#include "accessibility/ColorblindShaderManager.hpp"
#include "core/Logger.hpp"

namespace accessibility {

ColorblindShaderManager& ColorblindShaderManager::getInstance()
{
    static ColorblindShaderManager instance;
    return instance;
}

bool ColorblindShaderManager::initialize(std::shared_ptr<graphics::IWindow> window)
{
    if (_initialized) {
        return _shadersAvailable;
    }

    _window = window;
    _initialized = true;

    if (!window->supportsShaders()) {
        client::logging::Logger::getEngineLogger()->info(
            "Shaders not supported by graphics backend - colorblind mode will use fallback colors"
        );
        _shadersAvailable = false;
        return false;
    }

    if (!window->loadShader(SHADER_KEY, VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH)) {
        client::logging::Logger::getEngineLogger()->warn(
            "Failed to load colorblind shader - using fallback colors"
        );
        _shadersAvailable = false;
        return false;
    }

    _shadersAvailable = true;
    client::logging::Logger::getEngineLogger()->info("Colorblind shader initialized successfully");

    updateFromConfig();
    return true;
}

void ColorblindShaderManager::updateFromConfig()
{
    if (!_initialized || !_window) {
        return;
    }

    auto& config = AccessibilityConfig::getInstance();
    ColorBlindMode newMode = config.getColorBlindMode();

    if (newMode == _currentMode) {
        return;
    }
    _currentMode = newMode;

    if (!_shadersAvailable) {
        return;
    }

    if (newMode == ColorBlindMode::None) {
        _window->clearPostProcessShader();
    } else {
        _window->setPostProcessShader(SHADER_KEY);
        _window->setShaderUniform("colorBlindMode", static_cast<int>(newMode));
    }
}

bool ColorblindShaderManager::isAvailable() const
{
    return _shadersAvailable;
}

void ColorblindShaderManager::shutdown()
{
    _window.reset();
    _initialized = false;
    _shadersAvailable = false;
    _currentMode = ColorBlindMode::None;
}

}
