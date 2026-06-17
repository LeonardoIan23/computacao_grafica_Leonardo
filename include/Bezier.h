#pragma once

#include <glm/glm.hpp>
#include <vector>

// Animação de translação sobre uma curva de Bézier cúbica
class Bezier {
public:
    // pts: exatamente 4 pontos de controle; speed: velocidade de percurso (t/s)
    Bezier(const std::vector<glm::vec3>& pts, float speed);

    // Posição atual na curva
    glm::vec3 getCurrentPosition() const;

    // Avança o parâmetro t pelo delta de tempo
    void update(float dt);

    void setActive(bool a);
    bool isActive() const;

    // Reinicia a animação (t = 0)
    void reset();

private:
    std::vector<glm::vec3> controlPoints; // 4 pontos para Bézier cúbica
    float speed;
    float t      = 0.0f;
    bool  active = false;

    // === AVALIAÇÃO DA CURVA DE BÉZIER CÚBICA ===
    // B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
    glm::vec3 evaluate(float t) const;
};
