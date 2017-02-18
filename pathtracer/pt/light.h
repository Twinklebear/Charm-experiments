#pragma once

#include "ray.h"
#include "scene.h"

namespace pt {

struct LightSample {
	glm::vec3 illum;
	// Direction to the light
	glm::vec3 dir;
	// Ray to use for testing occlusion from this light
	Ray occlusion_ray;

	LightSample(const glm::vec3 &illum, const glm::vec3 &dir, const Ray &occlusion);
	// Test if the light coming from this sample is occluded
	bool occluded(const Scene &scene) const;
};

class Light {
public:
	virtual ~Light(){}
	virtual LightSample incident(const glm::vec3 &pt) const = 0;
};

class DirectionalLight : public Light {
	glm::vec3 dir, illum;

public:
	DirectionalLight(const glm::vec3 &dir, const glm::vec3 &illum);
	LightSample incident(const glm::vec3 &pt) const override;
};

}

