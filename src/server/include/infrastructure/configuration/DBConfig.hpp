/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** DBConfig
*/

#ifndef DBCONFIG_HPP_
#define DBCONFIG_HPP_

#include <string>

struct DBConfig {
    std::string connexionString;
    std::string dbName;
    int minPoolSize = 0;
    int maxPoolSize = 0;
};

#endif /* !DBCONFIG_HPP_ */
