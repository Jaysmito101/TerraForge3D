#pragma once

#include <glm/glm.hpp>

#undef far
#undef near

class Camera {
public:
	glm::mat4 view;
	glm::mat4 pers;
	glm::mat4 pv;
	glm::vec3 position;
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::vec3 rotation;
	float pitch;
	float yaw;
	float roll;
	float fov;
	float far;
	float aspect;
	float near;

	Camera();

	void UpdateRotation();

	void UpdateCamera(float* position, float* rotation);
};