#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>

// Various Monte Carlo integration utilities

namespace pt {

/* Sample a hemisphere using a cosine distribution to produce cosine weighted samples.
 * 'samples' should be two random samples in range [0, 1). Directions returned will
 * be in the hemisphere around (0, 0, 1)
 */
glm::vec3 cos_sample_hemisphere(const float *samples);
inline float cos_hemisphere_pdf(const float cos_theta) {
	return cos_theta * glm::one_over_pi<float>();
}
/* Compute concentric sample positions on a unit disk mapping input from range [0, 1)
 * to sample positions on a disk. 'samples' should be two random samples in range [0, 1)
 * See: [Shirley and Chiu, A Low Distortion Map Between Disk and Square](https://mediatech.aalto.fi/~jaakko/T111-5310/K2013/JGT-97.pdf)
 */
glm::vec2 concentric_sample_disk(const float *samples);

}

