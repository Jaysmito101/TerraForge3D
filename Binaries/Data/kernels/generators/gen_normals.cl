#include "utils/data.cl"

#ifndef GEN_NORMALS_CL
#define GEN_NORMALS_CL

__kernel void gen_normals(__global Vert* mesh, __global int* indices)
{
	int i = get_global_id(0);
	
	int ia = indices[i];
	int ib = indices[i + 1];
	int ic = indices[i + 2];

	float3 e1 = mesh[ia].position.xyz - mesh[ib].position.xyz;
	float3 e2 = mesh[ic].position.xyz - mesh[ib].position.xyz;
	float3 no = cross(e1, e2);

	float4 nor;
	nor.x = no.x;
	nor.y = no.y;
	nor.z = no.z;
	nor.w = 0.0f;

	mesh[ia].normal += nor;
	mesh[ib].normal += nor;
	mesh[ic].normal += nor;

}

__kernel void normalize_normals(__global Vert* mesh)
{
	int i = get_global_id(0);
	float ls = mesh[i].normal.x*mesh[i].normal.x + mesh[i].normal.y*mesh[i].normal.y + mesh[i].normal.z*mesh[i].normal.z;
	float l = sqrt(ls);
	mesh[i].normal /= l;
}

#endif //  GEN_NORMALS_CL