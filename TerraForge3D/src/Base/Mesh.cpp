#include <Mesh.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>

Mesh::Mesh()
{
	vert = nullptr;
	indices = nullptr;
}

Mesh::~Mesh() {
	if (deleteOnDestruction) {
		if (vert)
			delete vert;
		if (indices)
			delete indices;
	}
}

void Mesh::RecalculateNormals()
{
	if (currType == MeshType::Icosphere) 
	{
		for (int i = 0; i < vertexCount; i++) {
			vert[i].normal = glm::normalize(vert[i].position);
		}
	}
	else {
		for (int i = 0; i < indexCount; i += 3)
		{
			const int ia = indices[i];
			const int ib = indices[i + 1];
			const int ic = indices[i + 2];

			const glm::vec3 e1 = glm::vec3(vert[ia].position) - glm::vec3(vert[ib].position);
			const glm::vec3 e2 = glm::vec3(vert[ic].position) - glm::vec3(vert[ib].position);
			const glm::vec3 no = cross(e1, e2);

			vert[ia].normal += glm::vec4(no, 0.0);
			vert[ib].normal += glm::vec4(no, 0.0);
			vert[ic].normal += glm::vec4(no, 0.0);
		}

		for (int i = 0; i < vertexCount; i++) vert[i].normal = glm::normalize(vert[i].normal);
	}
}

void Mesh::GenerateIcoSphere(int resolution, float radius, float textureScale)
{
	currType = MeshType::Icosphere;
	glm::vec3 vertis[] = {
		glm::vec3(0, -1, 0),
		glm::vec3(0, 0, 1),
		glm::vec3(1, 0, 0),
		glm::vec3(0, 0, -1),
		glm::vec3(-1, 0, 0),
		glm::vec3(0, 1, 0)
	};

	int triangles[] = {
		   0, 1, 2,
		   0, 2, 3,
		   0, 3, 4,
		   0, 4, 1,

		   5, 2, 1,
		   5, 3, 2,
		   5, 4, 3,
		   5, 1, 4
	};

	for (int i = 0; i < sizeof(vertis)/sizeof(glm::vec3); i++) {
		vertis[i] *= radius;
	}

	vertexCount = sizeof(vertis)/sizeof(int);
	vert = new Vert[sizeof(vertis) / sizeof(int)];
	memset(vert, 0, sizeof(Vert) * vertexCount);
 	for (int i = 0; i < vertexCount; i++) {
		vert[i] = Vert();
		vert[i].position = glm::vec4(vertis[i], 1);
	}
	indexCount = sizeof(triangles)/sizeof(int);
	indices = new int[indexCount];
	memcpy(indices, triangles, sizeof(int) * indexCount);
}

void Mesh::GeneratePlane(int resolution, float scale, float textureScale)
{
	currType = MeshType::Plane;
	maxHeight = -100;
	minHeight = 100;
	res = resolution;
	sc = scale;
	texSc = textureScale;
	Vert* vertices = new Vert[resolution * resolution];
	int* inds = new int[(resolution-1) * (resolution-1) * 6];
	int triIndex = 0;

	for (int y = 0; y < resolution; y++)
	{ 
		for (int x = 0; x < resolution; x++)
		{
			int i = x + y * resolution;
			glm::vec2 percent = glm::vec2(x, y) / ((float)resolution - 1);
			glm::vec3 pointOnPlane = (percent.x - .5f) * 2 * right + (percent.y - .5f) * 2 * front;
			pointOnPlane *= scale;
			vertices[i] = Vert();
			vertices[i].position = glm::vec4(0.0f);

			vertices[i].position.x = (float)pointOnPlane.x;
			vertices[i].position.y = (float)pointOnPlane.y;
			vertices[i].position.z = (float)pointOnPlane.z;
			vertices[i].texCoord = glm::vec2(percent.x, percent.y)*textureScale;
			vertices[i].normal = glm::vec4(0.0f);
			if (x != resolution - 1 && y != resolution - 1)
			{
				inds[triIndex] = i;
				inds[triIndex + 1] = i + resolution + 1;
				inds[triIndex + 2] = i + resolution;

				inds[triIndex + 3] = i;
				inds[triIndex + 4] = i + 1;
				inds[triIndex + 5] = i + resolution + 1;
				triIndex += 6;
			}
		}
	}
	
	if (vert)
		delete vert;
	if (indices)
		delete indices;
	vertexCount = resolution * resolution;
	vert = new Vert[resolution * resolution];
	memset(vert, 0, sizeof(Vert) * vertexCount);
	memcpy(vert, vertices, sizeof(Vert)*vertexCount);
	indexCount = (resolution - 1) * (resolution - 1) * 6;
	indices = new int[(resolution - 1) * (resolution - 1) * 6];
	memset(indices, 0, indexCount);
	memcpy(indices, inds, sizeof(int) * indexCount);
	delete vertices;
	delete inds;
}

void Mesh::SetElevation(float elevation, int x, int y){
	if(!vert)
		return;
	int i = x + y * res;
	if (i > vertexCount)
		return;
	if (elevation > maxHeight)
		maxHeight = elevation;
	if (elevation < minHeight)
		minHeight = elevation;
	vert[i].position.y = elevation;
}

float Mesh::GetElevation(int x, int y) {
	if (!vert)
		return 0;
	int i = x + y * res;
	if (i > vertexCount)
		return 0;
	
	return vert[i].position.y;
}

glm::vec3 Mesh::GetNormals(int x, int y) {
	if (!vert)
		return glm::vec3(0);
	int i = x + y * res;
	if (i > vertexCount)
		return glm::vec3(0);
	return glm::vec3(vert[i].normal.x, vert[i].normal.y, vert[i].normal.z);
}

void Mesh::AddElevation(float elevation, int x, int y){
	if(!vert)
		return;
	int i = x + y * res;
	if (i > vertexCount)
		return;
	vert[i].position.y += elevation;
}

glm::vec2 Mesh::GetTexCoord(float x, float y, float z)
{
	if (currType == MeshType::Plane)
	{
		return (glm::vec2(x, y) / ((float)res - 1)) * texSc;
	}
	else {
		return glm::vec2(1.0f);
	}
}

Mesh* Mesh::Clone()
{
	Mesh* cloneMesh = new Mesh();
	cloneMesh->res = res;
	cloneMesh->sc = sc;
	cloneMesh->maxHeight = maxHeight;
	cloneMesh->vertexCount = vertexCount;
	cloneMesh->indexCount = indexCount;
	cloneMesh->vert = new Vert[vertexCount];
	memcpy(cloneMesh->vert, vert, sizeof(Vert) * vertexCount);
	cloneMesh->indices = new int[indexCount];
	memcpy(cloneMesh->indices, indices, sizeof(int) * indexCount);
	return cloneMesh;
}

bool Mesh::IsValid() {
	if (vert && indices)
		return true;
	return false;
}