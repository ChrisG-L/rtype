/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Boot
*/

#include "boot/Boot.hpp"

Boot::Boot():
    tcpClient(std::make_unique<client::network::TCPClient>()),
    engine(std::make_unique<core::Engine>())
{
}

void Boot::core()
{
    tcpClient->connect("127.0.0.1", 4123); // TODO: A voir si on laisse hardcoder l'ip et le port
    tcpClient->test(); // TODO: Retirer la function une fois les tests terminés
    
    engine->initialize();
    engine->run();
}
