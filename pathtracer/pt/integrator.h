#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "ray.h"

namespace pt {

enum RAY_TYPE {
	PRIMARY,
	SHADOW,
	SECONDARY
};

struct BVHTraversalState {
	size_t current;
	size_t bitstack;

	BVHTraversalState();
};

/* A ray being actively traversed through the scene, can be
 * either a shadow ray or a secondary (continuation) ray. On
 * a shadow ray the color is the final shaded color for the hit
 * point, and all the receiving end must do is determine if the
 * ray hits something before it hits the light. On a secondary ray
 * the color is the path throughput, for path tracing.
 *
 * Once the shadow test has been executed or the secondary ray
 * terminated the node on which it finishes must report the
 * shading result back to the owner.
 */
struct ActiveRay {
	RAY_TYPE type;
	Ray ray;
	BVHTraversalState traversal;
	glm::vec4 color;
	uint64_t owner_id, tile, pixel;
	// The number of shadow rays spawned from this ray
	uint64_t children;

	ActiveRay(const Ray &r, const uint64_t owner_id, const uint64_t tile,
			const uint64_t pixel);
	// TODO: Maybe just return a unique_ptr?
	static ActiveRay* shadow(const Ray &r, const ActiveRay &parent);
	static ActiveRay* secondary(const Ray &r, const ActiveRay &parent);
};

struct IntersectionResult {
	// TODO: For the shadow ray, I can compute the entire shading
	// result that would be sent because all nodes have all the light
	// information, since they're delta or analytic lights. The only
	// thing a node doesn't know is if there's some geometry on another
	// node blocking its shading point from the light.
	std::unique_ptr<ActiveRay> shadow, secondary;
};

}

std::ostream& operator<<(std::ostream &os, const pt::ActiveRay &r);
std::ostream& operator<<(std::ostream &os, const pt::BVHTraversalState &s);
