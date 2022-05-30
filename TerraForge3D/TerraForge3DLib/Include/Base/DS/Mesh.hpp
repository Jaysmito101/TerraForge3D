#pragma once

#include "Base/Core/Core.hpp"

#include <glm/glm.hpp>

namespace TerraForge3D
{

	namespace RendererAPI
	{
		class NativeMesh;
	}

	struct Vertex 
	{
		glm::vec4 position = glm::vec4(0.0f);
		glm::vec4 texCoord = glm::vec4(0.0f);
		glm::vec4 normal   = glm::vec4(0.0f);
		glm::vec4 extra    = glm::vec4(0.0f);
	};

	struct Face
	{
		uint32_t a = 0;
		uint32_t b = 0;
		uint32_t c = 0;
	};


	class Mesh
	{
	public:
		Mesh(std::string name = "Mesh");
		~Mesh();

		bool Clear();

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

		std::vector<Vertex> vertices;
		std::vector<Face> faces;

		std::string name = "";

	private:
		RendererAPI::NativeMesh* nativeHandle = nullptr;
	};

}