#include "SceneObject.h"
#include <glm/gtc/matrix_transform.hpp>

SceneObject::SceneObject(const std::string& name,
                         std::shared_ptr<Model> model,
                         glm::vec3 pos,
                         glm::vec3 rotDeg,
                         glm::vec3 scaleVec)
    : name(name)
    , model(std::move(model))
    , position(pos)
    , rotation(rotDeg)
    , scaleVec(scaleVec)
{}

// === CONSTRUÇÃO DA MATRIZ MODEL ===
// Aplicação na ordem: escala → rotação (X,Y,Z) → translação
glm::mat4 SceneObject::buildModelMatrix() const {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, position);
    m = glm::rotate(m, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    m = glm::rotate(m, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    m = glm::rotate(m, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    m = glm::scale(m, scaleVec);
    return m;
}

void SceneObject::draw(Shader& shader) const {
    // Envia a matriz model calculada para o shader
    shader.setMat4("uModel", buildModelMatrix());
    if (model) model->draw(shader);
}

void SceneObject::update(float dt) {
    if (bezier && bezier->isActive()) {
        bezier->update(dt);
        position = bezier->getCurrentPosition();
    }
}

void SceneObject::translate(glm::vec3 delta) {
    position += delta;
}

void SceneObject::rotateAxis(int axis, float angleDeg) {
    if      (axis == 0) rotation.x += angleDeg;
    else if (axis == 1) rotation.y += angleDeg;
    else if (axis == 2) rotation.z += angleDeg;
}

void SceneObject::scaleBy(float factor) {
    scaleVec *= factor;
}

void SceneObject::setAnimation(std::unique_ptr<Bezier> b) {
    bezier = std::move(b);
}

bool SceneObject::hasAnimation() const {
    return bezier != nullptr;
}

void SceneObject::setAnimationActive(bool a) {
    if (!bezier) return;
    // Se for ativar e a animação já chegou ao fim, reinicia do começo
    if (a && !bezier->isActive()) {
        bezier->reset();
        position = bezier->getCurrentPosition();
    }
    bezier->setActive(a);
}

bool SceneObject::isAnimationActive() const {
    return bezier && bezier->isActive();
}
