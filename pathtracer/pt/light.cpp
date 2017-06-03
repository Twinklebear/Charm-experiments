#include <iostream>
#include <glm/ext.hpp>
#include "diff_geom.h"
#include "light.h"

namespace pt {

LightSample::LightSample(const glm::vec3 &illum, const glm::vec3 &dir, const Ray &occlusion)
	: illum(illum), dir(dir), occlusion_ray(occlusion)
{}
bool LightSample::occluded(const Scene &scene) const {
	DifferentialGeometry dg;
	Ray r = occlusion_ray;
	//  Occluded only makes sense for testing against the local data
	assert(!scene.bvh);
	return scene.intersect(r, dg);
}

DirectionalLight::DirectionalLight(const glm::vec3 &dir, const glm::vec3 &illum)
	: dir(glm::normalize(dir)), illum(illum)
{}
LightSample DirectionalLight::incident(const glm::vec3 &pt) const {
	return LightSample(illum, -dir, Ray(pt, -dir, 0.001));
}

PointLight::PointLight(const glm::vec3 &position, const glm::vec3 &illum) : position(position), illum(illum) {}
LightSample PointLight::incident(const glm::vec3 &pt) const {
	const glm::vec3 dir = position - pt;
	return LightSample(illum / glm::length2(dir), glm::normalize(dir), Ray(pt, dir, 0.001, 0.999));
}

}


