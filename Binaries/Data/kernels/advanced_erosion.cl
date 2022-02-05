
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
    // The noise function returns a value in the range -1.0ff -> 1.0ff

    float3 p = floor(x);
    float3 f = fractf3(x);

    f       = f*f*(3.0f-2.0f*f);
    float n = p.x + p.y*57.0f + 113.0f*p.z;

    return lerp(lerp(lerp( hash(n+0.0f), hash(n+1.0f),f.x),
                   lerp( hash(n+57.0f), hash(n+58.0f),f.x),f.y),
               lerp(lerp( hash(n+113.0f), hash(n+114.0f),f.x),
                   lerp( hash(n+170.0f), hash(n+171.0f),f.x),f.y),f.z);
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
	verts[i].position.y = noise(tmp.xyz) * 2;
}