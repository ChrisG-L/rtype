/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** SparseSet - Stockage efficace des composants
*/

#pragma once

#include <vector>
#include <optional>
#include <cassert>
#include "Entity.hpp"

namespace rtype::ecs {

/**
 * @brief Sparse set pour un stockage et une itération efficaces des composants
 *
 * Fournit un accès, insertion et suppression en O(1) tout en maintenant
 * une itération dense compatible avec le cache.
 */
template<typename Component>
class SparseSet {
public:
    SparseSet() {
        m_sparse.resize(MAX_ENTITIES, NULL_ENTITY);
    }

    /**
     * @brief Insère un composant pour une entité
     */
    template<typename... Args>
    Component& insert(Entity entity, Args&&... args) {
        assert(entity < MAX_ENTITIES && "ID d'entité hors limites");

        if (contains(entity)) {
            return get(entity) = Component(std::forward<Args>(args)...);
        }

        std::size_t index = m_dense.size();
        m_dense.push_back(entity);
        m_components.emplace_back(std::forward<Args>(args)...);
        m_sparse[entity] = static_cast<Entity>(index);

        return m_components.back();
    }

    /**
     * @brief Supprime un composant d'une entité
     */
    void remove(Entity entity) {
        assert(contains(entity) && "L'entité n'a pas ce composant");

        std::size_t index = m_sparse[entity];
        std::size_t last = m_dense.size() - 1;

        // Échange avec le dernier élément
        Entity lastEntity = m_dense[last];
        m_dense[index] = lastEntity;
        m_components[index] = std::move(m_components[last]);
        m_sparse[lastEntity] = static_cast<Entity>(index);

        // Supprime le dernier élément
        m_dense.pop_back();
        m_components.pop_back();
        m_sparse[entity] = NULL_ENTITY;
    }

    /**
     * @brief Vérifie si l'entité possède ce composant
     */
    [[nodiscard]] bool contains(Entity entity) const {
        return entity < MAX_ENTITIES &&
               m_sparse[entity] != NULL_ENTITY;
    }

    /**
     * @brief Récupère le composant d'une entité
     */
    [[nodiscard]] Component& get(Entity entity) {
        assert(contains(entity) && "L'entité n'a pas ce composant");
        return m_components[m_sparse[entity]];
    }

    [[nodiscard]] const Component& get(Entity entity) const {
        assert(contains(entity) && "L'entité n'a pas ce composant");
        return m_components[m_sparse[entity]];
    }

    /**
     * @brief Tente de récupérer un composant (retourne nullptr si non trouvé)
     */
    [[nodiscard]] Component* tryGet(Entity entity) {
        if (!contains(entity)) return nullptr;
        return &m_components[m_sparse[entity]];
    }

    [[nodiscard]] const Component* tryGet(Entity entity) const {
        if (!contains(entity)) return nullptr;
        return &m_components[m_sparse[entity]];
    }

    /**
     * @brief Retourne le nombre de composants
     */
    [[nodiscard]] std::size_t size() const {
        return m_dense.size();
    }

    /**
     * @brief Vérifie si vide
     */
    [[nodiscard]] bool empty() const {
        return m_dense.empty();
    }

    /**
     * @brief Supprime tous les composants
     */
    void clear() {
        for (Entity entity : m_dense) {
            m_sparse[entity] = NULL_ENTITY;
        }
        m_dense.clear();
        m_components.clear();
    }

    /**
     * @brief Retourne le tableau dense des entités (pour itération)
     */
    [[nodiscard]] const std::vector<Entity>& entities() const {
        return m_dense;
    }

    /**
     * @brief Retourne le tableau dense des composants (pour itération)
     */
    [[nodiscard]] std::vector<Component>& components() {
        return m_components;
    }

    [[nodiscard]] const std::vector<Component>& components() const {
        return m_components;
    }

    // Support des itérateurs pour les boucles range-based
    auto begin() { return m_components.begin(); }
    auto end() { return m_components.end(); }
    auto begin() const { return m_components.begin(); }
    auto end() const { return m_components.end(); }

private:
    std::vector<Entity> m_sparse;        // Mapping Entity -> Index
    std::vector<Entity> m_dense;         // Mapping Index -> Entity
    std::vector<Component> m_components; // Données des composants
};

} // namespace rtype::ecs
