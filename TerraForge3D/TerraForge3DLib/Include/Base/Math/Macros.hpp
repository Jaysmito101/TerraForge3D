#pragma once
#include "Base/Math/Core.hpp"

#include <cmath>

// General math functions

// Temporary (TODO: Replace with something faster)
#define TF3D_FAST_SQRT(x) sqrt(x) 

// Vector3 Macros

#define TF3D_VEC3_ADD(out, a, b) \
		out.x = a.x + b.x; \
		out.y = a.y + b.y; \
		out.y = a.z + b.z;

#define TF3D_VEC3_SUB(out, a, b) \
		out.x = a.x - b.x; \
		out.y = a.y - b.y; \
		out.y = a.z - b.z;

#define TF3D_VEC3_MUL(out, a, b) \
		out.x = a.x * b.x; \
		out.y = a.y * b.y; \
		out.y = a.z * b.z;

#define TF3D_VEC3_DOT(out, a, b) \
		out = a.x * b.x + a.y * b.y + a.z * b.z; 

#define TF3D_VEC3_CROSS(out, a, b) \
		out.x = a.y * b.z - b.y * a.z; \
		out.y = a.x * b.z - b.x * a.z; \
		out.z = a.x * b.y - b.x * a.y;

#define TF3D_VEC3_LENGTH(out, a) \
		out = TF3D_FAST_SQRT(a.x * a.x + a.y * a.y + a.z * a.z);

#define TF3D_VEC3_NORMALIZE(a) \
		float inverseLength##__LINE__ = 1 / TF3D_VEC3_LENGTH(a); \
		a.x = a.x / inverseLength##__LINE__; \
		a.y = a.y / inverseLength##__LINE__; \
		a.z = a.z / inverseLength##__LINE__; 