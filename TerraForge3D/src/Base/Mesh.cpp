#include <Mesh.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>

// SIMD OPERATIONS

#include <immintrin.h>
// From :  https://geometrian.com/programming/tutorials/cross-product/index.php
[[nodiscard]] inline static __m128 cross_product(__m128 const& vec0, __m128 const& vec1) {
	__m128 tmp0 = _mm_shuffle_ps(vec0, vec0, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 tmp1 = _mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(3, 1, 0, 2));
	__m128 tmp2 = _mm_mul_ps(tmp0, vec1);
	__m128 tmp3 = _mm_mul_ps(tmp0, tmp1);
	__m128 tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
	return _mm_sub_ps(tmp3, tmp4);
}

void normalize(const glm::vec4& lpInput, glm::vec4& lpOutput) {
	const __m128& vInput = reinterpret_cast<const __m128&>(lpInput); // load input vector (x, y, z, a)
	__m128 vSquared = _mm_mul_ps(vInput, vInput); // square the input values
	__m128 vHalfSum = _mm_hadd_ps(vSquared, vSquared);
	__m128 vSum = _mm_hadd_ps(vHalfSum, vHalfSum); // compute the sum of values
	float fInvSqrt; _mm_store_ss(&fInvSqrt, _mm_rsqrt_ss(vSum)); // compute the inverse sqrt
	__m128 vNormalized = _mm_mul_ps(vInput, _mm_set1_ps(fInvSqrt)); // normalize the input vector
	lpOutput = reinterpret_cast<const glm::vec4&>(vNormalized); // store normalized vector (x, y, z, a)
}

// SIMD OPERATIONS

// QUICK HACKS

#define VEC3_SUB(a, b, out) out.x = a.x - b.x; \
			    out.y = a.y - b.y; \
			    out.z = a.z - b.z;

#define VEC3_ADD(a, b, out) out.x = a.x + b.x; \
			    out.y = a.y + b.y; \
			    out.z = a.z + b.z;

float inline __declspec (naked) __fastcall asm_sqrt(float n)
{
	_asm fld dword ptr [esp+4]
	_asm fsqrt
	_asm ret 4
} 

#define VEC3_NORMALIZE(v, out)  \
{				\
                                 \
	float tempLength = ( (v.x) * (v.x) ) + ( (v.y) * (v.y) ) +( (v.z) * (v.z) ); \
	float lengthSqrauedI = 1.0f / asm_sqrt(tempLength); \
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
	vert = nullptr;
	indices = nullptr;
}

Mesh::~Mesh() {
	if (deleteOnDestruction) {
		if (vert)
			delete vert;
		if (indices)
			delete indices;
	}
}

void Mesh::RecalculateNormals()
{
		glm::vec3 e1;
		glm::vec3 e2;	
		glm::vec3 no;

		int iabc[3];
		for (int i = 0; i < indexCount; i += 3)
		{
			iabc[0] = indices[i];
			iabc[1] = indices[i + 1];
			iabc[2] = indices[i + 2];

			glm::vec4& tmp4a = vert[iabc[0]].position;
			glm::vec4& tmp4b = vert[iabc[1]].position;
			glm::vec4& tmp4c = vert[iabc[2]].position;
						
			VEC3_SUB(tmp4a, tmp4b, e1);
			VEC3_SUB(tmp4c, tmp4b, e2);

			VEC3_CROSS(e1, e2, no);

			VEC3_ADD(vert[iabc[0]].normal, no, vert[iabc[0]].normal);
			VEC3_ADD(vert[iabc[1]].normal, no, vert[iabc[1]].normal);
			VEC3_ADD(vert[iabc[2]].normal, no, vert[iabc[2]].normal);
		}

		for (int i = 0; i < vertexCount; i++)
		{
			VEC3_NORMALIZE(vert[i].normal, vert[i].normal);
		}

}

void Mesh::GenerateIcoSphere(int resolution, float radius, float textureScale)
{
	currType = MeshType::Icosphere;
	glm::vec3 vertis[] = {
		glm::vec3(0, -1, 0),
		glm::vec3(0, 0, 1),
		glm::vec3(1, 0, 0),
		glm::vec3(0, 0, -1),
		glm::vec3(-1, 0, 0),
		glm::vec3(0, 1, 0)
	};

	int triangles[] = {
		   0, 1, 2,
		   0, 2, 3,
		   0, 3, 4,
		   0, 4, 1,

		   5, 2, 1,
		   5, 3, 2,
		   5, 4, 3,
		   5, 1, 4
	};

	for (int i = 0; i < sizeof(vertis)/sizeof(glm::vec3); i++) {
		vertis[i] *= radius;
	}

	vertexCount = sizeof(vertis)/sizeof(int);
	vert = new Vert[sizeof(vertis) / sizeof(int)];
	memset(vert, 0, sizeof(Vert) * vertexCount);
 	for (int i = 0; i < vertexCount; i++) {
		vert[i] = Vert();
		vert[i].position = glm::vec4(vertis[i], 1);
	}
	indexCount = sizeof(triangles)/sizeof(int);
	indices = new int[indexCount];
	memcpy(indices, triangles, sizeof(int) * indexCount);
}

