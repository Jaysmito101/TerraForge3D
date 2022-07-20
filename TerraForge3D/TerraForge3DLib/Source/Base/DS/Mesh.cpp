#include "Base/DS/Mesh.hpp"
#include "Base/Renderer/NativeMesh.hpp"

#include "Base/Math/Math.hpp"

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

	bool Mesh::Clear(bool destroyNativeMesh)
	{
		vertices.clear();
		faces.clear();
		if (destroyNativeMesh && nativeHandle->IsSetup())
			return nativeHandle->Destroy();
		return true;
	}

	void Mesh::Clone(Mesh* other)
	{
		other->name = this->name + " [CLONE]";
		
		other->flipY = this->flipY;
		
		other->rotation[0] = this->rotation[0];
		other->rotation[1] = this->rotation[1];
		other->rotation[2] = this->rotation[2];
		other->rotation[3] = this->rotation[3];
		
		other->position[0] = this->position[0];
		other->position[1] = this->position[1];
		other->position[2] = this->position[2];
		other->position[3] = this->position[3];

		other->scale[0] = this->scale[0];
		other->scale[1] = this->scale[1];
		other->scale[2] = this->scale[2];
		other->scale[3] = this->scale[3];
		
		other->vertices = this->vertices;
		other->faces = this->faces;
	}

	bool Mesh::UploadToGPU()
	{
		if (nativeHandle->IsSetup() && ( nativeHandle->GetIndexCount() != faces.size() * 3 || nativeHandle->GetVertexCount() != vertices.size()) )
			nativeHandle->Destroy();
		bool status = true;
		if (!nativeHandle->IsSetup())
		{
			nativeHandle->SetIndexCount(faces.size() * 3);
			nativeHandle->SetVertexCount(vertices.size());
			status = nativeHandle->Setup();
		}
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

	Mesh& Mesh::CentroidSubdivision(float* progress)
	{
		if (progress)
			*progress = 0.0f;
		std::vector<Face> newFaces;
		for (Face& face : faces)
		{
			Vertex& a = vertices[face.a];
			Vertex& b = vertices[face.b];
			Vertex& c = vertices[face.c];

			Vertex centroid;
			centroid.position = (a.position + b.position + c.position) / 3.0f;
			centroid.texCoord = (a.texCoord + b.texCoord + c.texCoord) / 3.0f;
			centroid.extra = glm::vec4(0.0f);
			centroid.normal = glm::vec4(0.0f);
			vertices.push_back(centroid);

			newFaces.emplace_back(face.a, face.b, vertices.size() - 1);
			newFaces.emplace_back(vertices.size() - 1, face.b, face.c);
			newFaces.emplace_back(face.a, vertices.size() - 1, face.c);

			if (progress && newFaces.size() % 10 == 0)
				*progress = (float)newFaces.size() / faces.size();

		}
		if (progress)
			*progress = 1.1f;
		faces = newFaces;
		return *this;
	}

	Mesh& Mesh::RecalculateNormals(float* progress)
	{
		if (progress)
			*progress = 0.0f;
		glm::vec3 e1(0.0f);
		glm::vec3 e2(0.0f);
		glm::vec3 no(0.0f);
		
#ifndef TF3D_MESH_RETAIN_OLD_NORMALS
		for (Vertex& v : vertices)
			v.normal.x = v.normal.y = v.normal.z = v.normal.w = 0.0f;
#endif // TF3D_MESH_RETAIN_OLD_NORMALS

		for (Face& f : faces)
		{
		
			glm::vec4& tmp4a = vertices[f.a].position;
			glm::vec4& tmp4b = vertices[f.b].position;
			glm::vec4& tmp4c = vertices[f.c].position;
			TF3D_VEC3_SUB(e1, tmp4a, tmp4b);
			TF3D_VEC3_SUB(e2, tmp4c, tmp4b);
			TF3D_VEC3_CROSS(no, e1, e2);
			TF3D_VEC3_ADD(vertices[f.a].normal, no, vertices[f.a].normal);
			TF3D_VEC3_ADD(vertices[f.b].normal, no, vertices[f.b].normal);
			TF3D_VEC3_ADD(vertices[f.c].normal, no, vertices[f.c].normal);
		}
		if (progress)
			*progress = 0.5f;
#ifndef TF3D_MESH_UNSAFE_NORMALS
		for (Vertex& v : vertices)
		{
			TF3D_VEC3_NORMALIZE(v.normal);
			v.normal.w = 0.0f;
		}
#endif // TF3D_MESH_UNSAFE_NORMALS
		if (progress)
			*progress = 1.1f;
		return *this;
	}
}