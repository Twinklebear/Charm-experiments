#include <algorithm>
#include <cmath>
#include "mc.h"

namespace pt {

glm::vec3 cos_sample_hemisphere(const float *samples) {
    // We use Malley's method here, generate samples on a disk then project
    // them up to the hemisphere
    const glm::vec2 d = concentric_sample_disk(samples);
    return glm::vec3(d.x, d.y, std::sqrt(std::max(0.f, 1.f - d.x * d.x - d.y * d.y)));
}
glm::vec2 concentric_sample_disk(const float *samples) {
    const glm::vec2 s(2.0 * samples[0] - 1.0, 2.0 * samples[1] - 1.0);
    float radius = 0;
    float theta = 0;
    if (s.x == 0.0 && s.y == 0.0) {
        return s;
    }
    if (s.x >= -s.y) {
        if (s.x > s.y) {
            radius = s.x;
            if (s.y > 0.0) {
                theta = s.y / s.x;
            } else {
                theta = 8.0 + s.y / s.x;
            }
        } else {
            radius = s.y;
            theta = 2.0 - s.x / s.y;
        }
    } else if (s.x <= s.y) {
            radius = -s.x;
            theta = 4.0 + s.y / s.x;
    } else {
        radius = -s.y;
        theta = 6.0 - s.x / s.y;
    }
    theta = theta * glm::quarter_pi<float>();
    return glm::vec2(radius * std::cos(theta), radius * std::sin(theta));
}

}

