#pragma once

#include <algorithm>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace sv {

class Volume {
protected:
	glm::uvec3 offset, dims;

public:
	Volume(const glm::uvec3 &offset, const glm::uvec3 &dims);
	virtual ~Volume();
	/* Sample the value at a point in the volume, the sample point should be in the
	 * volume space, where the volume is from the origin to dims - 1
	 */
	virtual float sample(const glm::vec3 &p) const = 0;
	virtual float get_max() const = 0;
	virtual float get_min() const = 0;
	const glm::uvec3& get_offset() const;
	const glm::uvec3& get_dims() const;
};

enum class VolumeDType {
	UINT8, UINT16, INT32, FLOAT, DOUBLE
};

// Load the region or subregion of a raw volume file
std::shared_ptr<Volume> load_raw_volume(const std::string &fname, const glm::uvec3 &dims,
		const VolumeDType dtype, const glm::uvec3 &offset = glm::uvec3{0},
		const glm::uvec3 &subregion_dims = glm::uvec3{0});

template<typename T>
class VolumeData : public Volume {
	std::vector<T> data;
	T min, max;

public:
	VolumeData(const glm::uvec3 &offset, const glm::uvec3 &dims)
		: Volume(offset, dims), data(dims.x * dims.y * dims.z, T{}),
		min(std::numeric_limits<T>::max()), max(std::numeric_limits<T>::min())
	{}
	VolumeData(const glm::uvec3 &offset, const glm::uvec3 &dims, std::vector<T> &&v)
		: Volume(offset, dims), data(v), min(std::numeric_limits<T>::max()), max(std::numeric_limits<T>::min())
	{
		auto minmax = std::minmax_element(data.begin(), data.end());
		min = *minmax->first;
		max = *minmax->second;
	}
	VolumeData(const glm::uvec3 &offset, const glm::uvec3 &dims, const void *v)
		: Volume(offset, dims), data(dims.x * dims.y * dims.z, T{}),
		min(std::numeric_limits<T>::max()), max(std::numeric_limits<T>::min())
	{
		const T *vt = static_cast<const T*>(v);
		for (unsigned int i = 0; i < dims.x * dims.y * dims.z; ++i) { 
			data[i] = vt[i];
			min = std::min(min, vt[i]);
			max = std::max(max, vt[i]);
		}
	}
	/* Set a region of the volume data, note that rtstart is in the global volume coordinates,
	 * e.g. if the volume has an offset of (10, 10, 10) to set the 0th voxel for this
	 * subvolume you'd still set starting at (10, 10, 10)
	 */
	void set_region(const glm::uvec3 &rstart, const glm::uvec3 &rsize, const std::vector<T> &region) {
		set_region(rstart, rsize, static_cast<const void*>(region.data()));
	}
	void set_region(const glm::uvec3 &rstart, const glm::uvec3 &rsize, const void *region) {
		const T *vt = static_cast<const T*>(region);
		const glm::uvec3 start = rstart - offset;
		for (size_t z = 0; z < rsize.z; ++z) {
			for (size_t y = 0; y < rsize.y; ++y) {
				for (size_t x = 0; x < rsize.x; ++x) {
					const glm::uvec3 p = start + glm::uvec3{x, y, z};
					const T& val = vt[(z * rsize.y + y) * rsize.x + x];
					data[(p.z * dims.y + p.y) * dims.x + p.x] = val;
					min = std::min(val, min);
					max = std::max(val, max);
				}
			}
		}
	}
	float sample(const glm::vec3 &p) const override {
		const int lo_z = p.z;
		const T lo_slice = bilinear_sample(glm::vec2(p), lo_z);
		const T hi_slice = bilinear_sample(glm::vec2(p), lo_z + 1);
		const float t = p.z - lo_z;
		return (1.0 - t) * static_cast<float>(lo_slice) + t * static_cast<float>(hi_slice);
	}
	float get_max() const override {
		return static_cast<float>(max);
	}
	float get_min() const override {
		return static_cast<float>(min);
	}

private:
	inline T get_voxel(int x, int y, int z) const {
		x = glm::clamp(x, 0, static_cast<int>(dims.x) - 1);
		y = glm::clamp(y, 0, static_cast<int>(dims.y) - 1);
		z = glm::clamp(z, 0, static_cast<int>(dims.z) - 1);
		return data[(z * dims.y + y) * dims.x + x];
	}
	inline T bilinear_sample(const glm::vec2 &coords, const int z) const {
		const glm::ivec2 origin = glm::ivec2(coords);
		const glm::vec2 local = glm::vec2{coords.x - origin.x, coords.y - origin.y};
		return get_voxel(origin.x, origin.y, z) * (1.0 - local.x) * (1.0 - local.y)
			+ get_voxel(origin.x + 1, origin.y, z) * local.x * (1.0 - local.y)
			+ get_voxel(origin.x, origin.y + 1, z) * (1.0 - local.x) * local.y
			+ get_voxel(origin.x + 1, origin.y + 1, z) * local.x * local.y;
	}
};

}

