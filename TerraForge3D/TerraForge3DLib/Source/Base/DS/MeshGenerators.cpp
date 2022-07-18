#include "Base/DS/Mesh.hpp"
#include "Base/Math/Math.hpp"

#include <cmath>

#define PI 3.141f

namespace TerraForge3D
{

	Mesh& Mesh::Triangle(float* A, float* B, float* C, float* progress)
	{
		if (progress)
			*progress = 0.0f;
		Clear();
		Vertex v;
		v.position = glm::vec4(A[0], A[1], A[2], 0.0f);
		vertices.push_back(v);

		v.position = glm::vec4(B[0], B[1], B[2], 0.0f);
		vertices.push_back(v);

		v.position = glm::vec4(C[0], C[1], C[2], 0.0f);
		vertices.push_back(v);

		faces.push_back({0, 1, 2});

		if (progress)
			*progress = 1.1f;
		return *this;
	}

	Mesh& Mesh::Plane(glm::vec3 position, glm::vec3 right, glm::vec3 front, uint32_t resolution, float scale, float* progress)
	{
		TF3D_ASSERT(resolution >= 2,  "Plane resulution must be greater than equal to 2");
		if (progress)
			*progress = 0.0f;
		Clear();
		glm::vec3 up = glm::normalize(glm::cross(right, front));
		for (uint32_t y = 0; y < resolution; y++)
		{
			for (uint32_t x = 0; x < resolution; x++)
			{
				int i = x + y * resolution;
				glm::vec2 percent = glm::vec2(x, y) / ((float)resolution - 1);
				glm::vec3 pointOnPlane = (percent.x - .5f) * 2 * right + (percent.y - .5f) * 2 * front;
				pointOnPlane -= position; // origin shift
				pointOnPlane.x *= scale; // scale the terrain on X axis
				pointOnPlane.z *= scale; // scale the terrain on Y axis
				Vertex v;
				v.position = glm::vec4(0.0f);
				v.position.x = (float)pointOnPlane.x;
				v.position.y = (float)pointOnPlane.y;
				v.position.z = (float)pointOnPlane.z;
				v.texCoord = glm::vec4(percent.x, percent.y, 0.0f, 0.0f);
				v.normal = glm::vec4(up.x, up.y, up.z, 0.0f);
				vertices.push_back(v);

				if (x != resolution - 1 && y != resolution - 1)
				{
					Face f;
					f.a = i;
					f.b = i + resolution + 1;
					f.c = i + resolution;
					faces.push_back(f);
					f.a = i;
					f.b = i + 1;
					f.c = i + resolution + 1;
					faces.push_back(f);
				}

			}
			if (progress)
				*progress = (float)y / resolution;
		}
		if (progress)
			*progress = 1.1f;
		return *this;
	}


	Mesh& Mesh::Sphere(glm::vec3 position, float radius, uint32_t subdivisions, float* progress)
	{
		if (progress)
			*progress = 0.0f;
		Clear();
		vertices.emplace_back( 0.0f,  1.0f,  0.0f); // +Y 0
		vertices.emplace_back( 0.0f, -1.0f,  0.0f); // -Y 1
		vertices.emplace_back( 1.0f,  0.0f,  0.0f); // +X 2
		vertices.emplace_back(-1.0f,  0.0f,  0.0f); // -X 3
		vertices.emplace_back( 0.0f,  0.0f, -1.0f); // +Z 4
		vertices.emplace_back( 0.0f,  0.0f,  1.0f); // -Z 5
		

		faces.emplace_back(0, 2, 4); // +Y +X +Z
		faces.emplace_back(0, 4, 3); // +Y +Z -X
		faces.emplace_back(0, 3, 5); // +Y -X -Z
		faces.emplace_back(0, 5, 2); // +Y -Z +X
		faces.emplace_back(4, 1, 2); // +Z -Y +X
		faces.emplace_back(2, 1, 4); // +X -Y +Z
		faces.emplace_back(4, 1, 3); // +Z -Y -X
		faces.emplace_back(3, 1, 5); // -X -Y -Z

		if (progress)
			*progress = 0.3f;

		for (int i = 0; i < subdivisions; i++)
			CentroidSubdivision();

		if (progress)
			*progress = 0.6f;

		for (Vertex& v : vertices)
		{
			float invDist = FastInverseSqrt(v.position.x* v.position.x + v.position.y* v.position.y + v.position.z* v.position.z);
			v.position *= radius * invDist;
		}

		if (progress)
			*progress = 1.1f;

		return *this;
	}
}