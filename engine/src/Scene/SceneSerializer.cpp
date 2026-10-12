#include "ArcEngine/Scene/SceneSerializer.h"
#include "ArcEngine/Core/File.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Scene/MeshComponent.h"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <filesystem>

namespace Arc {

namespace {

const char* kFormat = "arc-scene-2";

YAML::Node Vec3Node(const Vec3& v) {
    YAML::Node n(YAML::NodeType::Sequence);
    n.SetStyle(YAML::EmitterStyle::Flow); // [x, y, z]
    n.push_back(v.x);
    n.push_back(v.y);
    n.push_back(v.z);
    return n;
}

YAML::Node Vec2Node(const Vec2& v) {
    YAML::Node n(YAML::NodeType::Sequence);
    n.SetStyle(YAML::EmitterStyle::Flow);
    n.push_back(v.x);
    n.push_back(v.y);
    return n;
}

YAML::Node Vec4Node(const Vec4& v) {
    YAML::Node n(YAML::NodeType::Sequence);
    n.SetStyle(YAML::EmitterStyle::Flow);
    n.push_back(v.x);
    n.push_back(v.y);
    n.push_back(v.z);
    n.push_back(v.w);
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

Vec2 NodeVec2(const YAML::Node& n, const Vec2& def) {
    if (!n || !n.IsSequence() || n.size() < 2) return def;
    try {
        return Vec2(n[0].as<float>(), n[1].as<float>());
    } catch (...) {
        return def;
    }
}

Vec4 NodeVec4(const YAML::Node& n, const Vec4& def) {
    if (!n || !n.IsSequence() || n.size() < 4) return def;
    try {
        return Vec4(n[0].as<float>(), n[1].as<float>(), n[2].as<float>(), n[3].as<float>());
    } catch (...) {
        return def;
    }
}

Vec3 AverageTint(const MeshComponent& m, const Vec3& def) {
    size_t n = 0;
    Vec3 acc(0, 0, 0);
    for (size_t i = 0; i + 5 < m.Vertices.size(); i += 6) {
        acc.x += m.Vertices[i + 3];
        acc.y += m.Vertices[i + 4];
        acc.z += m.Vertices[i + 5];
        ++n;
    }
    if (n == 0) return def;
    return Vec3(acc.x / n, acc.y / n, acc.z / n);
}

std::string GuiKindToStr(GuiComponent::Kind k) {
    if (k == GuiComponent::Kind::Label) return "label";
    if (k == GuiComponent::Kind::Panel) return "panel";
    return "button";
}

GuiComponent::Kind GuiKindFromStr(const std::string& s) {
    if (s == "label") return GuiComponent::Kind::Label;
    if (s == "panel") return GuiComponent::Kind::Panel;
    return GuiComponent::Kind::Button;
}

std::string RigidBodyTypeToStr(RigidBodyType t) {
    if (t == RigidBodyType::Static) return "static";
    if (t == RigidBodyType::Kinematic) return "kinematic";
    return "dynamic";
}

RigidBodyType RigidBodyTypeFromStr(const std::string& s) {
    if (s == "static") return RigidBodyType::Static;
    if (s == "kinematic") return RigidBodyType::Kinematic;
    return RigidBodyType::Dynamic;
}

} // namespace

bool SceneSerializer::Save(Scene& scene, const std::string& path) {
    YAML::Node root;
    root["scene"]["format"] = kFormat;
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
        if (e.HasComponent<MeshComponent>()) {
            const auto& m = e.GetComponent<MeshComponent>();
            YAML::Node mn;
            mn["id"] = m.MeshId.empty() ? "triangle" : m.MeshId;
            mn["tint"] = Vec3Node(AverageTint(m, Vec3(0.9f, 0.9f, 0.9f)));
            ent["mesh"] = mn;
        }
        if (e.HasComponent<CameraComponent>()) {
            const auto& c = e.GetComponent<CameraComponent>();
            YAML::Node cn;
            cn["active"] = c.Active;
            cn["zoom"] = c.Zoom;
            ent["camera"] = cn;
        }
        if (e.HasComponent<SpriteComponent>()) {
            const auto& sp = e.GetComponent<SpriteComponent>();
            YAML::Node sn;
            sn["path"] = sp.TexturePath;
            sn["tint"] = Vec4Node(sp.Tint);
            sn["size"] = Vec2Node(sp.Size);
            sn["layer"] = sp.Layer;
            sn["flip_x"] = sp.FlipX;
            sn["flip_y"] = sp.FlipY;
            sn["filter_linear"] = sp.FilterLinear;
            YAML::Node rn;
            rn["min"] = Vec2Node(sp.RegionMin);
            rn["max"] = Vec2Node(sp.RegionMax);
            sn["region"] = rn;
            ent["sprite"] = sn;
        }
        if (e.HasComponent<LightComponent>()) {
            const auto& l = e.GetComponent<LightComponent>();
            YAML::Node ln;
            ln["color"] = Vec3Node(l.Color);
            ln["intensity"] = l.Intensity;
            ln["radius"] = l.Radius;
            ent["light"] = ln;
        }
        if (e.HasComponent<GuiComponent>()) {
            const auto& g = e.GetComponent<GuiComponent>();
            YAML::Node gn;
            gn["widget"] = GuiKindToStr(g.Widget);
            gn["size"] = Vec2Node(g.Size);
            gn["text"] = std::string(g.Text);
            YAML::Node bg;
            bg["bg"] = Vec3Node(g.BgColor);
            bg["text"] = Vec3Node(g.TextColor);
            gn["colors"] = bg;
            ent["gui"] = gn;
        }
        if (e.HasComponent<RigidBody2DComponent>()) {
            const auto& rb = e.GetComponent<RigidBody2DComponent>();
            YAML::Node rbn;
            rbn["type"] = RigidBodyTypeToStr(rb.Type);
            rbn["fixed_rotation"] = rb.FixedRotation;
            rbn["gravity_scale"] = rb.GravityScale;
            rbn["linear_damping"] = rb.LinearDamping;
            rbn["angular_damping"] = rb.AngularDamping;
            rbn["category"] = rb.Category;
            rbn["mask"] = rb.Mask;
            rbn["group_index"] = rb.GroupIndex;
            ent["rigidbody2d"] = rbn;
        }
        if (e.HasComponent<BoxCollider2DComponent>()) {
            const auto& col = e.GetComponent<BoxCollider2DComponent>();
            YAML::Node ccn;
            ccn["size"] = Vec2Node(col.Size);
            ccn["density"] = col.Density;
            ccn["friction"] = col.Friction;
            ccn["restitution"] = col.Restitution;
            ccn["is_sensor"] = col.IsSensor;
            ent["boxcollider2d"] = ccn;
        }
        entities.push_back(ent);
        count++;
    }
    root["entities"] = entities;

