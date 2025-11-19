/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** ITexture
*/

#ifndef ITEXTURE_HPP_
#define ITEXTURE_HPP_

#include "../utils/Vecs.hpp"

namespace graphics {
    class ITexture {
        public:
            virtual ~ITexture() = default;

            virtual Vec2u getSize() const = 0;
            
            virtual void loadFile() = 0;
    };
}

#endif /* !ITEXTURE_HPP_ */
