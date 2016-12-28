#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "sv/scivis.h"

/* This is a really bad image-parallel renderer since each tile loads the
 * volume independently. TODO: Maybe use node-group or something to load it
 * once per node? Could also do a readonly global message but I think that's
 * not as clean of a design.
 *
 * TODO: Maybe what I want to do is write pup | operators for glm types,
 * and then to send the scene data I want to make a custom packed SceneMessage
 * message which can use the pup serialization stuff to write itself to the buffer.
 */
class ImageParallelTile : public CBase_ImageParallelTile {
	std::string vol_file;
	glm::uvec3 dims;
	sv::VolumeDType dtype;

public:
	ImageParallelTile(const std::string vol, glm::uvec3 dims, sv::VolumeDType dtype);
	ImageParallelTile(CkMigrateMessage *msg);
	void pup(PUP::er &p) override;
	void render();
};

