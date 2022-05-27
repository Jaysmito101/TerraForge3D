#include "Base/DS/Mesh.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_INLINE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>



namespace TerraForge3D
{



	Mesh::Mesh(std::string name)
	{
	}

	Mesh::~Mesh()
	{
		if (nativeHandle)
		{
			// nativeHandle->Destory();
			delete nativeHandle;
		}
	}

	bool Mesh::Clear()
	{
		return false;
	}

	bool Mesh::SetupOnGPU()
	{
		return false;
	}

	bool Mesh::UploadToGPU()
	{
		return false;
	}

	void Mesh::RecalculateMatices()
	{
		glm::mat4 rotM = glm::mat4(1.0f);
		glm::mat4 transM = glm::mat4(1.0f);

		rotM = glm::rotate(rotM, glm::radians(rotation[0] * (flipY ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(rotation[1]), glm::vec3(0.0f, 1.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(rotation[2]), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::vec3 translation = glm::vec3(position[0], position[1], position[2]);
		if (flipY)
			translation.y *= -1.0f;
		transM = glm::translate(transM, translation);

		modelMatrix = transM * rotM;
	}

}