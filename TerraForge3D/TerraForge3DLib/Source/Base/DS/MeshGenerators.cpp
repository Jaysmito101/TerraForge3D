#include "Base/DS/Mesh.hpp"

namespace TerraForge3D
{

	Mesh& Mesh::Triangle(float* A, float* B, float* C)
	{
		Clear();
		Vertex v;
		v.position = glm::vec4(A[0], A[1], A[2], 0.0f);
		vertices.push_back(v);

		v.position = glm::vec4(B[0], B[1], B[2], 0.0f);
		vertices.push_back(v);

		v.position = glm::vec4(C[0], C[1], C[2], 0.0f);
		vertices.push_back(v);

		faces.push_back({0, 1, 2});

		return *this;
	}

	Mesh& Mesh::Plane(glm::vec3 position, glm::vec3 right, glm::vec3 front, uint32_t resolution)
	{
		TF3D_ASSERT(resolution >= 2,  "Plane resulution must be greater than equal to 2");
		Clear();
		glm::vec3 up = glm::cross(front, right);
		for (uint32_t y = 0; y < resolution; y++)
		{
			for (uint32_t x = 0; x < resolution; x++)
			{
				int i = x + y * resolution;
				glm::vec2 percent = glm::vec2(x, y) / ((float)resolution - 1);
				glm::vec3 pointOnPlane = (percent.x - .5f) * 2 * right + (percent.y - .5f) * 2 * front;
				pointOnPlane -= position; // origin shift
				// pointOnPlane *= scale;
				Vertex v;
				v.position = glm::vec4(0.0f);
				v.position.x = (float)pointOnPlane.x;
				v.position.y = (float)pointOnPlane.y;
				v.position.z = (float)pointOnPlane.z;
				v.texCoord = glm::vec4(percent.x, percent.y, 0.0f, 0.0f);
				v.normal = glm::vec4(0.0f);
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
		}
		return *this;
	}
}