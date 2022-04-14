#pragma once

#include "Base/Math/Core.hpp"
#include "Base/Math/Vector2.hpp"

#include <string>
#include <cmath>

namespace TerraForge3D
{
	class Vector3
	{
	public:
		// Consttructooors

		Vector3() :x(0.0f), y(0.0f), z(0.0f) {}

		Vector3(float x) :x(x), y(x), z(x) {}

		Vector3(float x, float y) : x(x), y(y), z(0.0f) {}

		Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

		Vector3(float* arr) : x(arr[0]), y(arr[1]), z(arr[2]) {}

		Vector3(const Vector3& v) :x(v.x), y(v.y), z(v.z) {}

		Vector3(Vector3* v) :x(v->x), y(v->y), z(v->z) {}

		Vector3(const Vector2& v) :x(v.x), y(v.y), z(0.0f) {}

		Vector3(const Vector2& xy, const float& z) :x(xy.x), y(xy.y), z(z) {}

		Vector3(const float& x, const Vector2& yz) :x(x), y(yz.x), z(yz.y) {}



		~Vector3() {}

		// Operators

		/*
		* Copy Assignent
		*/
		inline Vector3& operator=(const Vector3& other) { this->x = other.x, this->y = other.y; this->z = other.z; };

		inline Vector3& operator=(const Vector3* other) { this->x = other->x, this->y = other->y; this->z = other->z; };

		/*
		* Binary Operations
		*/
		inline Vector3& operator+=(const Vector3& rhs) { this->x += rhs.x; this->y += rhs.y; this->z += rhs.z; return *this;  }
		inline Vector3& operator-=(const Vector3& rhs) { this->x -= rhs.x; this->y -= rhs.y; this->z -= rhs.z; return *this; }
		inline Vector3& operator*=(const Vector3& rhs) { this->x *= rhs.x; this->y *= rhs.y; this->z *= rhs.z; return *this; }
		inline Vector3& operator/=(const Vector3& rhs) { this->x /= rhs.x; this->y /= rhs.y; this->z /= rhs.z; return *this; }

		inline Vector3& operator+=(const float& rhs) { this->x += rhs; this->y += rhs; this->z += rhs; return *this; }
		inline Vector3& operator-=(const float& rhs) { this->x -= rhs; this->y -= rhs; this->z -= rhs; return *this; }
		inline Vector3& operator*=(const float& rhs) { this->x *= rhs; this->y *= rhs; this->z *= rhs; return *this; }
		inline Vector3& operator/=(const float& rhs) { this->x /= rhs; this->y /= rhs; this->z /= rhs; return *this; }

		inline float& operator[](int n) {
			switch (n)
			{
			case 0: return this->x;
			case 1: return this->y;
			case 2: return this->z;
			default:return this->x;
			}
		}

		/*
		* General Vector Functions
		*/

		inline float Length() const { return sqrt(x * x + y * y + z * z); }

		inline float LengthSquared() const { return x * x + y * y + z * z; }

		inline Vector3& Normalize()
		{
			float lenghtInverse = FastInverseSqrt(x * x + y * y + z * z);
			x = x * lenghtInverse;
			y = y * lenghtInverse;
			z = z * lenghtInverse;
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
#endif

		inline std::string ToString() const { return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")"; }

		// Static Functions
		inline static float Dot(const Vector3& a, const Vector3& b)
		{
			return a.x * b.x + a.y * b.y + a.z * b.z;
		}


		/*
		* Cross product of 2 3D Vectors
		*/
		inline static Vector3 Cross(const Vector3& a, const Vector3& b)
		{
			return Vector3(
				a.y * b.z - b.y * a.z,
				a.x * b.z - b.x * a.z,
				a.x * b.y - b.x * a.y);			
		}

	public:
		float x = 0.0f, y = 0.0f, z = 0.0f;
	};

	inline Vector3 operator+(const Vector3& a, const Vector3& b) { return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);}
	inline Vector3 operator-(const Vector3& a, const Vector3& b) { return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);}
	inline Vector3 operator*(const Vector3& a, const Vector3& b) { return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);}
	inline Vector3 operator/(const Vector3& a, const Vector3& b) { return Vector3(a.x / b.x, a.y / b.y, a.z / b.z);}

	inline Vector3 operator+(const Vector3& a, const float& b) { return Vector3(a.x + b, a.y + b, a.z + b); }
	inline Vector3 operator-(const Vector3& a, const float& b) { return Vector3(a.x - b, a.y - b, a.z - b); }
	inline Vector3 operator*(const Vector3& a, const float& b) { return Vector3(a.x * b, a.y * b, a.z * b); }
	inline Vector3 operator/(const Vector3& a, const float& b) { return Vector3(a.x / b, a.y / b, a.z / b); }
}

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

template<typename OStream>
inline OStream& operator<<(OStream& os, const TerraForge3D::Vector3& v)
{
	return os << v.ToString();
}

