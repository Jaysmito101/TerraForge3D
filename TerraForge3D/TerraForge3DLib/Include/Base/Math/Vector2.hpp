#pragma once

#include "Base/Math/Core.hpp"

#include <string>
#include <cmath>

namespace TerraForge3D
{
	class Vector2
	{
	public:
		// Consttructooors

		Vector2() :x(0.0f), y(0.0f) {}

		Vector2(float x) :x(x), y(x) {}

		Vector2(float x, float y) : x(x), y(y) {}

		Vector2(float* arr) : x(arr[0]), y(arr[1]) {}

		Vector2(const Vector2& v) :x(v.x), y(v.y) {}

		Vector2(Vector2* v) :x(v->x), y(v->y) {}

		~Vector2() {}

		// Operators

		/*
		* Copy Assignent
		*/
		inline Vector2& operator=(const Vector2& other) { this->x = other.x, this->y = other.y; };
		
		inline Vector2& operator=(const Vector2* other) { this->x = other->x, this->y = other->y; };

		/*
		* Binary Operations
		*/
		inline Vector2& operator+=(const Vector2& rhs) { this->x += rhs.x; this->y += rhs.y; return *this; }
		inline Vector2& operator-=(const Vector2& rhs) { this->x -= rhs.x; this->y -= rhs.y; return *this; }
		inline Vector2& operator*=(const Vector2& rhs) { this->x *= rhs.x; this->y *= rhs.y; return *this; }
		inline Vector2& operator/=(const Vector2& rhs) { this->x /= rhs.x; this->y /= rhs.y; return *this; }

		inline Vector2& operator+=(const float& rhs) { this->x += rhs; this->y += rhs;  return *this; }
		inline Vector2& operator-=(const float& rhs) { this->x -= rhs; this->y -= rhs;  return *this; }
		inline Vector2& operator*=(const float& rhs) { this->x *= rhs; this->y *= rhs;  return *this; }
		inline Vector2& operator/=(const float& rhs) { this->x /= rhs; this->y /= rhs;  return *this; }

		inline float& operator[](int n) {
			switch (n)
			{
			case 0: return this->x;
			case 1: return this->y;
			default:return this->x;
			}
		}

		/*
		* General Vector Functions
		*/

		inline float Length() const {	return sqrt( x * x + y * y);}

		inline float LengthSquared() const { return x * x + y * y; }

		inline Vector2& Normalize() 
		{
			float lenghtInverse = FastInverseSqrt(x * x + y * y);
			x = x * lenghtInverse;
			y = y * lenghtInverse;
			return *this;
		}

		inline float* ValuePtr() { return reinterpret_cast<float*>(this); }

		inline std::string ToString()const { return "(" + std::to_string(x) + ", " + std::to_string(y) + ")"; }

#ifndef TF3D_MATH_DISABLE_SWIZZLE
		// Swizzle Functions
		inline Vector2 XY() const { return Vector2(x, y); };
		inline Vector2 YX() const { return Vector2(y, x); };
#endif


		// Static Functions
		inline static float Dot(const Vector2& a, const Vector2& b)
		{
			return a.x * b.x + a.y * b.y;			
		}


		/*
		* Cross product of 2 2D Vectors
		* 
		* NOTE: It returns a float instead of a Vector type as the result is just the z component of the resultant Vector3
		*/
		inline static float Cross(const Vector2& a, const Vector2& b)
		{
			return a.x * b.y - a.y * b.x;
		}

	public:
		float x = 0.0f, y = 0.0f;
	};

	inline Vector2 operator+(const Vector2& a, const Vector2& b) { return Vector2(a.x + b.x, a.y + b.y); }
	inline Vector2 operator-(const Vector2& a, const Vector2& b) { return Vector2(a.x - b.x, a.y - b.y); }
	inline Vector2 operator*(const Vector2& a, const Vector2& b) { return Vector2(a.x * b.x, a.y * b.y); }
	inline Vector2 operator/(const Vector2& a, const Vector2& b) { return Vector2(a.x / b.x, a.y / b.y); }

	inline Vector2 operator+(const Vector2& a, const float& b) { return Vector2(a.x + b, a.y + b); }
	inline Vector2 operator-(const Vector2& a, const float& b) { return Vector2(a.x - b, a.y - b); }
	inline Vector2 operator*(const Vector2& a, const float& b) { return Vector2(a.x * b, a.y * b); }
	inline Vector2 operator/(const Vector2& a, const float& b) { return Vector2(a.x / b, a.y / b); }
}

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

template<typename OStream>
inline OStream& operator<<(OStream& os, const TerraForge3D::Vector2& v)
{
	return os << v.ToString();
}

