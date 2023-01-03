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


#include <immintrin.h>


// Load an FP32 3D vector from memory
inline __m128 loadFloat3(const glm::vec3& pos)
{
	static_assert(sizeof(glm::vec3) == 12, "Expected to be 12 bytes (3 floats)");
	__m128 low = _mm_castpd_ps(_mm_load_sd((const double*)&pos));
	// Modern compilers are optimizing the following 2 intrinsics into a single insertps with a memory operand
	__m128 high = _mm_load_ss(((const float*)&pos) + 2);
	return _mm_insert_ps(low, high, 0x27);
}

// Store an FP32 3D vector to memory
inline void storeFloat3(glm::vec3& pos, __m128 vec)
{
	_mm_store_sd((double*)&pos, _mm_castps_pd(vec));
	((int*)(&pos))[2] = _mm_extract_ps(vec, 2);
}

// Normalize a 3D vector; if the source is zero, will instead return a zero vector
inline __m128 vector3Normalize(__m128 vec)
{
	// Compute x^2 + y^2 + z^2, broadcast the result to all 4 lanes
	__m128 dp = _mm_dp_ps(vec, vec, 0b01111111);
	// res = vec / sqrt( dp )
	__m128 res = _mm_div_ps(vec, _mm_sqrt_ps(dp));
	// Compare for dp > 0
	__m128 positiveLength = _mm_cmpgt_ps(dp, _mm_setzero_ps());
	// Zero out the vector if the dot product was zero
	return _mm_and_ps(res, positiveLength);
}

// Copy-pasted from there: https://github.com/microsoft/DirectXMath/blob/jan2021/Inc/DirectXMathVector.inl#L9506-L9519
// MIT license
#ifdef __AVX__
#define XM_PERMUTE_PS( v, c ) _mm_permute_ps( v, c )
#else
#define XM_PERMUTE_PS( v, c ) _mm_shuffle_ps( v, v, c )
#endif

#ifdef __AVX2__
#define XM_FNMADD_PS( a, b, c ) _mm_fnmadd_ps((a), (b), (c))
#else
#define XM_FNMADD_PS( a, b, c ) _mm_sub_ps((c), _mm_mul_ps((a), (b)))
#endif

inline __m128 vector3Cross(__m128 V1, __m128 V2)
{
	// y1,z1,x1,w1
	__m128 vTemp1 = XM_PERMUTE_PS(V1, _MM_SHUFFLE(3, 0, 2, 1));
	// z2,x2,y2,w2
	__m128 vTemp2 = XM_PERMUTE_PS(V2, _MM_SHUFFLE(3, 1, 0, 2));
	// Perform the left operation
	__m128 vResult = _mm_mul_ps(vTemp1, vTemp2);
	// z1, x1, y1, w1
	vTemp1 = XM_PERMUTE_PS(vTemp1, _MM_SHUFFLE(3, 0, 2, 1));
	// y2, z2, x2, w2
	vTemp2 = XM_PERMUTE_PS(vTemp2, _MM_SHUFFLE(3, 1, 0, 2));
	// Perform the right operation
	vResult = XM_FNMADD_PS(vTemp1, vTemp2, vResult);
	return vResult;
}

// QUICK HACKS

#define VEC3_SUB(a, b, out) out.x = a.x - b.x; \
			    out.y = a.y - b.y; \
			    out.z = a.z - b.z;

#define VEC3_ADD(a, b, out) out.x = a.x + b.x; \
			    out.y = a.y + b.y; \
			    out.z = a.z + b.z;
/*
// Does not work in 64 bit mode
inline float __fastcall asm_sqrt(float n)
{
	_asm fld dword ptr [esp+4]
	_asm fsqrt
	_asm ret 4
}
*/

inline float Q_rsqrt(float number)
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
	vert = nullptr;
	indices = nullptr;
}

Mesh::~Mesh()
{
	if (deleteOnDestruction)
	{
		if (vert)
		{
			delete vert;
		}

		if (indices)
		{
			delete indices;
		}
	}
}

