#include <Model.h>


#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>

#include <glad/glad.h>

Model::Model(std::string n)
{
	name = n;
	mesh = new Mesh();
}

void Model::SetupMeshOnGPU() {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * mesh.vertexCount, mesh.vert, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * mesh.indexCount, mesh.indices, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, texCoord));
	glEnableVertexAttribArray(2);
}

void Model::UploadToGPU()
{
	if (!mesh)
		return;
	if (!mesh->IsValid())
		return;

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * mesh->vertexCount, mesh->vert, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * mesh->indexCount, mesh->indices, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, texCoord));
	glEnableVertexAttribArray(2);
}

void Model::Update()
{
	modelMatrix = glm::translate(glm::mat4(1.0f), position);
	modelMatrix = glm::rotate(modelMatrix, rotation.x, glm::vec3(1, 0, 0));
	modelMatrix = glm::rotate(modelMatrix, rotation.y, glm::vec3(0, 1, 0));
	modelMatrix = glm::rotate(modelMatrix, rotation.z, glm::vec3(0, 0, 1));
	modelMatrix = glm::scale(modelMatrix, scale);
}

void Model::Render()
{
	if (!mesh)
		return;
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}


Model::~Model() {
	if (mesh)
		delete mesh;
}