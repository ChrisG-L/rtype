/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Components - Inclusion de tous les composants
*/

#pragma once

// Composants de base
#include "Transform.hpp"
#include "Velocity.hpp"
#include "Sprite.hpp"
#include "Animation.hpp"

// Composants de gameplay
#include "Collider.hpp"
#include "Health.hpp"
#include "PlayerInput.hpp"
#include "Missile.hpp"

// Composants réseau
#include "NetworkId.hpp"
#include "Interpolation.hpp"

namespace rtype::ecs::components {

// Namespace alias pour un accès plus court
namespace c = rtype::ecs::components;

} // namespace rtype::ecs::components
