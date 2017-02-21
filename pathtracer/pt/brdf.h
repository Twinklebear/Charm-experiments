#pragma once

#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>

namespace pt {

enum BRDFType { Diffuse, Specular, Reflection, Transmission };

struct BxDFSample {
	// Sampled BRDF value
	glm::vec3 color;
	// The sampled incident light direction
	glm::vec3 w_i;
	float pdf;

	BxDFSample(const glm::vec3 &color, const glm::vec3 &w_i, const float pdf);
};

/* Defines the reflectace of a surface
 * Note: all vectors passed to this objects methods are assumed to be
 * in shading space
 */
class BxDF {
public:
	virtual ~BxDF(){}
	virtual int bxdf_type() const = 0;
	/* Evaluate the BxDF for the incident (w_i) and outgoing (w_o)
	 * light direction pair. Sample should point to 2 floats for use as random samples
	 */
	virtual glm::vec3 eval(const glm::vec3 &w_i, const glm::vec3 &w_o) const = 0;
	// Sample an incident light direction for an outgoing light direction w_o
	virtual BxDFSample sample(const glm::vec3 &w_o, const float *samples) const;
	// Compute the PDF of sampling the pair of directions passed for this BxDF
	virtual float pdf(const glm::vec3 &w_i, const glm::vec3 &w_o) const;

	// Utilities for shading space trig
	/// Compute the value of cosine theta for a vector in shading space
	static inline float cos_theta(const glm::vec3 &v) { return v.z; }
	/// Compute the value of (sine theta)^2  for a vector in shading space
	static inline float sin_theta_sqr(const glm::vec3 &v) { return std::max(0.f, 1.f - v.z * v.z); }
	/// Compute the value of sine theta for a vector in shading space
	static inline float sin_theta(const glm::vec3 &v) { return std::sqrt(sin_theta_sqr(v)); }
	/// Compute the value of tan theta for a vector in shading space
	static inline float tan_theta(const glm::vec3 &v) {
		const float sin_theta_2 = BxDF::sin_theta_sqr(v);
		if (sin_theta_2 <= 0.f) {
			return 0.f;
		}
		return std::sqrt(sin_theta_2) / BxDF::cos_theta(v);
	}
	/// Compute the value of arctan theta for a vector in shading space
	static inline float arctan_theta(const glm::vec3 &v) {
		return BxDF::cos_theta(v) / BxDF::sin_theta(v);
	}
	/// Compute the value of cosine phi for a vector in shading space
	static inline float cos_phi(const glm::vec3 &v) {
		const float sin_theta = BxDF::sin_theta(v);
		if (sin_theta == 0.f) {
			return 1.f;
		}
		return glm::clamp(v.x / sin_theta, -1.f, 1.f);
	}
	/// Compute the value of sine phi for a vector in shading space
	static inline float sin_phi(const glm::vec3 &v) {
		const float sin_theta = BxDF::sin_theta(v);
		if (sin_theta == 0.f) {
			return 0.f;
		}
		return glm::clamp(v.y / sin_theta, -1.f, 1.f);
	}
	/// Check if two vectors are in the same hemisphere in shading space
	static inline bool same_hemisphere(const glm::vec3 &a, const glm::vec3 &b) {
		return a.z * b.z > 0.0;
	}
};

class Lambertian : public BxDF {
	glm::vec3 reflectance;

public:
	Lambertian(const glm::vec3 &reflectance);
	int bxdf_type() const override;
	glm::vec3 eval(const glm::vec3 &w_i, const glm::vec3 &w_o) const override;
};

class SpecularReflection : public BxDF {
	glm::vec3 reflectance;

public:
	SpecularReflection(const glm::vec3 &reflectance);
	int bxdf_type() const override;
	// Note: this will always return black as the brdf is a delta function
	glm::vec3 eval(const glm::vec3 &w_i, const glm::vec3 &w_o) const override;
	// Compute and return the specularly reflected direction
	BxDFSample sample(const glm::vec3 &w_o, const float *samples) const override;
	/* Note that for specular reflection the pdf is always 0, besides for the
	 * specular reflection dir returned by eval
	 */
	float pdf(const glm::vec3 &w_i, const glm::vec3 &w_o) const override;
};

}

