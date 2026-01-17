/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** IWindow
*/

#ifndef IWINDOW_HPP_
#define IWINDOW_HPP_

#include <string>
#include <optional>

#include "../utils/Vecs.hpp"
#include "IDrawable.hpp"
#include "../events/Event.hpp"

namespace graphics {

    class IWindow {
        public:
            virtual ~IWindow() = default;

            virtual Vec2u getSize() const = 0;
            virtual bool isOpen() = 0;
            virtual void close() = 0;
            virtual events::Event pollEvent() = 0;

            virtual void draw(const IDrawable& drawable) = 0;
            virtual void drawRect(float x, float y, float width, float height, rgba color) = 0;
            virtual void drawImg(graphics::IDrawable, float x, float y, float scaleX, float scaleY) = 0;
            virtual bool loadTexture(const std::string& key, const std::string& filepath) = 0;
            virtual void drawSprite(const std::string& textureKey, float x, float y, float width, float height) = 0;
            virtual bool loadFont(const std::string& key, const std::string& filepath) = 0;
            virtual void drawText(const std::string& fontKey, const std::string& text, float x, float y, unsigned int size, rgba color) = 0;

            virtual void* getNativeHandle() = 0;
 
            virtual void clear() = 0;
            virtual void display() = 0;

            // Post-processing shader support
            virtual bool loadShader(const std::string& key, const std::string& vertexPath, const std::string& fragmentPath) = 0;
            virtual void setPostProcessShader(const std::string& key) = 0;
            virtual void clearPostProcessShader() = 0;
            virtual void setShaderUniform(const std::string& name, int value) = 0;
            virtual bool supportsShaders() const = 0;

            // Frame management with post-processing
            virtual void beginFrame() = 0;
            virtual void endFrame() = 0;

            // Fullscreen support
            virtual void setFullscreen(bool enabled) = 0;
            virtual void toggleFullscreen() = 0;
            virtual bool isFullscreen() const = 0;

        private:
    };
}

#endif /* !IWINDOW_HPP_ */
