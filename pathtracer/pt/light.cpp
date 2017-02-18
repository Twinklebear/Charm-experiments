#include "diff_geom.h"
#include "light.h"

namespace pt {

LightSample::LightSample(const glm::vec3 &illum, const glm::vec3 &dir, const Ray &occlusion)
	: illum(illum), dir(dir), occlusion_ray(occlusion)
{}
bool LightSample::occluded(const Scene &scene) const {
	DifferentialGeometry dg;
	Ray r = occlusion_ray;
	return scene.intersect(r, dg);
}

DirectionalLight::DirectionalLight(const glm::vec3 &dir, const glm::vec3 &illum)
	: dir(glm::normalize(dir)), illum(illum)
{}
LightSample DirectionalLight::incident(const glm::vec3 &pt) const {
	return LightSample(illum, -dir, Ray(pt, -dir, 0.001));
}

}


