#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

Camera::Camera(glm::vec3 pos, float yaw, float pitch)
    : position(pos), yaw(yaw), pitch(pitch)
{
    worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    updateVectors();
}

// Recalcula os vetores front, right e up a partir de yaw/pitch
void Camera::updateVectors() {
    glm::vec3 f;
    f.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    f.y = std::sin(glm::radians(pitch));
    f.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front = glm::normalize(f);
    right = glm::normalize(glm::cross(front, worldUp));
    up    = glm::normalize(glm::cross(right, front));
}

// === CONSTRUÇÃO DA MATRIZ VIEW (lookAt) ===
glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float fov, float aspect,
                                       float nearP, float farP) const {
    return glm::perspective(glm::radians(fov), aspect, nearP, farP);
}

void Camera::moveForward(float dt) { position += front * speed * dt; }
void Camera::moveBack   (float dt) { position -= front * speed * dt; }
void Camera::moveLeft   (float dt) { position -= right * speed * dt; }
void Camera::moveRight  (float dt) { position += right * speed * dt; }
void Camera::moveUp     (float dt) { position += worldUp * speed * dt; }
void Camera::moveDown   (float dt) { position -= worldUp * speed * dt; }

void Camera::rotate(float xOff, float yOff) {
    yaw   += xOff * sensitivity;
    pitch += yOff * sensitivity;
    // Limitar o pitch para evitar gimbal lock
    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    updateVectors();
}
