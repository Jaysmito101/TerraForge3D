#include "utils/data.cl"
#include "utils/noise.cl"

#ifndef NOISE_LAYER_CL
#define NOISE_LAYER_CL  
 
__kernel void process_map_noise_layer(__global NoiseLayer* nl,
					__global int* nls,
					__global float4* data_layer_0,
					__global float4* data_layer_1)
{ 
	int tileX = (int)nl[0].value.z;
	int tileY = (int)nl[0].value.w;
	int tileRes = (int)nl[0].value.x;
	int x_coord = get_global_id(0) + tileX * tileRes;
	int y_coord = get_global_id(1) + tileY * tileRes;
	int tileResolution = (int)nl[0].offset.z;
	int size = *nls;

	float4 position = (float4){x_coord * nl[0].strength + nl[0].offset.x, 0.0f,  y_coord * nl[0].strength + nl[0].offset.y, 0.0f};
	position = position * nl[1].frequency + nl[1].offset;

	NoiseLayer noiseLayer; float n = 0.0f;
	for(int j = 2; j < size ; j++)
	{
		noiseLayer = nl[j];
		noiseLayer.value = position;
		n += get_noise(noiseLayer);
	}

	n = n * nl[1].strength;
	float previous_data = data_layer_0[get_global_id(1) * tileRes + get_global_id(0)].x;
	data_layer_0[get_global_id(1) * tileRes + get_global_id(0)].x = UpdateLayerWithUpdateMethod(previous_data, n, (int)nl[0].offset.w);
}


#endif // NOISE_LAYER_CL