/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ColorblindShaderManager - Manages colorblind post-processing shader
*/

#ifndef COLORBLINDSHADERRMANAGER_HPP_
#define COLORBLINDSHADERRMANAGER_HPP_

#include "graphics/IWindow.hpp"
#include "accessibility/AccessibilityConfig.hpp"
#include <memory>

namespace accessibility {

class ColorblindShaderManager {
public:
    static ColorblindShaderManager& getInstance();

    /**
     * @brief Initialize shader with window
     * @param window Shared pointer to the graphics window
     * @return true if shaders are available and loaded
     */
    bool initialize(std::shared_ptr<graphics::IWindow> window);

    /**
     * @brief Update shader based on current AccessibilityConfig
     * Call this in the game loop to apply any colorblind mode changes
     */
    void updateFromConfig();

    /**
     * @brief Check if shaders are available
     * @return true if GPU shaders are supported
     */
    bool isAvailable() const;

private:
    ColorblindShaderManager() = default;
    ~ColorblindShaderManager() = default;
    ColorblindShaderManager(const ColorblindShaderManager&) = delete;
    ColorblindShaderManager& operator=(const ColorblindShaderManager&) = delete;

    std::shared_ptr<graphics::IWindow> _window;
    bool _initialized = false;
    bool _shadersAvailable = false;
    ColorBlindMode _currentMode = ColorBlindMode::None;

    static constexpr const char* SHADER_KEY = "colorblind";
    static constexpr const char* VERTEX_SHADER_PATH = "assets/shaders/colorblind.vert";
    static constexpr const char* FRAGMENT_SHADER_PATH = "assets/shaders/colorblind.frag";
};

}

#endif /* !COLORBLINDSHADERRMANAGER_HPP_ */
