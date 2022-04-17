#pragma once

#include "Base/Math/Core.hpp"
#include "Base/Math/Vector2.hpp"
#include "Base/Math/Vector3.hpp"

#include <string>
#include <cmath>

namespace TerraForge3D
{
	class Vector4
	{
	public:
		// Consttructooors

		Vector4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f) : x(x), y(y), z(z), w(w) {}

		Vector4(float* arr) : x(arr[0]), y(arr[1]), z(arr[2]), w(arr[3]) {}

		Vector4(const Vector4& v) :x(v.x), y(v.y), z(v.z), w(v.w) {}

		Vector4(Vector4* v) :x(v->x), y(v->y), z(v->z), w(v->w) {}

		Vector4(const Vector2& v) :x(v.x), y(v.y), z(0.0f), w(0.0f) {}

		Vector4(const Vector2& xy, const float& z) :x(xy.x), y(xy.y), z(z), w(0.0f) {}

		Vector4(const float& x, const Vector2& yz) :x(x), y(yz.x), z(yz.y), w(0.0f) {}

		Vector4(const Vector2& xy, const Vector2& zw) :x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}

		Vector4(const Vector3& xyz, const float& w = 0.0f) :x(xyz.x), y(xyz.y), z(xyz.x), w(w){}

		~Vector4() {}

		// Operators

		/*
		* Copy Assignent
		*/
		inline Vector4& operator=(const Vector4& other) { this->x = other.x, this->y = other.y; this->z = other.z; this->w = other.w; return *this; };

		inline Vector4& operator=(const Vector4* other) { this->x = other->x, this->y = other->y; this->z = other->z; this->w = other->w; return *this; };

		/*
		* Binary Operations
		*/
		inline Vector4& operator+=(const Vector4& rhs) { this->x += rhs.x; this->y += rhs.y; this->z += rhs.z; this->w += rhs.w; return *this;}
		inline Vector4& operator-=(const Vector4& rhs) { this->x -= rhs.x; this->y -= rhs.y; this->z -= rhs.z; this->w -= rhs.w; return *this;}
		inline Vector4& operator*=(const Vector4& rhs) { this->x *= rhs.x; this->y *= rhs.y; this->z *= rhs.z; this->w *= rhs.w; return *this;}
		inline Vector4& operator/=(const Vector4& rhs) { this->x /= rhs.x; this->y /= rhs.y; this->z /= rhs.z; this->w /= rhs.w; return *this;}

		inline Vector4& operator+=(const float& rhs) { this->x += rhs; this->y += rhs; this->z += rhs; this->w += rhs;  return *this;}
		inline Vector4& operator-=(const float& rhs) { this->x -= rhs; this->y -= rhs; this->z -= rhs; this->w -= rhs;  return *this;}
		inline Vector4& operator*=(const float& rhs) { this->x *= rhs; this->y *= rhs; this->z *= rhs; this->w *= rhs;  return *this;}
		inline Vector4& operator/=(const float& rhs) { this->x /= rhs; this->y /= rhs; this->z /= rhs; this->w /= rhs;  return *this;}

		inline float& operator[](int n) {
			switch (n)
			{
			case 0: return this->x;
			case 1: return this->y;
			case 2: return this->z;
			case 3: return this->w;
			default:return this->x;
			}
		}

		/*
		* General Vector Functions
		*/

		inline float Length() const { return sqrt(x * x + y * y + z * z + w * w); }

		inline float LengthSquared() const { return x * x + y * y + z * z + w * w; }

		inline Vector4& Normalize()
		{
			float lenghtInverse = FastInverseSqrt(x * x + y * y + z * z + w * w);
			x = x * lenghtInverse;
			y = y * lenghtInverse;
			z = z * lenghtInverse;
			w = w * lenghtInverse;
			return *this;
		}

		inline float* ValuePtr() { return reinterpret_cast<float*>(this); }

#ifndef TF3D_MATH_DISABLE_SWIZZLE
		// Swizzle Functions
		inline Vector2 XY()  const{ return Vector2(x, y); };
		inline Vector2 YX()  const{ return Vector2(y, x); };
		inline Vector2 XZ()  const{ return Vector2(x, z); };
		inline Vector2 ZX()  const{ return Vector2(z, x); };
		inline Vector2 YZ()  const{ return Vector2(y, z); };
		inline Vector2 ZY()  const{ return Vector2(z, y); };
		inline Vector3 XYZ() const { return Vector3(x, y, z); };
		inline Vector3 XZY() const { return Vector3(x, z, y); };
		inline Vector3 YXZ() const { return Vector3(y, x, z); };
		inline Vector3 YZX() const { return Vector3(y, z, x); };
		inline Vector3 ZXY() const { return Vector3(z, x, y); };
		inline Vector3 ZYX() const { return Vector3(z, y, x); };

		// NOTE: Swizzle functions are not available for Vector4 W
#endif

		inline std::string ToString() const { return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) + ")"; }

		// Static Functions
		inline static float Dot(const Vector4& a, const Vector4& b)
		{
			return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
		}

		/*
		* Cross product of the xyz of the to Vector4
		*/
		inline static Vector4 Cross(const Vector4& a, const Vector4& b)
		{
			return Vector4(Vector3::Cross(a.XYZ(), b.XYZ()));
		}

	public:
		float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
	};

	inline Vector4 operator+(const Vector4& a, const Vector4& b) { return Vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
	inline Vector4 operator-(const Vector4& a, const Vector4& b) { return Vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
	inline Vector4 operator*(const Vector4& a, const Vector4& b) { return Vector4(a.x * b.x, a.y * b.y, a.z * b.z); }
	inline Vector4 operator/(const Vector4& a, const Vector4& b) { return Vector4(a.x / b.x, a.y / b.y, a.z / b.z); }

	inline Vector4 operator+(const Vector4& a, const float& b) { return Vector4(a.x + b, a.y + b, a.z + b); }
	inline Vector4 operator-(const Vector4& a, const float& b) { return Vector4(a.x - b, a.y - b, a.z - b); }
	inline Vector4 operator*(const Vector4& a, const float& b) { return Vector4(a.x * b, a.y * b, a.z * b); }
	inline Vector4 operator/(const Vector4& a, const float& b) { return Vector4(a.x / b, a.y / b, a.z / b); }
}

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

template<typename OStream>
inline OStream& operator<<(OStream& os, const TerraForge3D::Vector4& v)
{
	return os << v.ToString();
}

