#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "pt/pt.h"

/* This is a pretty bad image-parallel renderer since each processor (core, not node!)
 * loads the volume independently when we're unpacking the SceneMessage.
 * TODO: Maybe use node-group or something to load it once per node?
 */
class ImageParallelTile : public CBase_ImageParallelTile {
public:
	ImageParallelTile();
	ImageParallelTile(CkMigrateMessage *msg);
	void render();
};

