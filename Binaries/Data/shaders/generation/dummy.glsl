#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

uniform int r;
uniform int i;
uniform int j;
uniform float a;
uniform float b;
uniform float c;

layout(std430, binding = 0) buffer DataBuffer0
{
    vec4 data0[];
};

uint PixelCoordToDataOffset(uint x, uint y)
{
	return y * r + x;
}

vec2 hash( vec2 p ) // replace this by something better
{
	p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise2( in vec2 p )
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

	vec2  i = floor( p + (p.x+p.y)*K1 );
    vec2  a = p - i + (i.x+i.y)*K2;
    float m = step(a.y,a.x); 
    vec2  o = vec2(m,1.0-m);
    vec2  b = a - o + K2;
	vec2  c = a - 1.0 + 2.0*K2;
    vec3  h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
	vec3  n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
    return dot( n, vec3(70.0) );
}

float noise(in vec2 p)
{
	return noise2(vec2(noise2(p.xy), noise2(p.yx)));
}

void main(void)
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
	vec2 uv = offsetv2 / vec2(r, r);
	vec4 color = data0[offset];
	color.x = 0.0f;
	color.x += noise(uv * 1.0f) * a / 1.0f;
	color.x += noise(uv * 2.0f) * b / 2.0f;
	color.x += noise(uv * 4.0f) * c / 4.0f;
	for(int i = 3 ; i < 32 ; i++)
	{
		color.x += noise(uv * pow(2.0f, i)) / pow(2.0f, i);
	}
	data0[offset] = color;
}
