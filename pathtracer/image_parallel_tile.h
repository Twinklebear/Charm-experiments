#pragma once

#include <memory>
#include <random>
#include <glm/glm.hpp>
#include "pt/pt.h"

/* This is a pretty bad image-parallel renderer since each processor (core, not node!)
 * loads the volume independently when we're unpacking the SceneMessage.
 * TODO: Maybe use node-group or something to load it once per node?
 */
class ImageParallelTile : public CBase_ImageParallelTile {
	std::mt19937 rng;

public:
	ImageParallelTile();
	ImageParallelTile(CkMigrateMessage *msg);

	void pup(PUP::er &p) override;
	// Render a single sample for each pixel in this tile
	void render();
};

