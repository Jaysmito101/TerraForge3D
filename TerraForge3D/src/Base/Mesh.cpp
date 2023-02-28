#include <Mesh.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>

#include <vector>

#ifndef PI 
#define PI 3.141592653f
#endif

// From: https://github.com/cppcooper/icosahedron-sphere/blob/master/Source/Private/Geometry/icosphere.cpp
namespace Icosahedron
{

	const float X = .525731112119133606f;
	const float Z = .850650808352039932f;
	const float N = 0.f;

	static const float baseVertices[][3] =
	{
		{-X,N,Z}, {X,N,Z}, {-X,N,-Z}, {X,N,-Z},
		{N,Z,X}, {N,Z,-X}, {N,-Z,X}, {N,-Z,-X},
		{Z,X,N}, {-Z,X, N}, {Z,-X,N}, {-Z,-X, N}
	};

	static const int baseTriangles[][3] =
	{
		{0,4,1},{0,9,4},{9,5,4},{4,5,8},{4,8,1},
		{8,10,1},{8,3,10},{5,3,8},{5,2,3},{2,7,3},
		{7,10,3},{7,6,10},{7,11,6},{11,0,6},{0,1,6},
		{6,1,10},{9,0,11},{9,11,2},{9,2,5},{7,2,11}
	};

	static const int baseTriangleCount = 20;
	static const int baseVertexCount = 12;

	static inline glm::vec4 CalculateUV(const glm::vec4& normal)
	{
		glm::vec4 uv = glm::vec4(0.0f);
		const float& x = normal.x;
		const float& y = normal.y;
		const float& z = normal.z;
		float normalisedX = 0;
		float normalisedZ = -1;
		if (((x * x) + (z * z)) > 0)
		{
			normalisedX = sqrt((x * x) / ((x * x) + (z * z)));
			if (x < 0) normalisedX = -normalisedX;
			normalisedZ = sqrt((z * z) / ((x * x) + (z * z)));
			if (z < 0) normalisedZ = -normalisedZ;
		}
		if (normalisedZ == 0) uv.x = ((normalisedX * PI) / 2);
		else
		{
			uv.x = atan(normalisedX / normalisedZ);
			if (normalisedX < 0) uv.x = PI - uv.x;
			if (normalisedZ < 0) uv.x += PI;
		}
		if (uv.x < 0) uv.x += 2 * PI;
		uv.x /= 2 * PI;
		uv.y = (-y + 1) / 2;
		return uv;
	}
}


// QUICK HACKS

#define VEC3_SUB(a, b, out) out.x = a.x - b.x; \
			    out.y = a.y - b.y; \
			    out.z = a.z - b.z;

#define VEC3_ADD(a, b, out) out.x = a.x + b.x; \
			    out.y = a.y + b.y; \
			    out.z = a.z + b.z;

static inline float Q_rsqrt(float number)
{
	long i;
	float x2, y;
	const float threehalfs = 1.5F;
	x2 = number * 0.5F;
	y = number;
	i = *(long*)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float*)&i;
	y = y * (threehalfs - (x2 * y * y));
	// y = y * (threehalfs - (x2 * y * y));
	return y;
}

#define VEC3_NORMALIZE(v, out)  \
{				\
                                 \
	float tempLength = ( (v.x) * (v.x) ) + ( (v.y) * (v.y) ) +( (v.z) * (v.z) ); \
	float lengthSqrauedI = Q_rsqrt(tempLength); \
	out.x = (v.x) * lengthSqrauedI;              \
	out.y = (v.y) * lengthSqrauedI;              \
	out.z = (v.z) * lengthSqrauedI;              \
}

#define VEC3_CROSS(a, b, out) \
{                             \
	out.x = ( (a.y) * (b.z) ) - ( (a.z) * (b.y) ); \
	out.y = ( (a.z) * (b.x) ) - ( (a.x) * (b.z) ); \
	out.z = ( (a.x) * (b.y) ) - ( (a.y) * (b.x) ); \
}

