#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

// Lê o conteúdo de um arquivo de texto para uma string
std::string Shader::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Shader: nao foi possivel abrir o arquivo: " + path);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Compila um shader (vertex ou fragment) e verifica erros
GLuint Shader::compileShader(GLenum type, const std::string& source, const std::string& label) {
    GLuint id     = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    // Verificação de erros de compilação
    GLint success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(id, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "[Shader] Erro de compilacao em '" << label << "':\n"
                  << infoLog << "\n";
    }
    return id;
}

Shader::Shader(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSrc, fragSrc;
    try {
        vertSrc = readFile(vertPath);
        fragSrc = readFile(fragPath);
    } catch (const std::exception& e) {
        std::cerr << "[Shader] " << e.what() << "\n";
        return;
    }

    GLuint vert = compileShader(GL_VERTEX_SHADER,   vertSrc, vertPath);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc, fragPath);

    // Linkagem do programa
    ID = glCreateProgram();
    glAttachShader(ID, vert);
    glAttachShader(ID, frag);
    glLinkProgram(ID);

    // Verificação de erros de linkagem
    GLint success;
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(ID, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "[Shader] Erro de linkagem:\n" << infoLog << "\n";
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

Shader::~Shader() {
    if (ID) glDeleteProgram(ID);
}

void Shader::use() const {
    glUseProgram(ID);
}

// === PASSAGEM DE UNIFORMS PARA O SHADER ===

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()),
                       1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()),
                 1, glm::value_ptr(value));
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), static_cast<int>(value));
}
