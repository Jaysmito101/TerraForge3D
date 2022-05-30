#include "Base/DS/Mesh.hpp"
#include "Base/Renderer/NativeMesh.hpp"

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
		this->name = name;
		nativeHandle = RendererAPI::NativeMesh::Create();
	}

	Mesh::~Mesh()
	{
		if (nativeHandle)
			delete nativeHandle;
	}

	bool Mesh::Clear()
	{
		vertices.clear();
		faces.clear();
		if(nativeHandle->IsSetup())
			return nativeHandle->Destroy();
		return true;
	}


	bool Mesh::UploadToGPU()
	{
		if (nativeHandle->IsSetup() && ( nativeHandle->GetIndexCount() != faces.size() * 3 || nativeHandle->GetVertexCount() != vertices.size()) )
			nativeHandle->Destroy();
		if (!nativeHandle->IsSetup())
		{
			nativeHandle->SetIndexCount(faces.size() * 3);
			nativeHandle->SetVertexCount(vertices.size());
		}
		bool status = nativeHandle->Setup();
		status = status && nativeHandle->UploadData(vertices.data(), faces.data());
		return status;
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