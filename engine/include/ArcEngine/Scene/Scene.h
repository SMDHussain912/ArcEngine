#pragma once
#include "ArcEngine/Scene/Entity.h"

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Arc {

// Scene registry — owns entities + per-type component pools (Unity-style).
// Pools are INSTANCE-OWNED (std::any-typed maps keyed by typeid): two Scene
// objects never share component storage — required for load-while-old-scene
// round-trips. (M5 had process-wide static pools; fixed in P0 Step 4.)
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
    void RenameEntity(uint32_t id, const std::string& name);

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
    void RemoveComponent(uint32_t id) {
        auto it = m_pools.find(typeid(T));
        if (it == m_pools.end()) return;
        auto& pool = std::any_cast<std::unordered_map<uint32_t, T>&>(it->second);
        pool.erase(id);
    }

    template <typename T>
    const T& GetComponent(uint32_t id) const {
        return ComponentPool<T>().at(id);
    }

    // Selection + iteration order for the editor (M7).
    Entity GetFocusEntity() const { return m_focus; }
    void SetFocusEntity(Entity e) { m_focus = e; }

    // Iterate all entities having T (used by renderer + editor).
    template <typename T, typename Fn>
    void Each(Fn&& fn) {
        auto& pool = ComponentPool<T>();
        for (auto& [id, comp] : pool) {
            if (m_alive[id]) fn(Entity(this, id), comp);
        }
    }

private:
    // Per-instance, per-type pool: map<entityId, Component> erased behind std::any.
    template <typename T>
    std::unordered_map<uint32_t, T>& ComponentPool() {
        auto it = m_pools.find(typeid(T));
        if (it == m_pools.end())
            it = m_pools.emplace(typeid(T), std::unordered_map<uint32_t, T>{}).first;
        return std::any_cast<std::unordered_map<uint32_t, T>&>(it->second);
    }

    template <typename T>
    const std::unordered_map<uint32_t, T>& ComponentPool() const {
        static const std::unordered_map<uint32_t, T> s_empty;
        auto it = m_pools.find(typeid(T));
        if (it == m_pools.end()) return s_empty;
        return std::any_cast<const std::unordered_map<uint32_t, T>&>(it->second);
    }

    friend class Entity;
    friend class SceneSerializer;

    std::unordered_map<uint32_t, std::string> m_names;
    std::unordered_map<uint32_t, bool> m_alive;
    std::unordered_map<std::type_index, std::any> m_pools;
    uint32_t m_nextId = 1;
    UpdateFn m_updateFn;
    Entity m_focus; // editor selection (M7)
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

template <typename T>
bool Entity::RemoveComponent() {
    if (!Valid() || m_scene == nullptr) return false;
    if (!m_scene->HasComponent<T>(m_id)) return false;
    m_scene->RemoveComponent<T>(m_id);
    return true;
}

} // namespace Arc
