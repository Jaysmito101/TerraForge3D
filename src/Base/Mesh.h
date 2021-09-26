#pragma once



struct Vert 
{
	glm::vec3 position;
	glm::vec3 normal;
};

class Mesh
 {
public:
	Mesh();

	void RecalculateNormals();

	void GeneratePlane(int resolution, float scale);

	void SetElevation(float elevation, int x, int y);

	void AddElevation(float elevation, int x, int y);

	Vert* vert;
	int* indices;
	int vertexCount;
	int indexCount;

private:
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
};