    // Ensure the parent directory exists (e.g. assets/scenes/ on a fresh
    // checkout) — ofstream won't create it, so a missing dir silently fails.
    std::error_code ec;
    std::filesystem::path parent = std::filesystem::path(path).parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent, ec);

    std::ofstream f(path);
    if (!f) {
        Log::Error("SceneSerializer: cannot write " + path);
        return false;
    }
    f << "# ArcEngine scene (generated, " << kFormat << ")\n";
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

    std::string fmt = (root["scene"] && root["scene"]["format"])
                          ? root["scene"]["format"].as<std::string>("")
                          : "";
    if (fmt != kFormat && fmt != "arc-scene-1") {
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
            if (ent["mesh"]) {
                const auto& mn = ent["mesh"];
                std::string id =
                    mn["id"] ? mn["id"].as<std::string>("triangle") : "triangle";
                Vec3 tint = NodeVec3(mn["tint"], Vec3(0.9f, 0.9f, 0.9f));
                float rgb[3] = {tint.x, tint.y, tint.z};
                MeshComponent m = MeshComponent::FromId(id, rgb);
                m.MeshId = id;
                MeshComponent& d = e.AddComponent<MeshComponent>();
                d = std::move(m);
                d.Upload();
            }
            if (ent["camera"]) {
                const auto& cn = ent["camera"];
                CameraComponent c;
                c.Active = cn["active"] ? cn["active"].as<bool>(true) : true;
                c.Zoom = cn["zoom"] ? cn["zoom"].as<float>(1.0f) : 1.0f;
                e.AddComponent<CameraComponent>() = c;
            }
            if (ent["sprite"]) {
                const auto& sn = ent["sprite"];
                SpriteComponent sp;
                sp.TexturePath = sn["path"] ? sn["path"].as<std::string>("") : "";
                sp.Tint = NodeVec4(sn["tint"], sp.Tint);
                sp.Size = NodeVec2(sn["size"], sp.Size);
                sp.Layer = sn["layer"] ? sn["layer"].as<int>(0) : 0;
                sp.FlipX = sn["flip_x"] ? sn["flip_x"].as<bool>(false) : false;
                sp.FlipY = sn["flip_y"] ? sn["flip_y"].as<bool>(false) : false;
                sp.FilterLinear =
                    sn["filter_linear"] ? sn["filter_linear"].as<bool>(true) : true;
                if (sn["region"]) {
                    sp.RegionMin = NodeVec2(sn["region"]["min"], sp.RegionMin);
                    sp.RegionMax = NodeVec2(sn["region"]["max"], sp.RegionMax);
                }
                e.AddComponent<SpriteComponent>() = sp;
            }
            if (ent["light"]) {
                const auto& ln = ent["light"];
                LightComponent l;
                l.Color = NodeVec3(ln["color"], l.Color);
                l.Intensity = ln["intensity"] ? ln["intensity"].as<float>(1.0f) : 1.0f;
                l.Radius = ln["radius"] ? ln["radius"].as<float>(3.0f) : 3.0f;
                e.AddComponent<LightComponent>() = l;
            }
            if (ent["gui"]) {
                const auto& gn = ent["gui"];
                GuiComponent g;
                g.Widget = GuiKindFromStr(gn["widget"]
                                              ? gn["widget"].as<std::string>("button")
                                              : "button");
                g.Size = NodeVec2(gn["size"], g.Size);
                std::string text =
                    gn["text"] ? gn["text"].as<std::string>("Button") : "Button";
                std::snprintf(g.Text, sizeof(g.Text), "%s", text.c_str());
                if (gn["colors"]) {
                    g.BgColor = NodeVec3(gn["colors"]["bg"], g.BgColor);
                    g.TextColor = NodeVec3(gn["colors"]["text"], g.TextColor);
                }
                e.AddComponent<GuiComponent>() = g;
            }
            if (ent["rigidbody2d"]) {
                const auto& rbn = ent["rigidbody2d"];
                RigidBody2DComponent rb;
                rb.Type = RigidBodyTypeFromStr(
                    rbn["type"] ? rbn["type"].as<std::string>("dynamic") : "dynamic");
                rb.FixedRotation =
                    rbn["fixed_rotation"] ? rbn["fixed_rotation"].as<bool>(false) : false;
                rb.GravityScale =
                    rbn["gravity_scale"] ? rbn["gravity_scale"].as<float>(1.0f) : 1.0f;
                rb.LinearDamping =
                    rbn["linear_damping"] ? rbn["linear_damping"].as<float>(0.0f) : 0.0f;
                rb.AngularDamping =
                    rbn["angular_damping"] ? rbn["angular_damping"].as<float>(0.01f) : 0.01f;
                rb.Category =
                    rbn["category"] ? rbn["category"].as<uint32_t>(0x00000001u) : 0x00000001u;
                rb.Mask = rbn["mask"] ? rbn["mask"].as<uint32_t>(0xFFFFFFFFu) : 0xFFFFFFFFu;
                rb.GroupIndex =
                    rbn["group_index"] ? rbn["group_index"].as<int>(0) : 0;
                e.AddComponent<RigidBody2DComponent>() = rb;
            }
            if (ent["boxcollider2d"]) {
                const auto& ccn = ent["boxcollider2d"];
                BoxCollider2DComponent col;
                col.Size = NodeVec2(ccn["size"], col.Size);
                col.Density = ccn["density"] ? ccn["density"].as<float>(1.0f) : 1.0f;
                col.Friction = ccn["friction"] ? ccn["friction"].as<float>(0.3f) : 0.3f;
                col.Restitution = ccn["restitution"] ? ccn["restitution"].as<float>(0.0f) : 0.0f;
                col.IsSensor = ccn["is_sensor"] ? ccn["is_sensor"].as<bool>(false) : false;
                e.AddComponent<BoxCollider2DComponent>() = col;
            }
            count++;
        }
    }
    Log::Info("Scene loaded: " + path + " (" + std::to_string(count) + " entities)");
    return true;
}

} // namespace Arc
