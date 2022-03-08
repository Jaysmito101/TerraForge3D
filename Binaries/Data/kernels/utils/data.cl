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
	float depth;
	float4 offset;
	float4 value;
} NoiseLayer;

typedef struct __attribute__ ((packed)) WindParticle
{
	float dt;
	float suspension;
	float abrasion;
	float roughness;
	float settling;
	float sediment;
	float height;	
	float index;
	float seed;

	float2 dim;	
	float2 pos;

	float4 pspeed;

	float4 speed;
} WindParticle;

void print_nl(NoiseLayer nl)
{
	printf("Octaves : %f\nFractals : %f\nFrequency : %f\nStrength : %f\n", nl.octaves, nl.fractal, nl.frequency, nl.strength);
}

#endif // DATA_CL