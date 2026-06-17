#pragma once

#include "Mesh.h"
#include "Material.h"
#include "Shader.h"
#include <memory>
#include <string>

// Carrega um modelo 3D a partir de um arquivo .obj e seu .mtl associado
class Model {
public:
    explicit Model(const std::string& objPath);

    // Envia uniforms de material e textura ao shader e emite chamada de desenho
    void draw(Shader& shader) const;

    const Material& getMaterial() const { return material; }

private:
    std::unique_ptr<Mesh> mesh;
    Material              material;
    std::string           directory; // Diretório do arquivo .obj

    // === PARSER DO .OBJ ===
    void loadOBJ(const std::string& path);

    // === PARSER DO .MTL ===
    void loadMTL(const std::string& path);

    // Carrega uma textura do disco e retorna o ID OpenGL
    GLuint loadTexture(const std::string& path);

    // Cria uma textura 1x1 branca como fallback
    GLuint whiteFallback();
};
