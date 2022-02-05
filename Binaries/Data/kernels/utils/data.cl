#ifndef DATA_CL
#define DATA_CL

typedef struct __attribute__ ((packed)) Vert
{
	float4 position;
	float4 normal;
	float2 texCoord;
	float4 extras1;
} Vert;

typedef struct __attribute__ ((packed)) NoiseLayer
{
	float octaves;
	float fractal;
	float frequency;
	float lacunarity;
	float gain;
	float weightedStrength;
	float pingPongStrength;
	float strength;
	float4 offset;
	float4 value;
} NoiseLayer;

void print_nl(NoiseLayer nl)
{
	printf("Octaves : %f\nFractals : %f\nFrequency : %f\nStrength : %f\n", nl.octaves, nl.fractal, nl.frequency, nl.strength);
}

#endif // DATA_CL