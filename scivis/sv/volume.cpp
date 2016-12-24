#include <fstream>
#include "volume.h"

namespace sv {

Volume::Volume(const glm::uvec3 &offset, const glm::uvec3 &dims) : offset(offset), dims(dims) {}
Volume::~Volume() {}
const glm::uvec3& Volume::get_offset() const {
	return offset;
}
const glm::uvec3& Volume::get_dims() const {
	return dims;
}

static size_t dtype_size(const VolumeDType &dtype) {
	switch (dtype) {
		case VolumeDType::UINT8: return 1;
		case VolumeDType::UINT16: return 2;
		case VolumeDType::INT32: return 4;
		case VolumeDType::FLOAT: return 4;
		case VolumeDType::DOUBLE: return 8;
	}
	throw std::runtime_error("Invalid VolumeDType");
}
std::shared_ptr<Volume> load_raw_volume(const std::string &fname, const glm::uvec3 &dims,
		const VolumeDType dtype, const glm::uvec3 &offset, const glm::uvec3 &subregion_dims)
{
	std::ifstream fin{fname.c_str(), std::ios::binary};
	const glm::uvec3 load_dims = subregion_dims == glm::uvec3{0} ? dims : subregion_dims;
	std::vector<uint8_t> vol_data(load_dims.x * load_dims.y * load_dims.z * dtype_size(dtype), 0);

	switch (dtype) {
		case VolumeDType::UINT8:
			return std::make_shared<VolumeData<uint8_t>>(offset, load_dims,
					static_cast<const void*>(vol_data.data()));
		case VolumeDType::UINT16:
			return std::make_shared<VolumeData<uint16_t>>(offset, load_dims,
					static_cast<const void*>(vol_data.data()));
		case VolumeDType::INT32:
			return std::make_shared<VolumeData<int>>(offset, load_dims,
					static_cast<const void*>(vol_data.data()));
		case VolumeDType::FLOAT:
			return std::make_shared<VolumeData<float>>(offset, load_dims,
					static_cast<const void*>(vol_data.data()));
		case VolumeDType::DOUBLE:
			return std::make_shared<VolumeData<double>>(offset, load_dims,
					static_cast<const void*>(vol_data.data()));
	}
	throw std::runtime_error("Invalid VolumeDType");
}

}

