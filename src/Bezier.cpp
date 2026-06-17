#include "Bezier.h"
#include <cmath>
#include <stdexcept>

Bezier::Bezier(const std::vector<glm::vec3>& pts, float speed)
    : controlPoints(pts), speed(speed)
{
    if (controlPoints.size() != 4) {
        throw std::runtime_error("Bezier: sao necessarios exatamente 4 pontos de controle.");
    }
}

// === AVALIAÇÃO DA CURVA DE BÉZIER CÚBICA ===
// B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
glm::vec3 Bezier::evaluate(float t) const {
    float u  = 1.0f - t;
    float u2 = u  * u;
    float u3 = u2 * u;
    float t2 = t  * t;
    float t3 = t2 * t;

    return u3           * controlPoints[0]
         + 3.0f * u2 * t * controlPoints[1]
         + 3.0f * u  * t2 * controlPoints[2]
         +        t3      * controlPoints[3];
}

glm::vec3 Bezier::getCurrentPosition() const {
    return evaluate(t);
}

void Bezier::update(float dt) {
    if (!active) return;

    t += speed * dt;
    if (t >= 1.0f) {
        t = 1.0f;
        active = false; // Para automaticamente ao chegar no fim
    }
}

void Bezier::setActive(bool a) {
    active = a;
}

bool Bezier::isActive() const {
    return active;
}

void Bezier::reset() {
    t = 0.0f;
}
