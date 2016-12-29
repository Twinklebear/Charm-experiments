#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "sv/scivis.h"

/* A brick to be rendered of a distributed volume.
 * TODO: Only render tiles which the brick projects on to
 */
class VolumeBrick : public CBase_VolumeBrick {
	// The dimensions of this subbrick of the volume
	glm::uvec3 brick_dims;
	// Offset in the volume that this block is rendering
	glm::uvec3 offset;
	std::shared_ptr<sv::Volume> volume;

public:
	VolumeBrick();
	VolumeBrick(CkMigrateMessage *msg);
	void pup(PUP::er &p);
	void render();
};

