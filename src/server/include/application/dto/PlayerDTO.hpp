/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerDTO
*/

#ifndef PLAYERDTO_HPP_
#define PLAYERDTO_HPP_

#include <string>

struct PlayerDTO
{
    std::string id;
    float health;
    float damage;
    float x, y, z;
};


#endif /* !PLAYERDTO_HPP_ */
