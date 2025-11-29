/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SFMLPlugin
*/

#ifndef SFMLPLUGIN_HPP_
#define SFMLPLUGIN_HPP_

#include <memory>

#include "graphics/IGraphicPlugin.hpp"
#include "../SFMLWindow.hpp"
#include "../SFMLRenderer.hpp"


namespace sfml {
    class SFMLPlugin: public graphics::IGraphicPlugin {
        public:
            SFMLPlugin();

            const char* getName() const override;

            std::shared_ptr<graphics::IWindow> createWindow(
                Vec2u winSize,
                const std::string& name
            ) override;

            std::shared_ptr<core::IRenderer> createRenderer(
                std::shared_ptr<graphics::IWindow> window
            ) override;

        protected:
        private:
    };
}

#endif /* !SFMLPLUGIN_HPP_ */