void Mesh::GeneratePlane(int resolution, float scale, float textureScale)
{
	currType = MeshType::Plane;
	maxHeight = -100;
	minHeight = 100;
	res = resolution;
	sc = scale;
	texSc = textureScale;
	Vert* vertices = new Vert[resolution * resolution];
	int* inds = new int[(resolution-1) * (resolution-1) * 6];
	int triIndex = 0;

	for (int y = 0; y < resolution; y++)
	{ 
		for (int x = 0; x < resolution; x++)
		{
			int i = x + y * resolution;
			glm::vec2 percent = glm::vec2(x, y) / ((float)resolution - 1);
			glm::vec3 pointOnPlane = (percent.x - .5f) * 2 * right + (percent.y - .5f) * 2 * front;
			pointOnPlane *= scale;
			vertices[i] = Vert();
			vertices[i].position = glm::vec4(0.0f);

			vertices[i].position.x = (float)pointOnPlane.x;
			vertices[i].position.y = (float)pointOnPlane.y;
			vertices[i].position.z = (float)pointOnPlane.z;
			vertices[i].texCoord = glm::vec2(percent.x, percent.y)*textureScale;
			vertices[i].normal = glm::vec4(0.0f);
			if (x != resolution - 1 && y != resolution - 1)
			{
				inds[triIndex] = i;
				inds[triIndex + 1] = i + resolution + 1;
				inds[triIndex + 2] = i + resolution;

				inds[triIndex + 3] = i;
				inds[triIndex + 4] = i + 1;
				inds[triIndex + 5] = i + resolution + 1;
				triIndex += 6;
			}
			vertices[i].extras1 = glm::vec4(0.0f);			
		}
	}
	
	if (vert)
		delete vert;
	if (indices)
		delete indices;
	vertexCount = resolution * resolution;
	vert = new Vert[resolution * resolution];
	memset(vert, 0, sizeof(Vert) * vertexCount);
	memcpy(vert, vertices, sizeof(Vert)*vertexCount);
	indexCount = (resolution - 1) * (resolution - 1) * 6;
	indices = new int[(resolution - 1) * (resolution - 1) * 6];
	memset(indices, 0, indexCount);
	memcpy(indices, inds, sizeof(int) * indexCount);
	delete vertices;
	delete inds;
}

void Mesh::GenerateScreenQuad(float dist) 
{
	if (vert) 
		delete[] vert;
	if (indices)
		delete[] indices;
	Vert v;
	vert = new Vert[4];

	v.position = glm::vec4(-1, -1, dist, 0);
	v.texCoord = glm::vec2(0, 0);
	vert[0] = v;

	v.position = glm::vec4(-1, 1, dist, 0);
	v.texCoord = glm::vec2(0, 1);
	vert[1] = v;

	v.position = glm::vec4(1, 1, dist, 0);
	v.texCoord = glm::vec2(1, 1);
	vert[2] = v;

	v.position = glm::vec4(1, -1, dist, 0);
	v.texCoord = glm::vec2(1, 0);
	vert[3] = v;

	vertexCount = 4;

	indexCount = 6;

	indices = new int[6];

	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 0;
	indices[4] = 2;
	indices[5] = 3;
}

void Mesh::SetElevation(float elevation, int x, int y){
	if(!vert)
		return;
	int i = x + y * res;
	if (i > vertexCount)
		return;
	if (elevation > maxHeight)
		maxHeight = elevation;
	if (elevation < minHeight)
		minHeight = elevation;
	vert[i].position.y = elevation;
	vert[i].extras1.x = elevation;
}

float Mesh::GetElevation(int x, int y) {
	if (!vert)
		return 0;
	int i = x + y * res;
	if (i > vertexCount)
		return 0;
	
	return vert[i].position.y;
}

glm::vec3 Mesh::GetNormals(int x, int y) {
	if (!vert)
		return glm::vec3(0);
	int i = x + y * res;
	if (i > vertexCount)
		return glm::vec3(0);
	return glm::vec3(vert[i].normal.x, vert[i].normal.y, vert[i].normal.z);
}

void Mesh::AddElevation(float elevation, int x, int y){
	if(!vert)
		return;
	int i = x + y * res;
	if (i > vertexCount)
		return;
	vert[i].position.y += elevation;
}

glm::vec2 Mesh::GetTexCoord(float x, float y, float z)
{
	if (currType == MeshType::Plane)
	{
		return (glm::vec2(x, y) / ((float)res - 1)) * texSc;
	}
	else {
		return glm::vec2(1.0f);
	}
}

Mesh* Mesh::Clone()
{
	Mesh* cloneMesh = new Mesh();
	cloneMesh->res = res;
	cloneMesh->sc = sc;
	cloneMesh->maxHeight = maxHeight;
	cloneMesh->vertexCount = vertexCount;
	cloneMesh->indexCount = indexCount;
	cloneMesh->vert = new Vert[vertexCount];
	memcpy(cloneMesh->vert, vert, sizeof(Vert) * vertexCount);
	cloneMesh->indices = new int[indexCount];
	memcpy(cloneMesh->indices, indices, sizeof(int) * indexCount);
	return cloneMesh;
}

bool Mesh::IsValid() {
	if (vert && indices)
		return true;
	return false;
}