// QUICK HACKS

Mesh::Mesh()
{
	m_Vertices.clear();
	m_Faces.clear();
}

Mesh::~Mesh()
{
	m_Vertices.clear();
	m_Faces.clear();
}

void Mesh::ClearNormals()
{
	for (auto& v : m_Vertices) v.normal = glm::vec4(0.0f);
}

void Mesh::RecalculateNormals()
{
	glm::vec3 e1;
	glm::vec3 e2;
	glm::vec3 no;
	for (auto& face : m_Faces)
	{
		glm::vec4& tmp4a = m_Vertices[face.a].position;
		glm::vec4& tmp4b = m_Vertices[face.b].position;
		glm::vec4& tmp4c = m_Vertices[face.c].position;
		VEC3_SUB(tmp4a, tmp4b, e1);
		VEC3_SUB(tmp4c, tmp4b, e2);
		VEC3_CROSS(e1, e2, no);
		VEC3_ADD(m_Vertices[face.a].normal, no, m_Vertices[face.a].normal);
		VEC3_ADD(m_Vertices[face.b].normal, no, m_Vertices[face.b].normal);
		VEC3_ADD(m_Vertices[face.c].normal, no, m_Vertices[face.c].normal);
	}
	for (auto& v : m_Vertices)
	{
		VEC3_NORMALIZE(v.normal, v.normal);
	}
}

void Mesh::GenerateSphere(int resolution, float radius)
{
	Clear();
	for (int stack = 0; stack <= resolution; stack++)
	{
		float phi = PI * stack / static_cast<float>(resolution);
		float cosPhi = std::cos(phi);
		float sinPhi = std::sin(phi);
		for (int slice = 0; slice <= resolution; slice++)
		{
			float theta = 2.0f * PI * slice / static_cast<float>(resolution);
			float cosTheta = std::cos(theta);
			float sinTheta = std::sin(theta);
			Vert vert;
			vert.position.x = radius * cosTheta * sinPhi;
			vert.position.y = radius * sinTheta * sinPhi;
			vert.position.z = radius * cosPhi;
			vert.position.w = 1.0f;
			vert.normal = glm::vec4(glm::normalize(glm::vec3(vert.position)), 0.0f);
			vert.texCoord.x = static_cast<float>(slice) / static_cast<float>(resolution);
			vert.texCoord.y = static_cast<float>(stack) / static_cast<float>(resolution);
			vert.texCoord.z = 0.0f;
			vert.texCoord.w = 0.0f;
			m_Vertices.push_back(vert);
		}
	}
	for (int stack = 0; stack < resolution; stack++) for (int slice = 0; slice < resolution; slice++)
	{
		int index0 = stack * (resolution + 1) + slice;
		int index1 = index0 + 1;
		int index2 = (stack + 1) * (resolution + 1) + slice;
		int index3 = index2 + 1;
		Face face1 = { index0, index1, index2 };
		Face face2 = { index2, index1, index3 };
		m_Faces.push_back(face1);
		m_Faces.push_back(face2);
	}
	for (auto& vert : m_Vertices) vert.position = glm::normalize(vert.position);
}

void Mesh::GeneratePlane(int resolution, float scale, float textureScale)
{
	Clear();
	int triIndex = 0;
	for (int y = 0; y < resolution; y++)
	{
		for (int x = 0; x < resolution; x++)
		{
			int i = x + y * resolution;
			glm::vec2 percent = glm::vec2(x, y) / ((float)resolution - 1);
			glm::vec3 pointOnPlane = (percent.x - .5f) * 2 * right + (percent.y - .5f) * 2 * front;
			pointOnPlane *= scale;
			Vert v;
			v.position = glm::vec4(0.0f);
			v.position.x = static_cast<float>(pointOnPlane.x);
			v.position.y = static_cast<float>(pointOnPlane.y);
			v.position.z = static_cast<float>(pointOnPlane.z);
			v.texCoord = glm::vec4(percent.x, percent.y, 0.0f, 0.0f) * textureScale;
			v.normal = glm::vec4(0.0f);
			m_Vertices.push_back(v);
			if (x != resolution - 1 && y != resolution - 1)
			{
				Face f0 = { i, i + resolution + 1, i + resolution };
				m_Faces.push_back(f0);

				Face f1 = { i, i + 1, i + resolution + 1 };
				m_Faces.push_back(f1);
			}
		}
	}
}

