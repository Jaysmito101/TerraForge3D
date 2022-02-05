#include <Camera.h>

#include "imgui/imgui.h"
#include <string>


#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>

static int ID = 0;

Camera::Camera() {
	fov = 45;
	cNear = 0.01f;
	cFar = 200.0f;
	aspect = 16.0f / 9.0f;
	pitch = yaw = roll = 0;
	view = glm::mat4(1.0f);
	pv = glm::mat4(1.0f);
	pers = glm::perspective(fov, aspect, cNear, cFar);
	mposition = glm::vec3(0.0f, 0.0f, 3.0f);
	mrotation = glm::vec3(1.0f);

	position[0] = 0.0f;
	position[1] = 0.2f;
	position[2] = 3.1f;

	rotation[0] = 1.0f;
	rotation[1] = 2530.0f;
	rotation[2] = 1.0f;

	camID = ID++;
}

nlohmann::json Camera::Save()
{
	nlohmann::json data;

	data["cNear"] = cNear;
	data["cFar"] = cFar;
	data["aspect"] = aspect;
	data["fov"] = fov;
	data["ID"] = camID;
	
	nlohmann::json tmp;
	tmp["x"] = position[0];
	tmp["y"] = position[1];
	tmp["z"] = position[2];
	data["position "] = tmp;

	tmp = nlohmann::json();
	tmp["x"] = rotation[0];
	tmp["y"] = rotation[1];
	tmp["z"] = rotation[2];
	data["rotation"] = tmp;


	return data;
}

void Camera::Load(nlohmann::json data)
{
	cNear = data["cNear"];
	cFar = data["cFar"];
	aspect = data["aspect"];
	fov = data["fov"];
	camID = data["ID"];

	position[0] = data["position"]["x"];
	position[1] = data["position"]["y"];
	position[2] = data["position"]["z"];

	rotation[0] = data["rotation"]["x"];
	rotation[1] = data["rotation"]["y"];
	rotation[2] = data["rotation"]["z"];
}

void Camera::UpdateCamera() {
	mposition.x = position[0];
	mposition.y = position[1];
	mposition.z = position[2];
	mrotation.x = glm::radians(rotation[0]);
	mrotation.y = glm::radians(rotation[1]);
	mrotation.z = glm::radians(rotation[2]);
	pitch = mrotation.x;
	yaw = mrotation.y;
	roll = mrotation.z;
	view = glm::lookAt(mposition, mposition + cameraFront, cameraUp);
	view = glm::rotate(view, glm::radians(mrotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::rotate(view, glm::radians(mrotation.x), glm::vec3(0.0f, 1.0f, 0.0f));
	pers = glm::perspective(fov, aspect, cNear, cFar);
	pv = pers * view;
}

void Camera::ShowSettings(bool renderWindow, bool* pOpen)
{
	if(pOpen == nullptr || *pOpen)
	{
		if (renderWindow)
			ImGui::Begin(("Camera Controls##" + std::to_string(camID)).c_str(), pOpen);

		ImGui::Text("Camera Position");
		ImGui::DragFloat3("##cameraPosition", position, 0.1f);
		ImGui::Separator();
		ImGui::Separator();
		ImGui::Text("Camera Rotation");
		ImGui::DragFloat3("##cameraRotation", rotation, 10);
		ImGui::Separator();
		ImGui::Separator();
		ImGui::Text("Projection Settings");
		ImGui::Separator();
		ImGui::DragFloat("FOV", &fov, 0.01f);
		ImGui::DragFloat("Aspect Ratio", &aspect, 0.01f);
		ImGui::DragFloat("Near Clipping", &cNear, 0.01f);
		ImGui::DragFloat("Far Clipping", &cFar, 0.01f);

		if (renderWindow)
			ImGui::End();
	}
}
