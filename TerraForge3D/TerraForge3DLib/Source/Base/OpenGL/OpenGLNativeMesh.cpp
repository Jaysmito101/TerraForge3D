#include "Base/Renderer/NativeMesh.hpp"
#include "Base/OpenGL/NativeMesh.hpp"

#include "Base/DS/Mesh.hpp"

#ifdef TF3D_OPENGL_BACKEND
namespace TerraForge3D
{

	namespace OpenGL
	{
		NativeMesh::~NativeMesh()
		{
			if (autoDestroy)
				Destroy();
		}

		bool NativeMesh::Setup()
		{
			TF3D_ASSERT(!isSetup, "Mesh has already been setup call destory first");
			TF3D_ASSERT(indexCount > 0 && vertexCount > 0, "Invalid vertex or index cound provided");
			TF3D_ASSERT(indexCount % 3 == 0, "Index count must be a multiple of 3");

			glGenVertexArrays(1, &vertexArray);
			glGenBuffers(1, &vertexBuffer);
			glGenBuffers(1, &indexBuffer);

			glBindVertexArray(vertexArray);
			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), nullptr, GL_STATIC_DRAW);

			// vertex positions
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
			// vertex texture coords
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
			// vertex normals
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
			// vertex extras
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, extra));


			glBindVertexArray(0);

			isSetup = true;

			triangleCount = indexCount / 3;

			return true;
		}

		bool NativeMesh::Destroy()
		{
			TF3D_ASSERT(isSetup, "Mesh has not yet been setup call setup first");
			glDeleteVertexArrays(1, &vertexArray);
			glDeleteBuffers(1, &vertexBuffer);
			glDeleteBuffers(1, &indexBuffer);
			isSetup = false;
			return true;
		}

		bool NativeMesh::UploadData(void* vertices, void* indices)
		{
			TF3D_ASSERT(isSetup, "Mesh has not yet been setup call setup first");
			TF3D_ASSERT(vertices, "Vertices is null");
			TF3D_ASSERT(indices, "Indices is null");

			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);
			return true;
		}
	}

}
#endif