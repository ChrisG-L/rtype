/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Registry - Gestionnaire principal de l'ECS
*/

#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>
#include <typeindex>
#include <algorithm>
#include <functional>
#include "Entity.hpp"
#include "SparseSet.hpp"
#include "System.hpp"

namespace rtype::ecs {

/**
 * @brief Base type-erased pour les pools de composants
 */
class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void remove(Entity entity) = 0;
    virtual bool contains(Entity entity) const = 0;
    virtual void clear() = 0;
};

/**
 * @brief Pool de composants typé
 */
template<typename Component>
class ComponentPool : public IComponentPool {
public:
    SparseSet<Component> data;

    void remove(Entity entity) override {
        if (data.contains(entity)) {
            data.remove(entity);
        }
    }

    bool contains(Entity entity) const override {
        return data.contains(entity);
    }

    void clear() override {
        data.clear();
    }
};

/**
 * @brief Registre ECS principal - gère les entités, composants et systèmes
 *
 * Le Registry est le hub central de l'architecture ECS.
 * Il gère le cycle de vie des entités, le stockage des composants
 * et l'exécution des systèmes.
 */
class Registry {
public:
    Registry() = default;
    ~Registry() = default;

    // Non-copiable, déplaçable
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = default;
    Registry& operator=(Registry&&) = default;

    // ==================== Gestion des Entités ====================

    /**
     * @brief Crée une nouvelle entité
     */
    [[nodiscard]] Entity createEntity() {
        Entity entity;

        if (!m_freeEntities.empty()) {
            entity = m_freeEntities.front();
            m_freeEntities.pop();
        } else {
            entity = m_nextEntity++;
        }

        m_activeEntities.push_back(entity);
        return entity;
    }

    /**
     * @brief Détruit une entité et tous ses composants
     */
    void destroyEntity(Entity entity) {
        // Supprime tous les composants
        for (auto& [type, pool] : m_componentPools) {
            pool->remove(entity);
        }

        // Supprime des entités actives
        auto it = std::find(m_activeEntities.begin(), m_activeEntities.end(), entity);
        if (it != m_activeEntities.end()) {
            m_activeEntities.erase(it);
        }

        // Ajoute à la liste libre pour réutilisation
        m_freeEntities.push(entity);
    }

    /**
     * @brief Vérifie si l'entité est valide/active
     */
    [[nodiscard]] bool isEntityValid(Entity entity) const {
        return std::find(m_activeEntities.begin(), m_activeEntities.end(), entity)
               != m_activeEntities.end();
    }

    /**
     * @brief Retourne toutes les entités actives
     */
    [[nodiscard]] const std::vector<Entity>& getEntities() const {
        return m_activeEntities;
    }

    /**
     * @brief Retourne le nombre d'entités
     */
    [[nodiscard]] std::size_t getEntityCount() const {
        return m_activeEntities.size();
    }

    // ==================== Gestion des Composants ====================

