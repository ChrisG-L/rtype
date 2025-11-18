/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Types
*/

#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <cstdint>
#include <bitset>

using Entity = std::uint32_t;

const Entity MAX_ENTITIES = 5000;

using ComponentType = std::uint8_t;

const ComponentType MAX_COMPONENTS = 32;

using Signature = std::bitset<MAX_COMPONENTS>;

#endif /* !TYPES_HPP_ */
