#include "utils/data.cl"

#ifndef CLEAR_MESH_CL
#define CLEAR_MESH_CL

__kernel void clear_mesh_terrain(__global Vert* mesh)
{
	int i = get_global_id(0);

	mesh[i].position.y = 0.0f;

}

__kernel void clear_mesh_custom_base(__global Vert* mesh, __global Vert* mesh_copy)
{
	int i = get_global_id(0);
	mesh[i] = mesh_copy[i];
}

#endif //  CLEAR_MESH_CL