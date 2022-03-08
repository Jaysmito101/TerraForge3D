#pragma once

#include <glm/glm.hpp>

#include <json.hpp>


class Camera
{
public:
	glm::mat4 view;
	glm::mat4 pers;
	glm::mat4 pv;
	glm::vec3 mposition;
	glm::vec3 mrotation;
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	float pitch;
	float yaw;
	float roll;

	float fov;
	float cFar;
	float cNear;
	float aspect;

	int camID; // May be removed in future

	float position[3];
	float rotation[3];

	Camera();

	nlohmann::json  Save();

	void Load(nlohmann::json data);

	void ShowSettings(bool renderWindow = false, bool *pOpen = nullptr);

	void UpdateCamera();
};