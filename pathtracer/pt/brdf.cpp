#include <glm/ext.hpp>
#include "mc.h"
#include "brdf.h"

namespace pt {

BxDFSample::BxDFSample(const glm::vec3 &color, const glm::vec3 &w_i, const float pdf)
	: color(color), w_i(w_i), pdf(pdf)
{}

BxDFSample BxDF::sample(const glm::vec3 &w_o, const float *samples) const {
	glm::vec3 w_i = cos_sample_hemisphere(samples);
	if (w_o.z < 0.0) {
		w_i.z *= -1.0;
	}
	return BxDFSample(eval(w_o, w_i), w_i, pdf(w_o, w_i));
}
float BxDF::pdf(const glm::vec3 &w_i, const glm::vec3 &w_o) const {
	if (BxDF::same_hemisphere(w_i, w_o)) {
		return std::abs(BxDF::cos_theta(w_i)) * glm::one_over_pi<float>();
	}
	return 0.0;
}

Lambertian::Lambertian(const glm::vec3 &reflectance) : reflectance(reflectance) {}
int Lambertian::bxdf_type() const {
	return BRDFType::Diffuse | BRDFType::Reflection;
}
glm::vec3 Lambertian::eval(const glm::vec3&, const glm::vec3&) const {
	return reflectance * glm::one_over_pi<float>();
}

}

