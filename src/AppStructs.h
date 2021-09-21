#pragma once

#include <string.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>
#include <time.h>
#include <json.hpp>

struct Vec2 {
	float x = 0;
	float y = 0;
};

struct Vec3 {
	float x = 0;
	float y = 0;
	float z = 0;
};

struct Vec4 {
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;
};

struct Vert {
	glm::vec3 position;
	glm::vec3 normal;
};

struct Mesh {
	Vert* vert;
	int* indices;
	int vertexCount;
	int indexCount;
};

struct Camera {
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

	Camera() {
		fov = 45;
		near = 0.01f;
		far = 100.0f;
		aspect = 16.0f / 9.0f;
		pitch = yaw = roll = 0;
		view = glm::mat4(1.0f);
		pv = glm::mat4(1.0f);
		pers = glm::perspective(fov, aspect, near, far);
		position = glm::vec3(0.0f, 0.0f, 3.0f);
		rotation = glm::vec3(1.0f);
	}

	void UpdateRotation()
	{
		pitch = rotation.x;
		yaw = rotation.y;
		roll = rotation.z;
	}

	void UpdateCamera(float* pos, float* rot)
	{
		position.x = pos[0];
		position.y = pos[1];
		position.z = pos[2];
		rotation.x = glm::radians(rot[0]);
		rotation.y = glm::radians(rot[1]);
		rotation.z = glm::radians(rot[2]);
		UpdateRotation();
		view = glm::lookAt(position, position + cameraFront, cameraUp);
		view = glm::rotate(view, glm::radians(rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
		view = glm::rotate(view, glm::radians(rotation.x), glm::vec3(0.0f, 1.0f, 0.0f));
		pers = glm::perspective(fov, aspect, near, far);
		pv = pers * view;
	}
};

struct Stats
{
	float deltaTime = 1;
	float frameRate = 1;
	int triangles = 0;
	int vertCount = 0;
};

struct NoiseLayer {

	NoiseLayer() {
		noiseType = "Simplex Perlin";
		strcpy_s(name, "Noise Layer");
		strength = 0.0f;
		enabled = true;
		active = false;
		scale = 1;
		offsetX = 0;
		offsetY = 0;
	}

	const char* noiseType;
	char name[256];
	float strength;
	float offsetX, offsetY;
	float scale;
	bool enabled;
	bool active;
};

struct ActiveWindows {
	bool styleEditor = false;
	bool statsWindow = false;
	bool shaderEditorWindow = false;
};