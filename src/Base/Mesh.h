#pragma once

#include <glm/glm.hpp>


struct Vert 
{
	glm::vec4 position;
	glm::vec4 normal;
	glm::vec2 texCoord;
};

class Mesh
 {
public:
	Mesh();

	~Mesh();

	void RecalculateNormals();

	void GeneratePlane(int resolution, float scale, float textureScale = 1.0f);

	void SetElevation(float elevation, int x, int y);

	float GetElevation(int x, int y);

	void AddElevation(float elevation, int x, int y);

	glm::vec3 Mesh::GetNormals(int x, int y);

	Mesh* Clone();

	bool IsValid();

	Vert* vert;
	int* indices;
	int vertexCount;
	int indexCount;
	int res;
	float sc;
	float maxHeight = -100;
	float minHeight = 100;
	bool deleteOnDestruction = true;
private:
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
};
