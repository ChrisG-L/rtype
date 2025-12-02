/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Player
*/

#ifndef PLAYER_HPP_
#define PLAYER_HPP_

#include <string>

struct Player {
    char username[32];
    char email[255];

    bool isAuthenticated = false;
};

#endif /* !PLAYER_HPP_ */
