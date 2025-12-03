/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Asset
*/

#ifndef ASSET_HPP_
#define ASSET_HPP_

#include <variant>
#include <vector>
#include <list>

namespace graphic {
    class GraphicTexture;
    class GraphicElement;
}

using Asset = std::variant<graphic::GraphicTexture>;
using AssetE = std::variant<graphic::GraphicElement>;

typedef std::list<Asset> GraphicAssets;
typedef std::list<AssetE> GraphicAssetsE;

template<class... Ts> struct overloaded : Ts... {using Ts::operator()...;};
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

#endif /* !ASSET_HPP_ */
