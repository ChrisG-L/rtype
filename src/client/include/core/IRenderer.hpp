/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** IRenderer
*/

#ifndef IRENDERER_HPP_
#define IRENDERER_HPP_

#include <vector>

#include "../graphics/Asset.hpp"

namespace core {
    class IRenderer {
        public:
            virtual ~IRenderer() = default;

            virtual void initialize(GraphicAssets& assets, GraphicAssetsE& elements) = 0;
            virtual void update(float deltatime, GraphicAssets&, GraphicAssetsE& elements) = 0;
            virtual void render() = 0;

    };
}

#endif /* !IRENDERER_HPP_ */
