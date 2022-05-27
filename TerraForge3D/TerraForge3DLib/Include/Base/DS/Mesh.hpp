#pragma once

#include "Base/Core/Core.hpp"

#include <glm/glm.hpp>

namespace TerraForge3D
{

	namespace RendererAPI
	{
		class NativeMesh;
	}

	class Mesh
	{
	public:
		Mesh(std::string name = "Mesh");
		~Mesh();

		bool Clear();

		bool SetupOnGPU();
		bool UploadToGPU();

		void RecalculateMatices();


		inline RendererAPI::NativeMesh* GetNativeMesh() { return this->nativeHandle; }
		inline glm::mat4& GetModelMatrix() { return modelMatrix; };

		// Mesh Generators
		Mesh* Triangle(float* A, float* B, float* C);

	public:
		float position[3] = { 0.0f, 0.0f, 0.0f };
		float rotation[3] = { 0.0f, 0.0f, 0.0f };
		float scale[3]    = { 1.0f, 1.0f, 1.0f };
		bool flipY = false;

		glm::mat4 modelMatrix = glm::mat4(1.0f);

	private:
		RendererAPI::NativeMesh* nativeHandle = nullptr;
	};

}