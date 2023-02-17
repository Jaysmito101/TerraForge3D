#include "utils/data.cl"
#include "utils/noise.cl"

#ifndef NOISE_LAYER_CL
#define NOISE_LAYER_CL  
 
__kernel void process_map_noise_layer(__global NoiseLayer* nl,
					__global int* nls,
					__global float4* data_layer_0,
					__global float4* data_layer_1,
					__global float4* data_layer_2,
					__global float4* data_layer_3,
					__global float4* data_layer_4,
					__global float4* data_layer_5)
{ 
	int x_coord = get_global_id(0);
	int y_coord = get_global_id(1);
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

	data_layer_0[y_coord * tileResolution + x_coord].x = UpdateLayerWithUpdateMethod(data_layer_0[y_coord * tileResolution + x_coord].x, n, (int)nl[0].offset.w);
}


#endif // NOISE_LAYER_CL