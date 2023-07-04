#version 430 core

// work group size
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// output data buffer
layout (std430, binding = 0) buffer DataSourceBuffer
{
	float dataSource[];
};

layout (std430, binding = 1) buffer DataTargetBuffer
{
	float dataTarget[];
};

// uniforms
uniform int u_Resolution;
uniform int u_Seed;
uniform bool u_UseSeedTexture;
uniform sampler2D u_SeedTexture;
uniform float u_Strength;
uniform float u_Influence;
uniform float u_Frequency;
uniform float u_Lacunarity;
uniform float u_Persistence;
uniform int u_MixMethod;
uniform int u_TransformFactor;
uniform vec3 u_Offset;
uniform float u_NoiseOctaveStrengths[16];
uniform int u_NoiseOctaveStrengthsCount;

/* discontinuous pseudorandom uniformly distributed in [-0.5, +0.5]^3 */
vec3 random3(vec3 c) 
{
	float j = 4096.0*sin(dot(c,vec3(17.0, 59.4, 15.0)));
	vec3 r;
	r.z = fract(512.0*j);
	j *= .125;
	r.x = fract(512.0*j);
	j *= .125;
	r.y = fract(512.0*j);
	return r-0.5;
}

/* skew constants for 3d simplex functions */
const float F3 =  0.3333333;
const float G3 =  0.1666667;
// this is the 2d rotation matrix 
// const mat2 rotationMat = mat2(cos(30.0), -sin(30.0), sin(30.0), cos(30.0));
const mat2 rotationMat = mat2(0.86602540378, -0.5, 0.5, 0.86602540378);

/* 3d simplex noise */
float simplex3d(vec3 p) 
{
	 /* 1. find current tetrahedron T and it's four vertices */
	 /* s, s+i1, s+i2, s+1.0 - absolute skewed (integer) coordinates of T vertices */
	 /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
	 
	 /* calculate s and x */
	 vec3 s = floor(p + dot(p, vec3(F3)));
	 vec3 x = p - s + dot(s, vec3(G3));
	 
	 /* calculate i1 and i2 */
	 vec3 e = step(vec3(0.0), x - x.yzx);
	 vec3 i1 = e*(1.0 - e.zxy);
	 vec3 i2 = 1.0 - e.zxy*(1.0 - e);
	 	
	 /* x1, x2, x3 */
	 vec3 x1 = x - i1 + G3;
	 vec3 x2 = x - i2 + 2.0*G3;
	 vec3 x3 = x - 1.0 + 3.0*G3;
	 
	 /* 2. find four surflets and store them in d */
	 vec4 w, d;
	 
	 /* calculate surflet weights */
	 w.x = dot(x, x);
	 w.y = dot(x1, x1);
	 w.z = dot(x2, x2);
	 w.w = dot(x3, x3);
	 
	 /* w fades from 0.6 at the center of the surflet to 0.0 at the margin */
	 w = max(0.6 - w, 0.0);
	 
	 /* calculate surflet components */
	 d.x = dot(random3(s), x);
	 d.y = dot(random3(s + i1), x1);
	 d.z = dot(random3(s + i2), x2);
	 d.w = dot(random3(s + 1.0), x3);
	 
	 /* multiply d by w^4 */
	 w *= w;
	 w *= w;
	 d *= w;
	 
	 /* 3. return the sum of the four surflets */
	 return dot(d, vec4(52.0));
}

uint PixelCoordToDataOffset(uint x, uint y)
{
	return y * u_Resolution + x;
}

float calculateSlopeFactor()
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;

	if ( offsetv2.x == 0) offsetv2.x = 1;
	if ( offsetv2.y == 0) offsetv2.y = 1;
	if ( offsetv2.x == u_Resolution - 1) offsetv2.x = u_Resolution - 2;
	if ( offsetv2.y == u_Resolution - 1) offsetv2.y = u_Resolution - 2;


	float C = dataSource[PixelCoordToDataOffset(offsetv2.x, offsetv2.y)];
	float T = dataSource[PixelCoordToDataOffset(offsetv2.x, offsetv2.y - 1)];
	float B = dataSource[PixelCoordToDataOffset(offsetv2.x, offsetv2.y + 1)];
	float L = dataSource[PixelCoordToDataOffset(offsetv2.x - 1, offsetv2.y)];
	float R = dataSource[PixelCoordToDataOffset(offsetv2.x + 1, offsetv2.y)];

	float slopeFactor = 0.0f;

	// calculate the slope factor

	
    // calculate the slope factor

	float dX = (R - L) / 2.0f;
	float dY = (B - T) / 2.0f;
	slopeFactor = sqrt(dX * dX + dY * dY);

	return slopeFactor * u_Resolution * 0.4;
}


void main(void)
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
	vec2 uv = offsetv2 / float(u_Resolution);
	vec3 seed = vec3(uv * 2.0f - vec2(1.0f), 0.0f);
	if (u_UseSeedTexture)
	{
		seed = texture(u_SeedTexture, uv).rgb; 
	}
	seed = seed * u_Frequency + u_Offset + vec3(u_Seed);

	float n = 0.0f;
	float frequency = 1.0f;
	float amplitude = 1.0f;
	for (int i = 0 ; i < u_NoiseOctaveStrengthsCount ; i++)
	{
		n += simplex3d(seed * frequency) * amplitude * u_NoiseOctaveStrengths[i];
		frequency *= u_Lacunarity;
		amplitude *= u_Persistence;
	}


	if ( u_TransformFactor == 1) n = n * calculateSlopeFactor();
	else if ( u_TransformFactor == 2) n = clamp(dataSource[offset] * n, 0, 1);

	n = n * u_Strength * u_Influence;

	if ( u_MixMethod == 0 ) dataTarget[offset] = dataSource[offset] + n;
	else if ( u_MixMethod == 1 ) dataTarget[offset] = dataSource[offset] * n;
	else if ( u_MixMethod == 2 ) dataTarget[offset] = dataSource[offset] * n + n;
	else dataTarget[offset] = dataSource[offset];

}

