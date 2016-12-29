#include "pup_operators.h"
#include "main.decl.h"
#include "main.h"
#include "volume_brick.decl.h"
#include "volume_brick.h"

// readonly global Charm++ variables
extern CProxy_Main main_proxy;
extern SceneMessage *scene;
extern uint64_t IMAGE_W;
extern uint64_t IMAGE_H;
extern uint64_t TILE_W;
extern uint64_t TILE_H;

VolumeBrick::VolumeBrick() : brick_dims(scene->dims / scene->bricking),
	offset(glm::uvec3(thisIndex.x, thisIndex.y, thisIndex.z) * brick_dims)
{
	CkPrintf("VolumeBrick [%d, %d, %d] will render brick at [%d, %d, %d]"
			" of dims [%d, %d, %d]\n", thisIndex.x, thisIndex.y, thisIndex.z,
			offset.x, offset.y, offset.z, brick_dims.x, brick_dims.y, brick_dims.z);
	volume = sv::load_raw_volume(scene->vol_file, scene->dims, scene->dtype,
			offset, brick_dims);
	CkPrintf("VolumeBrick [%d, %d, %d] value range = [%f, %f]\n",
			thisIndex.x, thisIndex.y, thisIndex.z, volume->get_min(), volume->get_max());
}
VolumeBrick::VolumeBrick(CkMigrateMessage *msg) {}
void VolumeBrick::pup(PUP::er &p) {
	p | offset;
	p | brick_dims;
	if (p.isUnpacking()) {
		volume = sv::load_raw_volume(scene->vol_file, scene->dims, scene->dtype,
				offset, brick_dims);
	}
}
void VolumeBrick::render() {
	const uint64_t tiles_x = IMAGE_W / TILE_W;
	const uint64_t tiles_y = IMAGE_H / TILE_H;
	const sv::Camera camera(scene->cam_pos, scene->cam_target, scene->cam_up, 65.0, IMAGE_W, IMAGE_H);

	const sv::RaycastRender renderer(0.5, volume);
	// Each tile is RGB8 color data
	//uint8_t *tile = new uint8_t[TILE_W * TILE_H * 3];
	uint8_t *image = new uint8_t[IMAGE_W * IMAGE_H * 3];
	for (uint64_t y = 0; y < tiles_y; ++y) {
		for (uint64_t x = 0; x < tiles_x; ++x) {
			const uint64_t start_x = x * TILE_W;
			const uint64_t start_y = y * TILE_H;
			for (uint64_t i = 0; i < TILE_H; ++i) {
				for (uint64_t j = 0; j < TILE_W; ++j) {
					const float px = j + start_x;
					const float py = i + start_y;
					sv::Ray ray = camera.generate_ray(px, py, {0.5, 0.5});
					const glm::vec4 vol_color = renderer.render(ray);
					glm::vec3 col = glm::vec3(vol_color) + (1.f - vol_color.w) * glm::vec3(1);
					// Composite onto the background color
					for (size_t c = 0; c < 3; ++c) {
						image[((i + start_y) * IMAGE_W + j + start_x) * 3 + c] = col[c] * 255.f;
					}
				}
			}
		}
	}
	const std::string brick_outname = "scivis_render" + std::to_string(thisIndex.x)
		+ "x" + std::to_string(thisIndex.y) + "x" + std::to_string(thisIndex.z) + ".png";
	stbi_write_png(brick_outname.c_str(), IMAGE_W, IMAGE_H, 3, image, IMAGE_W * 3);
	delete[] image;
	main_proxy.brick_done();
}

#include "volume_brick.def.h"

