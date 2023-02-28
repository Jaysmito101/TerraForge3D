#pragma once

#define GLM_FORCE_INLINE
#define GLM_FORCE_SIMD_AVX2
#include <glm/glm.hpp>
#include <vector>

enum MeshType
{
	Plane = 0,
	Icosphere
};

struct Vert
{
	glm::vec4 position;
	glm::vec4 normal;
	glm::vec4 texCoord;
//	glm::vec4 extras1;
};

union Face
{
	struct
	{
		int a;
		int b;
		int c;
	};
	int indices[3];
};

class Mesh
{
public:
	Mesh();
	~Mesh();
	void RecalculateNormals();
	void Subdivide();
	void GeneratePlane(int resolution, float scale, float textureScale = 1.0f);
	void GenerateScreenQuad(float dist = 0);
	void GenerateSphere(int resolution, float radius);
	void GenerateTorus(float outerRadius, float innerRadius, int numSegments, int numRings);
	void GenerateCube();
	void ClearNormals();
	Mesh *Clone();
	bool IsValid();
	void Clear();

	inline const std::vector<Vert>& GetVertices() const { return m_Vertices; }
	inline int GetVertexCount() const { return (int)m_Vertices.size(); }
	inline const std::vector<Face>& GetFaces() const { return m_Faces; }
	inline const int* GetIndicesPTR() const { return reinterpret_cast<const int*>(m_Faces.data()); }
	inline const Vert* GetVerticesPTR() const { return m_Vertices.data(); }
	inline int GetIndexCount() const { return (int)m_Faces.size() * 3; }
	inline int GetFaceCount() const { return (int)m_Faces.size(); }
	inline const glm::vec4& GetPosition(int index) const { return m_Vertices[index].position; }
	inline const glm::vec4& GetNormal(int index) const { return m_Vertices[index].normal; }
	inline const glm::vec4& GetTexCoord(int index) const { return m_Vertices[index].texCoord; }
	inline const Face& GetFace(int index) const { return m_Faces[index]; }
	inline const Vert& GetVertex(int index) const { return m_Vertices[index]; }
	inline void SetVertex(const Vert& v, int index) { m_Vertices[index] = v; }
	inline void SetPosition(const glm::vec4& v, int index) { m_Vertices[index].position = v; }
	inline void SetNormal(const glm::vec4& v, int index) { m_Vertices[index].normal = v; }



private:
	std::vector<Vert> m_Vertices;
	std::vector<Face> m_Faces;
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
};
