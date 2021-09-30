#pragma once

#include <Mesh.h>

#include <string>

class Model
{
public:
	Model(std::string name);

	void Update();
	void SetupMeshOnGPU();
	void UploadToGPU();
	void Render();

	glm::vec3 position = glm::vec3(0.0f);
	glm::mat4 modelMatrix = glm::mat4(0.0f);
	Mesh mesh;
	std::string name;
	uint32_t vao;
	uint32_t vbo;
	uint32_t ebo;
};