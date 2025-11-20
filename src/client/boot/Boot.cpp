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
    // Configuration des callbacks réseau
    tcpClient->setOnConnected([this]() {
        std::cout << "[Boot] Connecté au serveur!" << std::endl;
    });
    tcpClient->setOnDisconnected([]() {
        std::cout << "[Boot] Déconnecté du serveur" << std::endl;
    });
    tcpClient->connect("127.0.0.1", 4123); // TODO: A voir si on laisse hardcoder l'ip et le port

    // Le moteur démarre sans attendre la connexion
    engine->initialize(tcpClient.get());
    engine->run();
}
