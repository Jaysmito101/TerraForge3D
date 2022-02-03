#pragma OPENCL EXTENSION cl_intel_printf : enable

float fractf(float f){return f - floor(f);}

float fractf3(float3 f){return f - floor(f);}

float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

float hash( float n )
{
    return fractf(sin(n)*43758.5453);
}

float noise( float3 x )
{
    // The noise function returns a value in the range -1.0f -> 1.0f

    float3 p = floor(x);
    float3 f = fractf3(x);

    f       = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0 + 113.0*p.z;

    return lerp(lerp(lerp( hash(n+0.0), hash(n+1.0),f.x),
                   lerp( hash(n+57.0), hash(n+58.0),f.x),f.y),
               lerp(lerp( hash(n+113.0), hash(n+114.0),f.x),
                   lerp( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
}

typedef struct __attribute__ ((packed)) Vert
{
	float4 position;
	float4 normal;
	float2 texCoord;
	float4 extras1;
} Vert;



__kernel void erode(__global Vert *verts)
{
	int i;
	i = get_global_id(0);
	float4 tmp = verts[i].position;
	verts[i].position.y = noise(verts[i].texCoord);
}