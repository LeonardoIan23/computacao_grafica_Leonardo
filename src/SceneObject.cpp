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
// Ordem: escala → rotação (X,Y,Z) → translação
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
    shader.setMat4("uModel", buildModelMatrix());
    if (model) model->draw(shader);
}

void SceneObject::update(float dt) {
    // Trajetória tem prioridade sobre Bézier quando ambas estiverem ativas
    if (trajectory && trajectory->isActive()) {
        trajectory->update(dt);
        position = trajectory->getCurrentPosition();
    } else if (bezier && bezier->isActive()) {
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

// --- Bézier (M4/M5) ---

void SceneObject::setAnimation(std::unique_ptr<Bezier> b) {
    bezier = std::move(b);
}

bool SceneObject::hasAnimation() const {
    return bezier != nullptr;
}

void SceneObject::setAnimationActive(bool a) {
    if (!bezier) return;
    if (a && !bezier->isActive()) {
        bezier->reset();
        position = bezier->getCurrentPosition();
    }
    bezier->setActive(a);
}

bool SceneObject::isAnimationActive() const {
    return bezier && bezier->isActive();
}

// --- Trajetória (M6) ---

void SceneObject::setTrajectory(std::unique_ptr<Trajectory> traj) {
    trajectory = std::move(traj);
}

bool SceneObject::hasTrajectory() const {
    return trajectory && trajectory->hasPoints();
}

void SceneObject::setTrajectoryActive(bool a) {
    if (!trajectory || !trajectory->hasPoints()) return;
    if (a) {
        trajectory->reset();
        position = trajectory->getCurrentPosition();
    }
    trajectory->setActive(a);
}

bool SceneObject::isTrajectoryActive() const {
    return trajectory && trajectory->isActive();
}

void SceneObject::addTrajectoryPoint(glm::vec3 pt) {
    if (!trajectory) {
        trajectory = std::make_unique<Trajectory>();
    }
    trajectory->addPoint(pt);
}

void SceneObject::clearTrajectoryPoints() {
    if (trajectory) trajectory->clearPoints();
}

static const std::vector<glm::vec3> s_emptyPts;

const std::vector<glm::vec3>& SceneObject::getTrajectoryPoints() const {
    return trajectory ? trajectory->getPoints() : s_emptyPts;
}
