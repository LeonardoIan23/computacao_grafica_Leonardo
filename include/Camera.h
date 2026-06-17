#pragma once

#include <glm/glm.hpp>

// Câmera FPS: controle por teclado e mouse
class Camera {
public:
    glm::vec3 position;
    float     yaw;
    float     pitch;
    float     speed       = 5.0f;
    float     sensitivity = 0.1f;

    Camera(glm::vec3 pos   = glm::vec3(0.0f, 3.0f, 10.0f),
           float     yaw   = -90.0f,
           float     pitch =   0.0f);

    // === CONSTRUÇÃO DA MATRIZ VIEW (lookAt) ===
    glm::mat4 getViewMatrix() const;

    // Retorna a matriz de projeção perspectiva
    glm::mat4 getProjectionMatrix(float fov, float aspect,
                                  float nearP, float farP) const;

    // Movimentos
    void moveForward(float dt);
    void moveBack   (float dt);
    void moveLeft   (float dt);
    void moveRight  (float dt);
    void moveUp     (float dt);
    void moveDown   (float dt);

    // Rotação via delta do mouse (em pixels)
    void rotate(float xOff, float yOff);

private:
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 worldUp{0.0f, 1.0f, 0.0f};

    // Recalcula front, right, up a partir de yaw e pitch
    void updateVectors();
};
