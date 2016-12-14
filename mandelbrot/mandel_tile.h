#pragma once

#include <array>

class MandelTile : public CBase_MandelTile {
	// Axis range to compute the Mandelbrot set on
	std::array<float, 2> x_axis;
	std::array<float, 2> y_axis;
	uint64_t max_iters;
	uint64_t subsamples;

public:
	MandelTile(uint64_t subsamples);
	MandelTile(CkMigrateMessage *msg);

	// Entry method for rendering this chare's tile.
	void render();

private:
	// Compute the exit iteration count for some pixel
	unsigned int mandel(const float c_real, const float c_imag);
};

