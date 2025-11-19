/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** SFMLTexture
*/

#include "implementations/sfml/SFMLTexture.hpp"

SFMLTexture::SFMLTexture(): _size{.x = 0, .y = 0}
{
}

Vec2u SFMLTexture::getSize() const
{
    return _size;
}

void SFMLTexture::loadFile()
{

}
