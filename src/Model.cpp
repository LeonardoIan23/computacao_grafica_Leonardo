// STB_IMAGE: definir implementação apenas neste arquivo
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Model.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

// Extrai o diretório de um caminho de arquivo
static std::string extractDirectory(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? "./" : path.substr(0, pos + 1);
}

Model::Model(const std::string& objPath) {
    directory = extractDirectory(objPath);
    loadOBJ(objPath);

    // Se o caminho de textura foi encontrado, carrega a imagem
    if (!material.texturePath.empty()) {
        material.textureID = loadTexture(material.texturePath);
    }
    // Garante textura fallback se não encontrou ou falhou ao carregar
    if (material.textureID == 0) {
        material.textureID = whiteFallback();
    }
}

// === PARSER DO .OBJ ===
void Model::loadOBJ(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Model] Nao foi possivel abrir: " << path << "\n";
        // Cria malha vazia para evitar crash
        mesh = std::make_unique<Mesh>(std::vector<Vertex>{});
        return;
    }

    // Listas temporárias de atributos
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<Vertex>    vertices;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "v") {
            // Posição do vértice
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);

        } else if (token == "vt") {
            // Coordenada de textura
            glm::vec2 tc;
            iss >> tc.x >> tc.y;
            texCoords.push_back(tc);

        } else if (token == "vn") {
            // Normal do vértice
            glm::vec3 n;
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);

        } else if (token == "mtllib") {
            // Referência ao arquivo de material
            std::string mtlName;
            iss >> mtlName;
            loadMTL(directory + mtlName);

        } else if (token == "f") {
            // Face: lê todos os vértices e aplica fan triangulation para N>3
            std::vector<Vertex> faceVerts;
            std::string vtxStr;
            while (iss >> vtxStr) {
                // Formato: v/vt/vn
                Vertex vtx{};
                int vi = 0, ti = 0, ni = 0;

                // Substitui '/' por espaço para facilitar parsing
                for (char& c : vtxStr) if (c == '/') c = ' ';
                std::istringstream vs(vtxStr);
                vs >> vi >> ti >> ni;

                if (vi != 0 && vi <= (int)positions.size())
                    vtx.position = positions[vi - 1];
                if (ti != 0 && ti <= (int)texCoords.size())
                    vtx.texCoord = texCoords[ti - 1];
                if (ni != 0 && ni <= (int)normals.size())
                    vtx.normal   = normals[ni - 1];

                faceVerts.push_back(vtx);
            }

            // Fan triangulation: (0,k,k+1) para k = 1..N-2
            for (size_t k = 1; k + 1 < faceVerts.size(); ++k) {
                vertices.push_back(faceVerts[0]);
                vertices.push_back(faceVerts[k]);
                vertices.push_back(faceVerts[k + 1]);
            }
        }
    }

    mesh = std::make_unique<Mesh>(vertices);
}

// === PARSER DO .MTL ===
void Model::loadMTL(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Model] MTL nao encontrado: " << path << "\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "Ka") {
            iss >> material.Ka.r >> material.Ka.g >> material.Ka.b;
        } else if (token == "Kd") {
            iss >> material.Kd.r >> material.Kd.g >> material.Kd.b;
        } else if (token == "Ks") {
            iss >> material.Ks.r >> material.Ks.g >> material.Ks.b;
        } else if (token == "Ns") {
            iss >> material.Ns;
        } else if (token == "map_Kd") {
            std::string texName;
            iss >> texName;
            material.texturePath = directory + texName;
        }
    }
}

// Carrega uma textura do disco para a GPU
GLuint Model::loadTexture(const std::string& path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "[Model] Falha ao carregar textura: " << path << "\n";
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 4) format = GL_RGBA;
    else if (channels == 1) format = GL_RED;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
                 format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Parâmetros de filtragem e repetição
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return texID;
}

// Cria textura 1x1 branca como fallback quando a textura não é encontrada
GLuint Model::whiteFallback() {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    unsigned char white[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, white);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texID;
}

// Envia uniforms de material para o shader e emite chamada de desenho
void Model::draw(Shader& shader) const {
    // Enviar parâmetros do material
    shader.setVec3 ("uKa", material.Ka);
    shader.setVec3 ("uKd", material.Kd);
    shader.setVec3 ("uKs", material.Ks);
    shader.setFloat("uNs", material.Ns);

    // Bind da textura na unidade 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material.textureID);
    shader.setInt("uTexture", 0);

    if (mesh) mesh->draw();
}
