#include "ExplorerControls.h"

#include <Application.h>

#include <Utils.h>
#include <GLFW/glfw3.h>

static float cpos[3], crot[3];

static glm::vec2 mosePos(0.0f);

void SetupExplorerControls()
{
	double x, y;
	glfwGetCursorPos(Application::Get()->GetWindow()->GetNativeWindow(), &x, &y);
	mosePos.x = (float)x;
	mosePos.y = (float)y;
}

void UpdateExplorerControls(float* pos, float* rot)
{
	double x, y;
	glfwGetCursorPos(Application::Get()->GetWindow()->GetNativeWindow(), &x, &y);
	float dX = (float)x - mosePos.x;
	float dY = (float)y - mosePos.y;
	mosePos.x = (float)x;
	mosePos.y = (float)y;
	rot[0] += dX * 10.0f;
	rot[1] += dY * 10.0f;

	static float movementSpeed = 0.001f;

	if (glfwGetKey(Application::Get()->GetWindow()->GetNativeWindow(), GLFW_KEY_W)) {
		glm::vec3 pPos = glm::vec3(pos[0], pos[1], pos[2]);
		float dist = pPos.x * pPos.x + pPos.y * pPos.y + pPos.z * pPos.z;
		pPos += glm::vec3(0.0f, 0.0f, -1.0f) * dist * movementSpeed;
		pos[0] = pPos.x;
		pos[1] = pPos.y;
		pos[2] = pPos.z;
	}

	if (glfwGetKey(Application::Get()->GetWindow()->GetNativeWindow(), GLFW_KEY_S)) {
		glm::vec3 pPos = glm::vec3(pos[0], pos[1], pos[2]);
		float dist = pPos.x * pPos.x + pPos.y * pPos.y + pPos.z * pPos.z;
		pPos -= glm::vec3(0.0f, 0.0f, -1.0f) * dist * movementSpeed;
		pos[0] = pPos.x;
		pos[1] = pPos.y;
		pos[2] = pPos.z;
	}

	if (glfwGetKey(Application::Get()->GetWindow()->GetNativeWindow(), GLFW_KEY_A)) {
		glm::vec3 pPos = glm::vec3(pos[0], pos[1], pos[2]);
		float dist =  pPos.y * pPos.y * 5 + 1;
		pPos -= glm::vec3(1.0f, 0.0f, 0.0f) * dist * movementSpeed;
		pos[0] = pPos.x;
		pos[1] = pPos.y;
		pos[2] = pPos.z;
	}

	if (glfwGetKey(Application::Get()->GetWindow()->GetNativeWindow(), GLFW_KEY_D)) {
		glm::vec3 pPos = glm::vec3(pos[0], pos[1], pos[2]);
		float dist = pPos.y * pPos.y * 5 + 1;
		pPos += glm::vec3(1.0f, 0.0f, 0.0f) * dist * movementSpeed;
		pos[0] = pPos.x;
		pos[1] = pPos.y;
		pos[2] = pPos.z;
	}
}

void ExPRestoreCamera(float* pos, float* rot)
{
	pos[0] = cpos[0];
	pos[1] = cpos[1];
	pos[2] = cpos[2];
			 
	rot[0] = crot[0];
	rot[1] = crot[1];
	rot[2] = crot[2];
}

void ExPSaveCamera(float* pos, float* rot)
{
	cpos[0] = pos[0];
	cpos[1] = pos[1];
	cpos[2] = pos[2];

	crot[0] = rot[0];
	crot[1] = rot[1];
	crot[2] = rot[2];
}
