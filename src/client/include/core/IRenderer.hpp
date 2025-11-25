/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** IRenderer
*/

#ifndef IRENDERER_HPP_
#define IRENDERER_HPP_

#include "../graphics/IWindow.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <variant>
#include <vector>
#include "../graphics/Asset.hpp"

typedef std::vector<Asset> GraphicAssets;

namespace core {
    class IRenderer {
        public:
            virtual ~IRenderer() = default;

            virtual void initialize(GraphicAssets) = 0;
            virtual void update() = 0;
            virtual void render() = 0;

    };
}

#endif /* !IRENDERER_HPP_ */
