#pragma once
#include "ArcEngine/Scene/Entity.h"

#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Arc {

// Minimal scene registry (M5) — owns entities + per-type component pools.
// No entt dependency: simple unordered_map<Type, unordered_map<EntityId, Component>>.
// Enough for hundreds of entities; M7 editor iterates the same pools.
class Scene {
public:
    Scene() = default;
    ~Scene() = default;

    Entity CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(Entity e);
    void Clear();

    size_t EntityCount() const { return m_names.size(); }
    std::vector<Entity> Entities();

    const std::string& EntityName(uint32_t id) const;

    void Update(float dt); // calls UpdateFn on each entity (M9 Lua hooks here)
    using UpdateFn = std::function<void(Entity, float)>;
    void SetUpdateFn(UpdateFn fn) { m_updateFn = std::move(fn); }

    template <typename T, typename... Args>
    T& AddComponent(uint32_t id, Args&&... args) {
        auto& pool = ComponentPool<T>();
        auto it = pool.find(id);
        if (it != pool.end()) return it->second;
        return pool.emplace(id, T(std::forward<Args>(args)...)).first->second;
    }

    template <typename T>
    bool HasComponent(uint32_t id) const {
        const auto& pool = ComponentPool<T>();
        return pool.find(id) != pool.end();
    }

    template <typename T>
    T& GetComponent(uint32_t id) {
        return ComponentPool<T>().at(id);
    }

    template <typename T>
    const T& GetComponent(uint32_t id) const {
        return ComponentPool<T>().at(id);
    }

    // Iterate all entities having T (used by renderer + editor).
    template <typename T, typename Fn>
    void Each(Fn&& fn) {
        auto& pool = ComponentPool<T>();
        for (auto& [id, comp] : pool) {
            if (m_alive[id]) fn(Entity(this, id), comp);
        }
    }

private:
    template <typename T>
    static std::unordered_map<uint32_t, T>& ComponentPool() {
        static std::unordered_map<uint32_t, T> s_pool;
        return s_pool;
    }
    // NOTE: static pools are per-type process-wide (simple for M5).
    // M6+ moves pools into Scene instance for multi-scene support.

    friend class Entity;

    std::unordered_map<uint32_t, std::string> m_names;
    std::unordered_map<uint32_t, bool> m_alive;
    uint32_t m_nextId = 1;
    UpdateFn m_updateFn;
};

inline Entity::Entity(Scene* scene, uint32_t id) : m_scene(scene), m_id(id) {}

inline bool Entity::Valid() const {
    return m_scene != nullptr && m_id != 0;
}

template <typename T, typename... Args>
T& Entity::AddComponent(Args&&... args) {
    return m_scene->AddComponent<T>(m_id, std::forward<Args>(args)...);
}

inline const std::string& Entity::Name() const {
    static const std::string kEmpty;
    if (!Valid()) return kEmpty;
    return m_scene->EntityName(m_id);
}

template <typename T>
bool Entity::HasComponent() const {
    return Valid() && m_scene->HasComponent<T>(m_id);
}

template <typename T>
T& Entity::GetComponent() {
    return m_scene->GetComponent<T>(m_id);
}

template <typename T>
const T& Entity::GetComponent() const {
    return m_scene->GetComponent<T>(m_id);
}

} // namespace Arc
