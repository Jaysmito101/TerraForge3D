#include <Camera.h>


#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>

Camera::Camera() {
	fov = 45;
	near = 0.01f;
	far = 200.0f;
	aspect = 16.0f / 9.0f;
	pitch = yaw = roll = 0;
	view = glm::mat4(1.0f);
	pv = glm::mat4(1.0f);
	pers = glm::perspective(fov, aspect, near, far);
	position = glm::vec3(0.0f, 0.0f, 3.0f);
	rotation = glm::vec3(1.0f);
}

void Camera::UpdateCamera(float* pos, float* rot) {
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

void Camera::UpdateRotation()
{
	pitch = rotation.x;
	yaw = rotation.y;
	roll = rotation.z;
}
