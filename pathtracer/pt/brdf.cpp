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
		return /*std::abs(BxDF::cos_theta(w_i)) */ glm::one_over_pi<float>();
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

SpecularReflection::SpecularReflection(const glm::vec3 &reflectance) : reflectance(reflectance) {}
int SpecularReflection::bxdf_type() const {
	return BRDFType::Specular | BRDFType::Reflection;
}
glm::vec3 SpecularReflection::eval(const glm::vec3 &, const glm::vec3 &) const {
	return glm::vec3(0);
}
BxDFSample SpecularReflection::sample(const glm::vec3 &w_o, const float*) const {
	const glm::vec3 w_i(-w_o.x, -w_o.y, w_o.z);
	if (w_i.z != 0.f) {
		// TODO: Fresnel
		const glm::vec3 color = reflectance / std::abs(BxDF::cos_theta(w_i));
		return BxDFSample(color, w_i, 1.f);
	}
	return BxDFSample(glm::vec3(0), w_i, 0.f);
}
float SpecularReflection::pdf(const glm::vec3 &, const glm::vec3 &) const {
	return 0.0;
}

}

