/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Asset
*/

#ifndef ASSET_HPP_
#define ASSET_HPP_

#include <variant>

namespace graphic {
    class GraphicTexture;
}

using Asset = std::variant<graphic::GraphicTexture>;

template<class... Ts> struct overloaded : Ts... {using Ts::operator()...;};
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

#endif /* !ASSET_HPP_ */
