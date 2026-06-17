#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

// Struct que armazena os coeficientes do material lidos do arquivo .mtl
struct Material {
    glm::vec3 Ka = glm::vec3(0.2f);    // Coeficiente ambiente
    glm::vec3 Kd = glm::vec3(0.8f);    // Coeficiente difuso
    glm::vec3 Ks = glm::vec3(0.5f);    // Coeficiente especular
    float     Ns = 32.0f;              // Expoente especular (shininess)

    GLuint      textureID   = 0;       // ID da textura OpenGL (0 = sem textura)
    std::string texturePath = "";      // Caminho para o arquivo de textura
};
