#include "utils/data.cl"
#include "utils/noise.cl"

#ifndef NOISE_LAYER_CL
#define NOISE_LAYER_CL

__kernel void noise_layer_terrain(__global Vert* mesh, __global NoiseLayer* nl, __global int* nls)
{
	int i = get_global_id(0);
	int size=*nls;
	float n = 0.0f;
	float4 position = mesh[i].position;
	position.y = 0.0f;
	NoiseLayer noiseLayer;
	for(int j=0;j<size;j++)
	{
		noiseLayer = nl[j];
		noiseLayer.value = position;
		n += get_noise(noiseLayer);
	}
	mesh[i].position.y += n;
	mesh[i].extras1.x += n;

}

__kernel void noise_layer_custom_base(__global Vert* mesh, __global Vert* mesh_copy, __global NoiseLayer* nl, __global int* nls)
{
	int i = get_global_id(0);
	int size=*nls;
	float n = 0.0f;
	float4 position = mesh_copy[i].position;	
	NoiseLayer noiseLayer;
	for(int j=0;j<size;j++)
	{
		noiseLayer = nl[j];
		noiseLayer.value = position;
		n += get_noise(noiseLayer);
	}
	mesh[i].position += mesh_copy[i].normal * n;
	mesh[i].extras1.x += n;
}

#endif // NOISE_LAYER_CL