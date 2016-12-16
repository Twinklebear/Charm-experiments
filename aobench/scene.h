#pragma once

#include <array>
#include <cmath>

// Object defining the AO Bench scene structure

struct vec3f {
	float x, y, z;

	inline vec3f(float s = 0) : x(s), y(s), z(s) {}
	inline vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
};

inline vec3f operator+(const vec3f &a, const vec3f &b) {
	return vec3f{a.x + b.x, a.y + b.y, a.z + b.z};
}
inline vec3f operator-(const vec3f &a, const vec3f &b) {
	return vec3f{a.x - b.x, a.y - b.y, a.z - b.z};
}
inline vec3f operator*(const vec3f &a, const vec3f &b) {
	return vec3f{a.x * b.x, a.y * b.y, a.z * b.z};
}
inline vec3f operator/(const vec3f &a, const vec3f &b) {
	return vec3f{a.x / b.x, a.y / b.y, a.z / b.z};
}
inline vec3f operator*(const vec3f &a, const float s) {
	return vec3f{a.x * s, a.y * s, a.z * s};
}
inline vec3f operator*(const float s, const vec3f &a) {
	return vec3f{a.x * s, a.y * s, a.z * s};
}
inline vec3f operator/(const vec3f &a, const float s) {
	const float inv_s = 1.0 / s;
	return a * inv_s;
}

inline float dot(const vec3f &a, const vec3f &b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline vec3f normalized(const vec3f &v) {
	const float inv_len = 1.0 / std::sqrt(dot(v, v));
	return v * inv_len;
}
inline vec3f cross(const vec3f &a, const vec3f &b) {
	vec3f c{
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
	return c;
}

void ortho_basis(std::array<vec3f, 3> &basis, const vec3f &n);

struct Ray {
	vec3f origin, dir;
};

struct Isect {
	float t;
	vec3f p, n;
	int hit;
};

struct Sphere {
	vec3f center;
	float radius;

	Sphere(const vec3f &center, float radius);
	void intersect(Isect &isect, const Ray &ray) const;
};

struct Plane {
	vec3f p, n;

	Plane(const vec3f &p, const vec3f &n);
	void intersect(Isect &isect, const Ray &ray) const;
};

