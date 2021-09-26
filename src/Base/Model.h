#pragma once

#include <Mesh.h>

#include <string>

class Model
{
public:
	Model(std::string name);

	void Update();

	glm::vec3 position = glm::vec3(0.0f);
	glm::mat4 modelMatrix = glm::mat4(0.0f);
	Mesh mesh;
	std::string name;
};