#pragma once

#include "Model.h"
#include "Bezier.h"
#include "Trajectory.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

// Objeto da cena: contém transformações, modelo 3D e animações opcionais
class SceneObject {
public:
    std::string name;
    bool        selected = false;

    SceneObject(const std::string& name,
                std::shared_ptr<Model> model,
                glm::vec3 pos,
                glm::vec3 rotDeg,
                glm::vec3 scaleVec);

    // Envia uModel ao shader e chama model->draw()
    void draw  (Shader& shader) const;

    // Atualiza animações (se ativas)
    void update(float dt);

    // Manipulação de transformações
    void translate  (glm::vec3 delta);
    void rotateAxis (int axis, float angleDeg); // 0=X, 1=Y, 2=Z
    void scaleBy    (float factor);

    // === Animação Bézier (M4/M5) ===
    void setAnimation      (std::unique_ptr<Bezier> b);
    bool hasAnimation      () const;
    void setAnimationActive(bool a);
    bool isAnimationActive () const;

    // === Trajetória cíclica (M6) ===
    void setTrajectory        (std::unique_ptr<Trajectory> traj);
    bool hasTrajectory        () const;             // true se ≥ 2 pontos
    void setTrajectoryActive  (bool a);
    bool isTrajectoryActive   () const;
    void addTrajectoryPoint   (glm::vec3 pt);       // adiciona ponto em runtime
    void clearTrajectoryPoints();
    const std::vector<glm::vec3>& getTrajectoryPoints() const;

    glm::vec3 getPosition() const { return position; }

private:
    std::shared_ptr<Model>      model;
    glm::vec3                   position;
    glm::vec3                   rotation;   // graus (Euler XYZ)
    glm::vec3                   scaleVec;
    std::unique_ptr<Bezier>     bezier;
    std::unique_ptr<Trajectory> trajectory;

    // === CONSTRUÇÃO DA MATRIZ MODEL ===
    glm::mat4 buildModelMatrix() const;
};
