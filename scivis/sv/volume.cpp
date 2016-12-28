#include <fstream>
#include <string>
#include <iostream>
#include "volume.h"

namespace sv {

Volume::Volume(const glm::uvec3 &offset, const glm::uvec3 &dims)
	: offset(offset), dims(dims), bounds(glm::vec3(offset), glm::vec3(offset + dims))
{}
Volume::~Volume() {}
const glm::uvec3& Volume::get_offset() const {
	return offset;
}
const glm::uvec3& Volume::get_dims() const {
	return dims;
}
const BBox& Volume::get_bounds() const {
	return bounds;
}

VolumeDType parse_volume_dtype(const std::string &s) {
	if (s == "uint8") {
		return VolumeDType::UINT8;
	} else if (s == "uint16") {
		return VolumeDType::UINT16;
	} else if (s == "int32") {
		return VolumeDType::INT32;
	} else if (s == "float") {
		return VolumeDType::FLOAT;
	} else if (s == "double") {
		return VolumeDType::DOUBLE;
	} else {
		throw std::runtime_error("Invalid VolumeDType: '" + s + "'");
	}
}
std::istream& operator>>(std::istream &is, VolumeDType &dtype) {
	std::string s;
	is >> s;
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	dtype = parse_volume_dtype(s);
	return is;
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
	const glm::uvec3 load_dims = subregion_dims == glm::uvec3(0) ? dims : subregion_dims;
	std::vector<uint8_t> vol_data(load_dims.x * load_dims.y * load_dims.z * dtype_size(dtype), 0);

	// Go through and read scanlines of the volume until we read the subregion we're loading
	for (size_t z = offset.z; z < offset.z + load_dims.z; ++z) {
		for (size_t y = offset.y; y < offset.y + load_dims.y; ++y) {
			fin.seekg((z * dims.y + y) * dims.x + offset.x);
			const size_t i = (z * load_dims.y + y) * load_dims.x * dtype_size(dtype);
			fin.read(reinterpret_cast<char*>(&vol_data[i]), (load_dims.x - offset.x) * dtype_size(dtype));
		}
	}

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

