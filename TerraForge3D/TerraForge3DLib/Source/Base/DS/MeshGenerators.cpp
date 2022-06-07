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

}