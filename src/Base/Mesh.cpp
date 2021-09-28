#include <Mesh.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>

Mesh::Mesh()
{
	
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
	for (int i = 0; i < indexCount; i += 3)
	{
		const int ia = indices[i];
		const int ib = indices[i + 1];
		const int ic = indices[i + 2];

		const glm::vec3 e1 = vert[ia].position - vert[ib].position;
		const glm::vec3 e2 = vert[ic].position - vert[ib].position;
		const glm::vec3 no = cross(e1, e2);

		vert[ia].normal += no;
		vert[ib].normal += no;
		vert[ic].normal += no;
	}

	for (int i = 0; i < vertexCount; i++) vert[i].normal = glm::normalize(vert[i].normal);
}

void Mesh::GeneratePlane(int resolution, float scale)
{
	res = resolution;
	sc = scale;
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
			vertices[i].position = glm::vec3(0.0f);

			vertices[i].position.x = (float)pointOnPlane.x;
			vertices[i].position.y = (float)pointOnPlane.y;
			vertices[i].position.z = (float)pointOnPlane.z;
			vertices[i].normal = glm::vec3(0.0f);
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
	vert[i].position.y = elevation;
}

void Mesh::AddElevation(float elevation, int x, int y){
	if(!vert)
		return;
	int i = x + y * res;
	if (i > vertexCount)
		return;
	vert[i].position.y += elevation;
}

Mesh* Mesh::Clone()
{
	Mesh* cloneMesh = new Mesh();
	cloneMesh->res = res;
	cloneMesh->sc = sc;
	cloneMesh->vertexCount = vertexCount;
	cloneMesh->indexCount = indexCount;
	cloneMesh->vert = new Vert[res * res];
	memcpy(cloneMesh->vert, vert, sizeof(Vert) * vertexCount);
	cloneMesh->indices = new int[(res - 1) * (res - 1) * 6];
	memcpy(cloneMesh->indices, indices, sizeof(int) * indexCount);
	return cloneMesh;
}

bool Mesh::IsValid() {
	if (vert && indices)
		return true;
	return false;
}