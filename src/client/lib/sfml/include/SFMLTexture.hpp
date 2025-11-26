/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** SFMLTexture
*/

#ifndef SFMLTEXTURE_HPP_
#define SFMLTEXTURE_HPP_

#include "graphics/ITexture.hpp"

class SFMLTexture: public graphics::ITexture {
    public:
        SFMLTexture();
        Vec2u getSize() const override;
        void loadFile() override;

    private:
        Vec2u _size;
};

#endif /* !SFMLTEXTURE_HPP_ */
