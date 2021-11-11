#pragma once

#include <Mesh.h>
#include <Texture2D.h>

#include <string>

class Model
{
public:
	Model(std::string name);
	~Model();

	void Update();
	void SetupMeshOnGPU();
	void UploadToGPU();
	void Render();

	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(1.0f);
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	Mesh* mesh;
	std::string name;
	uint32_t vao;
	uint32_t vbo;
	uint32_t ebo;
	std::string path = "";
	Texture2D* diffuse;
};