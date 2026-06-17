#include "Trajectory.h"
#include <cmath>

Trajectory::Trajectory(const std::vector<glm::vec3>& pts, float speed, bool cyclic)
    : speed(speed), cyclic(cyclic), waypoints(pts)
{}

// Posição atual: interpolação linear entre waypoints[seg] e waypoints[seg+1]
glm::vec3 Trajectory::getCurrentPosition() const {
    if (waypoints.empty()) return glm::vec3(0.0f);
    if (waypoints.size() == 1) return waypoints[0];

    int   N    = static_cast<int>(waypoints.size());
    int   seg  = static_cast<int>(t);
    float frac = t - static_cast<float>(seg);

    if (seg >= N - 1) { seg = N - 2; frac = 1.0f; }

    return glm::mix(waypoints[seg], waypoints[seg + 1], frac);
}

void Trajectory::update(float dt) {
    if (!active || waypoints.size() < 2) return;

    float maxT = static_cast<float>(waypoints.size() - 1);
    t += speed * dt;

    if (t >= maxT) {
        if (cyclic) {
            t = std::fmod(t, maxT);
        } else {
            t = maxT;
            active = false;
        }
    }
}

void Trajectory::setActive(bool a) {
    active = a;
}

void Trajectory::reset() {
    t = 0.0f;
}

void Trajectory::addPoint(glm::vec3 pt) {
    waypoints.push_back(pt);
}

void Trajectory::clearPoints() {
    waypoints.clear();
    t      = 0.0f;
    active = false;
}