void Mesh::GenerateScreenQuad(float dist)
{
	Clear();

	Vert v;


	v.position = glm::vec4(-1, -1, dist, 0); 
	v.texCoord = glm::vec4(0, 0, 0, 0);
	m_Vertices.push_back(v);

	v.position = glm::vec4(-1, 1, dist, 0);
	v.texCoord = glm::vec4(0, 1, 0, 0);
	m_Vertices.push_back(v);

	v.position = glm::vec4(1, 1, dist, 0);
	v.texCoord = glm::vec4(1, 1, 0, 0);
	m_Vertices.push_back(v);

	v.position = glm::vec4(1, -1, dist, 0);
	v.texCoord = glm::vec4(1, 0, 0, 0);
	m_Vertices.push_back(v);

	 
	Face f0 = {0, 1, 2};
	m_Faces.push_back(f0);
	Face f1 = {0, 2, 3};
	m_Faces.push_back(f1);
}

void Mesh::GenerateCube()
{
	static float skyboxVertices[][3] = {
		{-1.0f,  1.0f, -1.0f},
		{-1.0f, -1.0f, -1.0f},
		{ 1.0f, -1.0f, -1.0f},
		{ 1.0f, -1.0f, -1.0f},
		{ 1.0f,  1.0f, -1.0f},
		{-1.0f,  1.0f, -1.0f},
		{-1.0f, -1.0f,  1.0f},
		{-1.0f, -1.0f, -1.0f},
		{-1.0f,  1.0f, -1.0f},
		{-1.0f,  1.0f, -1.0f},
		{-1.0f,  1.0f,  1.0f},
		{-1.0f, -1.0f,  1.0f},
		{ 1.0f, -1.0f, -1.0f},
		{ 1.0f, -1.0f,  1.0f},
		{ 1.0f,  1.0f,  1.0f},
		{ 1.0f,  1.0f,  1.0f},
		{ 1.0f,  1.0f, -1.0f},
		{ 1.0f, -1.0f, -1.0f},
		{-1.0f, -1.0f,  1.0f},
		{-1.0f,  1.0f,  1.0f},
		{ 1.0f,  1.0f,  1.0f},
		{ 1.0f,  1.0f,  1.0f},
		{ 1.0f, -1.0f,  1.0f},
		{-1.0f, -1.0f,  1.0f},
		{-1.0f,  1.0f, -1.0f},
		{ 1.0f,  1.0f, -1.0f},
		{ 1.0f,  1.0f,  1.0f},
		{ 1.0f,  1.0f,  1.0f},
		{-1.0f,  1.0f,  1.0f},
		{-1.0f,  1.0f, -1.0f},
		{-1.0f, -1.0f, -1.0f},
		{-1.0f, -1.0f,  1.0f},
		{ 1.0f, -1.0f, -1.0f},
		{ 1.0f, -1.0f, -1.0f},
		{-1.0f, -1.0f,  1.0f},
		{ 1.0f, -1.0f,  1.0f}
	};

	Clear();

	for (auto i = 0; i < 36; i++)
	{
		Vert v;
		v.position = glm::vec4(skyboxVertices[i][0], skyboxVertices[i][1], skyboxVertices[i][2], 0);
		m_Vertices.push_back(v);
	}

	for (auto i = 0; i < 12; i++)
	{
		Face f;
		f.a = i * 3 + 0;
		f.b = i * 3 + 1;
		f.c = i * 3 + 2;
		m_Faces.push_back(f);
	}
}

