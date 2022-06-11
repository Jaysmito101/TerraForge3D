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

		Vertex(float x, float y, float z)
		: position(x, y, z, 0.0f)
		{}

		Vertex(){}
	};

	struct Face
	{
		uint32_t a = 0;
		uint32_t b = 0;
		uint32_t c = 0;

		Face(uint32_t a, uint32_t b, uint32_t c)
		:a(a), b(b), c(c)
		{}

		Face(){}
	};


	class Mesh
	{
	public:
		Mesh(std::string name = "Mesh");
		virtual ~Mesh();

		bool Clear();

		bool UploadToGPU();

		void RecalculateMatices();


		inline RendererAPI::NativeMesh* GetNativeMesh() { return this->nativeHandle; }
		inline glm::mat4& GetModelMatrix() { return modelMatrix; };

		// Mesh Generators
		Mesh& Triangle(float* A, float* B, float* C);
		Mesh& Plane(glm::vec3 position, glm::vec3 right, glm::vec3 front, uint32_t resolution = 256);
		Mesh& Sphere(glm::vec3 position, float radius);

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