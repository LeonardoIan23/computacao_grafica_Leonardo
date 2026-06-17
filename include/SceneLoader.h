#pragma once

#include "Light.h"
#include "SceneObject.h"
#include <glm/glm.hpp>
#include <array>
#include <string>
#include <vector>

// Configuração completa da cena lida do arquivo JSON
struct SceneConfig {
    int         windowWidth  = 1280;
    int         windowHeight = 720;
    std::string windowTitle  = "Diorama - Estacao de Metro";

    glm::vec3 camPosition = glm::vec3(0.0f, 3.0f, 10.0f);
    float     camYaw      = -90.0f;
    float     camPitch    =   0.0f;
    float     fov         =  60.0f;
    float     nearP       =   0.1f;
    float     farP        = 200.0f;

    std::string floorTexture = "";
    std::string wallTexture  = "";

    std::array<Light, 3>       lights;
    std::vector<SceneObject>   objects;
};

// Carrega a configuração da cena a partir de um arquivo JSON
class SceneLoader {
public:
    // === PARSER DO SCENE.JSON ===
    static SceneConfig load(const std::string& jsonPath);
};
