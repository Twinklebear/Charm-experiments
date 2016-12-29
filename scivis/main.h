#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include "sv/scivis.h"

class Main : public CBase_Main {
	uint64_t num_tiles;
	uint64_t done_count;
	std::vector<uint8_t> image;

public:
	Main(CkArgMsg *msg);
	Main(CkMigrateMessage *msg);

	void tile_done(const uint64_t x, const uint64_t y, const uint8_t *tile);
	void brick_done();
};

class SceneMessage : public CMessage_SceneMessage {
	SceneMessage();
public:
	// Info about the volume in the scene
	std::string vol_file;
	glm::uvec3 dims, bricking;
	sv::VolumeDType dtype;
	std::shared_ptr<sv::Volume> volume;
	// Info about the camera in the scene
	glm::vec3 cam_pos, cam_target, cam_up;

	SceneMessage(const std::string &vol_file, const glm::uvec3 &dims, const glm::uvec3 &bricking,
			sv::VolumeDType dtype, const glm::vec3 &cam_pos, const glm::vec3 &cam_target,
			const glm::vec3 &cam_up);
	void msg_pup(PUP::er &p);
	bool data_parallel() const;
	static void* pack(SceneMessage *msg);
	static SceneMessage* unpack(void*);
};

