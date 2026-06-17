#include "SceneLoader.h"
#include "Trajectory.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

// Helper para ler vec3 de array JSON [x,y,z]
static glm::vec3 readVec3(const json& arr) {
    return glm::vec3(arr[0].get<float>(),
                     arr[1].get<float>(),
                     arr[2].get<float>());
}

// === PARSER DO SCENE.JSON ===
SceneConfig SceneLoader::load(const std::string& jsonPath) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        throw std::runtime_error("SceneLoader: nao foi possivel abrir: " + jsonPath);
    }

    json j;
    file >> j;

    SceneConfig cfg;

    // --- Janela ---
    if (j.contains("window")) {
        auto& w = j["window"];
        if (w.contains("width"))  cfg.windowWidth  = w["width"].get<int>();
        if (w.contains("height")) cfg.windowHeight = w["height"].get<int>();
        if (w.contains("title"))  cfg.windowTitle  = w["title"].get<std::string>();
    }

    // --- Câmera ---
    if (j.contains("camera")) {
        auto& c = j["camera"];
        if (c.contains("position")) cfg.camPosition = readVec3(c["position"]);
        if (c.contains("yaw"))      cfg.camYaw       = c["yaw"].get<float>();
        if (c.contains("pitch"))    cfg.camPitch      = c["pitch"].get<float>();
        if (c.contains("fov"))      cfg.fov           = c["fov"].get<float>();
        if (c.contains("near"))     cfg.nearP         = c["near"].get<float>();
        if (c.contains("far"))      cfg.farP          = c["far"].get<float>();
    }

    // --- Texturas do ambiente ---
    if (j.contains("floor_texture")) cfg.floorTexture = j["floor_texture"].get<std::string>();
    if (j.contains("wall_texture"))  cfg.wallTexture  = j["wall_texture"].get<std::string>();

    // --- Luzes (array de 3: Key, Fill, Back) ---
    if (j.contains("lights") && j["lights"].is_array()) {
        auto& lightsArr = j["lights"];
        for (size_t i = 0; i < 3 && i < lightsArr.size(); ++i) {
            auto& lj = lightsArr[i];
            Light& light = cfg.lights[i];
            if (lj.contains("name"))      light.name      = lj["name"].get<std::string>();
            if (lj.contains("position"))  light.position  = readVec3(lj["position"]);
            if (lj.contains("color"))     light.color     = readVec3(lj["color"]);
            if (lj.contains("intensity")) light.intensity = lj["intensity"].get<float>();
            if (lj.contains("enabled"))   light.enabled   = lj["enabled"].get<bool>();
        }
    }

    // --- Objetos da cena ---
    if (j.contains("objects") && j["objects"].is_array()) {
        for (auto& oj : j["objects"]) {
            std::string name    = oj.value("name", "Object");
            std::string objPath = oj.value("obj", "");

            // Posição
            glm::vec3 pos(0.0f);
            if (oj.contains("position")) pos = readVec3(oj["position"]);

            // Rotação (graus)
            glm::vec3 rot(0.0f);
            if (oj.contains("rotation")) rot = readVec3(oj["rotation"]);

            // Escala: número uniforme ou array [x,y,z]
            glm::vec3 scl(1.0f);
            if (oj.contains("scale")) {
                auto& sv = oj["scale"];
                if (sv.is_number()) {
                    float s = sv.get<float>();
                    scl = glm::vec3(s, s, s);
                } else if (sv.is_array()) {
                    scl = readVec3(sv);
                }
            }

            // Carrega o modelo 3D
            std::shared_ptr<Model> model;
            if (!objPath.empty()) {
                try {
                    model = std::make_shared<Model>(objPath);
                } catch (const std::exception& e) {
                    std::cerr << "[SceneLoader] Erro ao carregar modelo '" << name << "': "
                              << e.what() << "\n";
                    model = std::make_shared<Model>(""); // modelo vazio
                }
            }

            SceneObject obj(name, model, pos, rot, scl);

            // --- Animação Bézier ---
            if (oj.contains("animation")) {
                auto& anim = oj["animation"];
                std::string animType = anim.value("type", "");
                if (animType == "bezier" && anim.contains("control_points")) {
                    float speed = anim.value("speed", 0.3f);
                    std::vector<glm::vec3> pts;
                    for (auto& pt : anim["control_points"]) {
                        pts.push_back(readVec3(pt));
                    }
                    if (pts.size() == 4) {
                        auto bezier = std::make_unique<Bezier>(pts, speed);
                        obj.setAnimation(std::move(bezier));
                    }
                }
            }

            // --- Trajetória (M6) ---
            if (oj.contains("trajectory")) {
                auto& tj    = oj["trajectory"];
                float speed = tj.value("speed",  0.5f);
                bool cyclic = tj.value("cyclic", true);

                std::vector<glm::vec3> pts;
                if (tj.contains("control_points")) {
                    for (auto& pt : tj["control_points"])
                        pts.push_back(readVec3(pt));
                }

                if (pts.size() >= 2) {
                    auto traj = std::make_unique<Trajectory>(pts, speed, cyclic);
                    bool startActive = tj.value("start_active", false);
                    traj->setActive(startActive);
                    obj.setTrajectory(std::move(traj));
                }
            }

            cfg.objects.push_back(std::move(obj));
        }
    }

    return cfg;
}