void Mesh::ClearNormals()
{
	for (int i = 0; i < vertexCount; i++)
	{
		vert[i].normal.x = 0.0f;
		vert[i].normal.y = 0.0f;
		vert[i].normal.z = 0.0f;
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

void Mesh::GenerateSphere(int resolution, float radius)
{
	currType = MeshType::Icosphere;
	maxHeight = -100;
	minHeight = 100;
	res = resolution;
	sc = radius;

	Vert* vertices = new Vert[resolution * resolution * 6];
	int* inds = new int[(resolution - 1) * (resolution - 1) * 6 * 6];

	int triIndex = 0;

	const std::vector<glm::vec3> right_vs = {
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, -1.0f)
	};

	const std::vector<glm::vec3> front_vs = {
		glm::vec3(0.0f, 0.0f, -1.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, -1.0f, 0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(-1.0f, 0.0f, 0.0f)
	};



	for (int fc = 0; fc < 6; fc++)
	{
		for (int y = 0; y < resolution; y++)
		{
			for (int x = 0; x < resolution; x++)
			{
				int i = x + y * resolution + resolution * resolution * fc;
				glm::vec2 percent = glm::vec2(x, y) / ((float)resolution - 1);
				glm::vec3 nor = glm::cross(front_vs[fc], right_vs[fc]);
				glm::vec3 pointOnPlane = (percent.x - .5f) * 2 * right_vs[fc] + (percent.y - .5f) * 2 * front_vs[fc] + nor * 1.0f;
				pointOnPlane = glm::normalize(pointOnPlane);
				vertices[i] = Vert();
				vertices[i].position = glm::vec4(0.0f);
				vertices[i].position.x = (float)pointOnPlane.x;
				vertices[i].position.y = (float)pointOnPlane.y;
				vertices[i].position.z = (float)pointOnPlane.z;
				vertices[i].texCoord = glm::vec2(percent.x, percent.y);
				vertices[i].normal = glm::vec4(nor, 0.0f);

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
	}

	if (vert) delete[] vert;
	if (indices) delete[] indices;
	vertexCount = resolution * resolution * 6;
	vert = new Vert[vertexCount];
	memcpy(vert, vertices, sizeof(Vert) * vertexCount);
	indexCount = (resolution - 1) * (resolution - 1) * 6 * 6;
	indices = new int[indexCount];
	memcpy(indices, inds, sizeof(int) * indexCount);
	delete[] vertices;
	delete[] inds;


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
	int* inds = new int[(resolution - 1) * (resolution - 1) * 6];
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
			vertices[i].texCoord = glm::vec2(percent.x, percent.y) * textureScale;
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

	if (vert) delete[] vert;
	if (indices) delete[] indices;

	vertexCount = resolution * resolution;
	vert = new Vert[vertexCount];
	memcpy(vert, vertices, sizeof(Vert) * vertexCount);
	indexCount = (resolution - 1) * (resolution - 1) * 6;
	indices = new int[indexCount];
	memcpy(indices, inds, sizeof(int) * indexCount);
	delete[] vertices;
	delete[] inds;
}

void Mesh::GenerateScreenQuad(float dist)
{
	if (vert) delete[] vert;
	if (indices) delete[] indices;

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

void Mesh::SetElevation(float elevation, int x, int y)
{
	if (!vert)
	{
		return;
	}

	int i = x + y * res;

	if (i > vertexCount)
	{
		return;
	}

	if (elevation > maxHeight)
	{
		maxHeight = elevation;
	}

	if (elevation < minHeight)
	{
		minHeight = elevation;
	}

	vert[i].position.y = elevation;
	vert[i].extras1.x = elevation;
}

float Mesh::GetElevation(int x, int y)
{
	if (!vert)
	{
		return 0;
	}

	int i = x + y * res;

	if (i > vertexCount)
	{
		return 0;
	}

	return vert[i].position.y;
}

glm::vec3 Mesh::GetNormals(int x, int y)
{
	if (!vert)
	{
		return glm::vec3(0);
	}

	int i = x + y * res;

	if (i > vertexCount)
	{
		return glm::vec3(0);
	}

	return glm::vec3(vert[i].normal.x, vert[i].normal.y, vert[i].normal.z);
}

void Mesh::AddElevation(float elevation, int x, int y)
{
	if (!vert)
	{
		return;
	}

	int i = x + y * res;

	if (i > vertexCount)
	{
		return;
	}

	vert[i].position.y += elevation;
}

glm::vec2 Mesh::GetTexCoord(float x, float y, float z)
{
	if (currType == MeshType::Plane)
	{
		return (glm::vec2(x, y) / ((float)res - 1)) * texSc;
	}

	else
	{
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

bool Mesh::IsValid()
{
	if (vert && indices)
	{
		return true;
	}

	return false;
}