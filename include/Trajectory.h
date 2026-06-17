#pragma once

#include <glm/glm.hpp>
#include <vector>

// Trajetória cíclica com N pontos de controle (interpolação linear entre waypoints)
// O parâmetro t percorre [0, N-1]: cada unidade inteira representa um segmento.
// speed = segmentos por segundo (ex.: 0.5 → 2 segundos entre cada par de pontos)
class Trajectory {
public:
    float speed  = 0.5f;
    bool  cyclic = true;

    Trajectory() = default;
    Trajectory(const std::vector<glm::vec3>& pts, float speed, bool cyclic = true);

    glm::vec3 getCurrentPosition() const;
    void      update(float dt);

    void setActive(bool a);
    bool isActive()  const { return active; }

    // Retorna true se há pelo menos 2 pontos (mínimo para interpolar)
    bool hasPoints() const { return waypoints.size() >= 2; }

    void reset();
    void addPoint(glm::vec3 pt);
    void clearPoints();

    const std::vector<glm::vec3>& getPoints() const { return waypoints; }

private:
    std::vector<glm::vec3> waypoints;
    float t      = 0.0f;
    bool  active = false;
};