    /**
     * @brief Ajoute un composant à une entité
     */
    template<typename Component, typename... Args>
    Component& addComponent(Entity entity, Args&&... args) {
        auto& pool = getOrCreatePool<Component>();
        return pool.data.insert(entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Supprime un composant d'une entité
     */
    template<typename Component>
    void removeComponent(Entity entity) {
        auto* pool = getPool<Component>();
        if (pool) {
            pool->data.remove(entity);
        }
    }

    /**
     * @brief Récupère un composant d'une entité
     */
    template<typename Component>
    [[nodiscard]] Component& getComponent(Entity entity) {
        auto* pool = getPool<Component>();
        assert(pool && "Type de composant non enregistré");
        return pool->data.get(entity);
    }

    template<typename Component>
    [[nodiscard]] const Component& getComponent(Entity entity) const {
        auto* pool = getPool<Component>();
        assert(pool && "Type de composant non enregistré");
        return pool->data.get(entity);
    }

    /**
     * @brief Tente de récupérer un composant (retourne nullptr si non trouvé)
     */
    template<typename Component>
    [[nodiscard]] Component* tryGetComponent(Entity entity) {
        auto* pool = getPool<Component>();
        if (!pool) return nullptr;
        return pool->data.tryGet(entity);
    }

    template<typename Component>
    [[nodiscard]] const Component* tryGetComponent(Entity entity) const {
        auto* pool = getPool<Component>();
        if (!pool) return nullptr;
        return pool->data.tryGet(entity);
    }

    /**
     * @brief Vérifie si l'entité possède un composant
     */
    template<typename Component>
    [[nodiscard]] bool hasComponent(Entity entity) const {
        auto* pool = getPool<Component>();
        return pool && pool->data.contains(entity);
    }

    /**
     * @brief Vérifie si l'entité possède tous les composants spécifiés
     */
    template<typename... Components>
    [[nodiscard]] bool hasComponents(Entity entity) const {
        return (hasComponent<Components>(entity) && ...);
    }

    /**
     * @brief Retourne toutes les entités avec un composant spécifique
     */
    template<typename Component>
    [[nodiscard]] const std::vector<Entity>& getEntitiesWith() const {
        static std::vector<Entity> empty;
        auto* pool = getPool<Component>();
        if (!pool) return empty;
        return pool->data.entities();
    }

    /**
     * @brief Récupère le pool de composants pour itération
     */
    template<typename Component>
    [[nodiscard]] SparseSet<Component>* getComponentPool() {
        auto* pool = getPool<Component>();
        if (!pool) return nullptr;
        return &pool->data;
    }

    // ==================== View (itérer sur les entités avec composants) ====================

    /**
     * @brief Itère sur toutes les entités avec des composants spécifiques
     */
    template<typename... Components, typename Func>
    void view(Func&& func) {
        // Récupère le plus petit pool pour une meilleure itération
        auto* smallestPool = findSmallestPool<Components...>();
        if (!smallestPool) return;

        for (Entity entity : *smallestPool) {
            if (hasComponents<Components...>(entity)) {
                func(entity, getComponent<Components>(entity)...);
            }
        }
    }

    /**
     * @brief Itère avec accès const
     */
    template<typename... Components, typename Func>
    void view(Func&& func) const {
        auto* smallestPool = findSmallestPool<Components...>();
        if (!smallestPool) return;

        for (Entity entity : *smallestPool) {
            if (hasComponents<Components...>(entity)) {
                func(entity, getComponent<Components>(entity)...);
            }
        }
    }

    // ==================== Gestion des Systèmes ====================

    /**
     * @brief Ajoute un système
     */
    template<typename System, typename... Args>
    System& addSystem(Args&&... args) {
        auto system = std::make_unique<System>(std::forward<Args>(args)...);
        System& ref = *system;
        m_systems.push_back(std::move(system));

        // Trie par priorité
        std::sort(m_systems.begin(), m_systems.end(),
            [](const auto& a, const auto& b) {
                return a->priority() < b->priority();
            });

        return ref;
    }

    /**
     * @brief Récupère un système par type
     */
    template<typename System>
    [[nodiscard]] System* getSystem() {
        for (auto& system : m_systems) {
            if (auto* s = dynamic_cast<System*>(system.get())) {
                return s;
            }
        }
        return nullptr;
    }

    /**
     * @brief Met à jour tous les systèmes
     */
    void update(float dt) {
        for (auto& system : m_systems) {
            if (system->isEnabled()) {
                system->update(*this, dt);
            }
        }
    }

    // ==================== Utilitaires ====================

    /**
     * @brief Supprime toutes les entités et composants
     */
    void clear() {
        m_activeEntities.clear();
        while (!m_freeEntities.empty()) {
            m_freeEntities.pop();
        }
        m_nextEntity = 0;

        for (auto& [type, pool] : m_componentPools) {
            pool->clear();
        }
    }

    /**
     * @brief Planifie la destruction d'une entité (sûr pendant l'itération)
     */
    void scheduleDestroy(Entity entity) {
        m_toDestroy.push_back(entity);
    }

    /**
     * @brief Traite les destructions planifiées
     */
    void processDestructions() {
        for (Entity entity : m_toDestroy) {
            destroyEntity(entity);
        }
        m_toDestroy.clear();
    }

private:
    template<typename Component>
    ComponentPool<Component>& getOrCreatePool() {
        std::type_index type = typeid(Component);
        auto it = m_componentPools.find(type);

        if (it == m_componentPools.end()) {
            auto pool = std::make_unique<ComponentPool<Component>>();
            auto& ref = *pool;
            m_componentPools[type] = std::move(pool);
            return ref;
        }

        return static_cast<ComponentPool<Component>&>(*it->second);
    }

    template<typename Component>
    ComponentPool<Component>* getPool() {
        std::type_index type = typeid(Component);
        auto it = m_componentPools.find(type);
        if (it == m_componentPools.end()) return nullptr;
        return static_cast<ComponentPool<Component>*>(it->second.get());
    }

    template<typename Component>
    const ComponentPool<Component>* getPool() const {
        std::type_index type = typeid(Component);
        auto it = m_componentPools.find(type);
        if (it == m_componentPools.end()) return nullptr;
        return static_cast<const ComponentPool<Component>*>(it->second.get());
    }

    template<typename First, typename... Rest>
    const std::vector<Entity>* findSmallestPool() const {
        const std::vector<Entity>* smallest = nullptr;
        std::size_t minSize = std::numeric_limits<std::size_t>::max();

        auto checkPool = [&]<typename T>() {
            if (auto* pool = getPool<T>()) {
                if (pool->data.size() < minSize) {
                    minSize = pool->data.size();
                    smallest = &pool->data.entities();
                }
            }
        };

        checkPool.template operator()<First>();
        (checkPool.template operator()<Rest>(), ...);

        return smallest;
    }

private:
    // Gestion des entités
    Entity m_nextEntity = 0;
    std::vector<Entity> m_activeEntities;
    std::queue<Entity> m_freeEntities;
    std::vector<Entity> m_toDestroy;

    // Stockage des composants
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_componentPools;

    // Systèmes
    std::vector<std::unique_ptr<ISystem>> m_systems;
};

} // namespace rtype::ecs
