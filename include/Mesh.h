#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// Vértice com posição, coordenada de textura e normal
struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;
};

// Classe que encapsula VAO/VBO e o desenho de uma malha triangular
class Mesh {
public:
    // Constrói a malha a partir de um vetor de vértices e faz upload para a GPU
    explicit Mesh(const std::vector<Vertex>& verts);
    ~Mesh();

    // Não permitir cópia (recurso OpenGL)
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Emite chamada de desenho (glDrawArrays)
    void draw() const;

private:
    GLuint VAO = 0, VBO = 0;
    GLsizei vertexCount = 0;

    // Configura os atributos de vértice no VAO
    void setupMesh(const std::vector<Vertex>& verts);
};
