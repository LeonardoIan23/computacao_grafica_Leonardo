#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

// Encapsula a compilação, linkagem e uso de um programa GLSL
class Shader {
public:
    GLuint ID = 0; // ID do programa shader

    // Lê os arquivos .glsl, compila e linka o programa
    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    // Não permitir cópia
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    // Ativa este shader program
    void use() const;

    // === PASSAGEM DE UNIFORMS PARA O SHADER ===
    void setMat4 (const std::string& name, const glm::mat4& value) const;
    void setVec3 (const std::string& name, const glm::vec3& value) const;
    void setFloat(const std::string& name, float value)            const;
    void setInt  (const std::string& name, int value)              const;
    void setBool (const std::string& name, bool value)             const;

private:
    // Lê o conteúdo de um arquivo de texto
    static std::string readFile(const std::string& path);

    // Compila um shader individual e retorna seu ID
    static GLuint compileShader(GLenum type, const std::string& source,
                                const std::string& label);
};
