#pragma once

#define GLM_FORCE_SIMD_AVX2
#include <glm/glm.hpp>

enum MeshType
{
	Plane = 0,
	Icosphere
};

struct Vert
{
	glm::vec4 position;
	glm::vec4 normal;
	glm::vec4 texCoord;
	glm::vec4 extras1;
};

class Mesh
{
public:
	Mesh();

	~Mesh();

	void RecalculateNormals();

	void GeneratePlane(int resolution, float scale, float textureScale = 1.0f);

	void GenerateScreenQuad(float dist = 0);

	void GenerateSphere(int resolution, float radius);

	void GenerateCube();

	void SetElevation(float elevation, int x, int y);

	float GetElevation(int x, int y);

	void AddElevation(float elevation, int x, int y);

	glm::vec2 GetTexCoord(float x, float y, float z);

	glm::vec3 GetNormals(int x, int y);

	void ClearNormals();

	Mesh *Clone();

	bool IsValid();

	Vert *vert;
	int *indices;
	int vertexCount;
	int indexCount;
	int res;
	float sc;
	float texSc;
	float maxHeight = -100;
	float minHeight = 100;
	bool deleteOnDestruction = true;
private:
	MeshType currType;
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
};
