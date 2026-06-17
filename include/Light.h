#pragma once

#include <glm/glm.hpp>
#include <string>

// Struct que representa uma fonte de luz pontual na cena
struct Light {
    std::string name      = "";
    glm::vec3   position  = glm::vec3(0.0f, 5.0f, 0.0f);
    glm::vec3   color     = glm::vec3(1.0f);
    float       intensity = 1.0f;
    bool        enabled   = true;
};
