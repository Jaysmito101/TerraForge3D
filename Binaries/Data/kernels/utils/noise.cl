#include "utils/data.cl"

#ifndef NOISE_CL
#define NOISE_CL


float fractf(float f){return f - floor(f);}

float3 fractf3(float3 f){return f - floor(f);}

float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

float hash( float n )
{
    return fractf(sin(n)*43758.5453f);
}

float noise( float3 x )
{
    // The noise function returns a value in the range -1.0f -> 1.0f

    float3 p = floor(x);
    float3 f = fractf3(x);

    f       = f*f*(3.0f-2.0f*f);
    float n = p.x + p.y*57.0f + 113.0f*p.z;

    return lerp(lerp(lerp( hash(n+0.0f), hash(n+1.0f),f.x),
                   lerp( hash(n+57.0f), hash(n+58.0f),f.x),f.y),
               lerp(lerp( hash(n+113.0f), hash(n+114.0f),f.x),
                   lerp( hash(n+170.0f), hash(n+171.0f),f.x),f.y),f.z);
}

float pingpong(float t)
{
	t -= (int)(t * 0.5f) * 2.0f;
	return t < 1 ? t : 2 - t;
}

float get_simple_noise(NoiseLayer nl){
	return noise(nl.value.xyz * nl.frequency + nl.offset.xyz) * nl.strength;
}

float get_fbm_noise(NoiseLayer nl){
	float amp =  1.0f / 1.75f;
	float sum = 0.0f;
	float3 val = nl.value.xyz * nl.frequency + nl.offset.xyz;

	for(int i=0;i<nl.octaves;i++)
	{
		float n = noise(val);
		sum += n * amp;
		amp *= lerp(1.0f, (n + 1.0f) * 0.5f, nl.weightedStrength);

		val *= nl.lacunarity;
		amp *= nl.gain;
	}

	return sum * nl.strength;
}

float get_ridged_noise(NoiseLayer nl){
	float amp =  1.0f / 1.75f;
	float sum = 0.0f;
	float3 val = nl.value.xyz * nl.frequency + nl.offset.xyz;

	for(int i=0;i<nl.octaves;i++)
	{
		float n = fabs(noise(val));
		sum += (n * -2.0f + 1.0f) * amp;
		amp *= lerp(1.0f, 1.0f - n, nl.weightedStrength);

		val *= nl.lacunarity;
		amp *= nl.gain;
	}

	return sum * nl.strength;	
}

float get_pingpong_noise(NoiseLayer nl){
	float amp =  1.0f / 1.75f;
	float sum = 0.0f;
	float3 val = nl.value.xyz * nl.frequency + nl.offset.xyz;

	for(int i=0;i<nl.octaves;i++)
	{
		float n = pingpong(noise(val) * nl.pingPongStrength);
		sum += (n - 0.5f) * 2.0f * amp;
		amp *= lerp(1.0f, n, nl.weightedStrength);

		val *= nl.lacunarity;
		amp *= nl.gain;
	}

	return sum * nl.strength;
}

float get_noise(NoiseLayer nl)
{
	if(nl.fractal == 0.0f)	
		return get_simple_noise(nl);
	else if (nl.fractal == 1.0f)	
		return get_fbm_noise(nl);
	else if (nl.fractal == 2.0f)	
		return get_ridged_noise(nl);
	else if (nl.fractal == 3.0f)	
		return get_pingpong_noise(nl);
	else
		return 0.0f;
}


#endif