void Mesh::GenerateTorus(float outerRadius, float innerRadius, int numSegments, int numRings)
{
	Clear();
	for (int i = 0; i <= numRings; i++)
	{
		float ringAngle = glm::pi<float>() * 2.0f * i / numRings;
		float ringRadius = outerRadius + innerRadius * std::cos(ringAngle);
		for (int j = 0; j <= numSegments; j++)
		{
			float segmentAngle = glm::pi<float>() * 2.0f * j / numSegments;
			glm::vec3 position(std::cos(segmentAngle) * ringRadius, std::sin(segmentAngle) * ringRadius, innerRadius * std::sin(ringAngle));
			glm::vec3 tangent(-std::sin(segmentAngle), std::cos(segmentAngle), 0.0f);
			glm::vec3 bitangent(-std::cos(segmentAngle) * std::sin(ringAngle), -std::sin(segmentAngle) * std::sin(ringAngle), std::cos(ringAngle));
			glm::vec3 normal = glm::normalize(glm::cross(bitangent, tangent));
			glm::vec2 texCoord((float)i / numRings, (float)j / numSegments);
			m_Vertices.push_back({ glm::vec4(position, 1.0f), glm::vec4(normal, 0.0f), glm::vec4(texCoord, 0.0f, 0.0f) });
		}
	}
	for (int i = 0; i < numRings; i++) for (int j = 0; j < numSegments; j++)
	{
		int index0 = (i * (numSegments + 1)) + j;
		int index1 = index0 + numSegments + 1;
		int index2 = index0 + 1;
		int index3 = index1 + 1;
		Face f0 = {index0, index1, index2};
		m_Faces.push_back(f0);
		Face f1 = {index1, index3, index2};
		m_Faces.push_back(f1);
	}
}



Mesh* Mesh::Clone()
{
	Mesh* cloneMesh = new Mesh();
	cloneMesh->m_Vertices = m_Vertices;
	cloneMesh->m_Faces = m_Faces;
	return cloneMesh;
}

bool Mesh::IsValid()
{
	return (m_Vertices.size() > 0 && m_Faces.size() > 0);
}

void Mesh::Clear()
{
	m_Vertices.clear();
	m_Faces.clear();
}

void Mesh::Subdivide()
{
	std::vector<Vert> newVerts;
	std::vector<Face> newFaces;

	// Loop over each face
	for (auto face : m_Faces)
	{
		// Get the vertices of the current face
		Vert vertA = m_Vertices[face.a];
		Vert vertB = m_Vertices[face.b];
		Vert vertC = m_Vertices[face.c];

		// Calculate the midpoints of each edge
		Vert midAB, midBC, midCA;
		midAB.position = (vertA.position + vertB.position) / 2.0f;
		midBC.position = (vertB.position + vertC.position) / 2.0f;
		midCA.position = (vertC.position + vertA.position) / 2.0f;
		midAB.normal = midBC.normal = midCA.normal = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		midAB.texCoord = midBC.texCoord = midCA.texCoord = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

		// Add the new vertices to the list
		int indexA = newVerts.size();
		newVerts.push_back(vertA);
		int indexB = newVerts.size();
		newVerts.push_back(vertB);
		int indexC = newVerts.size();
		newVerts.push_back(vertC);
		int indexAB = newVerts.size();
		newVerts.push_back(midAB);
		int indexBC = newVerts.size();
		newVerts.push_back(midBC);
		int indexCA = newVerts.size();
		newVerts.push_back(midCA);

		// Create the new faces
		Face face1 = { face.a, indexAB, indexCA };
		Face face2 = { indexAB, face.b, indexBC };
		Face face3 = { indexCA, indexBC, face.c };
		Face face4 = { indexAB, indexBC, indexCA };

		// Add the new faces to the list
		newFaces.push_back(face1);
		newFaces.push_back(face2);
		newFaces.push_back(face3);
		newFaces.push_back(face4);
	}

	// Update the mesh with the new vertices and faces
	m_Vertices = newVerts;
	m_Faces = newFaces;
}
