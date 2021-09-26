#pragma once

#include <glm/glm.hpp>


struct Vert 
{
	glm::vec3 position;
	glm::vec3 normal;
};

class Mesh
 {
public:
	Mesh();

	~Mesh();

	void RecalculateNormals();

	void GeneratePlane(int resolution, float scale);

	void SetElevation(float elevation, int x, int y);

	void AddElevation(float elevation, int x, int y);

	bool IsValid();

	Vert* vert;
	int* indices;
	int vertexCount;
	int indexCount;
	int res;
	float sc;

private:
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
};
