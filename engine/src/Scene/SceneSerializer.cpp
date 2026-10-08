#include "ArcEngine/Scene/SceneSerializer.h"
#include "ArcEngine/Core/File.h"
#include "ArcEngine/Core/Log.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Arc {

namespace {

YAML::Node Vec3Node(const Vec3& v) {
    YAML::Node n(YAML::NodeType::Sequence);
    n.SetStyle(YAML::EmitterStyle::Flow); // [x, y, z]
    n.push_back(v.x);
    n.push_back(v.y);
    n.push_back(v.z);
    return n;
}

Vec3 NodeVec3(const YAML::Node& n, const Vec3& def) {
    if (!n || !n.IsSequence() || n.size() < 3) return def;
    try {
        return Vec3(n[0].as<float>(), n[1].as<float>(), n[2].as<float>());
    } catch (...) {
        return def;
    }
}

} // namespace

bool SceneSerializer::Save(Scene& scene, const std::string& path) {
    YAML::Node root;
    root["scene"]["format"] = "arc-scene-1";
    YAML::Node entities(YAML::NodeType::Sequence);

    int count = 0;
    for (Entity e : scene.Entities()) {
        YAML::Node ent;
        ent["name"] = e.Name();
        if (e.HasComponent<TransformComponent>()) {
            const auto& t = e.GetComponent<TransformComponent>();
            YAML::Node tr;
            tr["position"] = Vec3Node(t.Position);
            tr["rotation"] = Vec3Node(t.Rotation);
            tr["scale"] = Vec3Node(t.Scale);
            ent["transform"] = tr;
        }
        entities.push_back(ent);
        count++;
    }
    root["entities"] = entities;

    std::ofstream f(path);
    if (!f) {
        Log::Error("SceneSerializer: cannot write " + path);
        return false;
    }
    f << "# ArcEngine scene (generated)\n";
    f << root;
    Log::Info("Scene saved: " + path + " (" + std::to_string(count) + " entities)");
    return true;
}

bool SceneSerializer::Load(const std::string& path, Scene& outScene) {
    if (!File::Exists(path)) {
        Log::Error("SceneSerializer: scene not found: " + path);
        return false;
    }

    YAML::Node root;
    try {
        root = YAML::LoadFile(path);
    } catch (const YAML::Exception& ex) {
        Log::Error(std::string("SceneSerializer: YAML parse error in ") + path +
                   ": " + ex.what());
        return false;
    }

    if (!root["scene"] || !root["scene"]["format"] ||
        root["scene"]["format"].as<std::string>("") != "arc-scene-1") {
        Log::Warn("SceneSerializer: missing/unknown format header in " + path +
                  " — attempting load anyway");
    }

    outScene.Clear();
    int count = 0;
    if (root["entities"] && root["entities"].IsSequence()) {
        for (const auto& ent : root["entities"]) {
            const std::string name =
                ent["name"] ? ent["name"].as<std::string>("Entity") : "Entity";
            Entity e = outScene.CreateEntity(name);
            if (ent["transform"]) {
                const auto& tr = ent["transform"];
                auto& t = e.GetComponent<TransformComponent>();
                t.Position = NodeVec3(tr["position"], t.Position);
                t.Rotation = NodeVec3(tr["rotation"], t.Rotation);
                t.Scale = NodeVec3(tr["scale"], t.Scale);
            }
            count++;
        }
    }
    Log::Info("Scene loaded: " + path + " (" + std::to_string(count) + " entities)");
    return true;
}

} // namespace Arc
