/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** main
*/

#include "main.hpp"



int main()
{
    EntityManager em;
    ComponentManager cm;

    cm.registerComponent<Position>();

    Entity e1 = em.createEntity();
    Entity e2 = em.createEntity();
    std::cout << "e1: " << e1 << std::endl;
    std::cout << "e2: " << e2 << std::endl;
    Position position(1.0, 2.0, 3.0);
    cm.addComponent<Position>(e1, position);
    cm.addComponent<Position>(e2, position);

    std::vector<Entity> moveEntities = cm.getEntitiesWithComponents<Position>();
    for (auto& ent: moveEntities) {
        Position *pos = cm.getComponent<Position>(ent);
        
        std::cout << "ent: " << ent << std::endl;
        std::cout << "pos: " << pos->_z << std::endl;
        pos->_z += 1.0;
    }
}