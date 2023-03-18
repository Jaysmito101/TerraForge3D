#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

uniform int u_Resolution;
uniform bool u_UseSeedTexture;
uniform sampler2D u_SeedTexture;
uniform float u_Strength;
uniform float u_CrackShapeDistortion;
uniform float u_Scale;
uniform float u_Smoothness;
uniform float u_RandomHeights;
uniform float u_NoiseStrength;
uniform float u_NoiseScale;
uniform int u_Seed;
uniform vec3 u_Offset;
uniform bool u_SquareValue;
uniform bool u_AbsoluteValue;
uniform vec2 u_MinMaxHeight;

layout(std430, binding = 0) buffer DataBuffer
{
    float data[];
};

uint PixelCoordToDataOffset(uint x, uint y)
{
	return y * u_Resolution + x;
}

// IQ's polynomial-based smooth minimum function.
float smin( float a, float b, float k ){

    float h = clamp(.5 + .5*(b - a)/k, 0., 1.);
    return mix(b, a, h) - k*h*(1. - h);
}

float smax( float a, float b, float k ){

	float h = clamp(.5 + .5*(a - b)/k, 0., 1.);
	return mix(b, a, h) + k*h*(1. - h);
}

/* discontinuous pseudorandom uniformly distributed in [-0.5, +0.5]^3 */
vec3 random3(vec3 c) {
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

/* 3d simplex noise */
float simplex3d(vec3 p) {
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


float rand(float co) { return fract(sin(co*(91.3458)) * 47453.5453); }
float rand(vec2 co){ return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453); }
float rand(vec3 co){ return rand(co.xy+rand(co.z)); }

vec4 voronoi(vec3 sd)
{
	float res = 0.0f;

	//sd.z = 0.0f;

	float bX = floor(sd.x), bY = floor(sd.y), bZ = floor(sd.z);
	// float fX = fract(sd.x), fY = fract(sd.y), fZ = fract(sd.z);
	ivec3 sd0 = ivec3(bX, bY, bZ);
	vec3 nearestPoint = vec3(0.0f);
	float nearestDistance = 999999.0f;
	vec3 d = vec3(999999.0f);

	for (int i = -1 ; i <= 1 ; i++)
	{
		for (int j = -1 ; j <= 1 ; j++)
		{
			//int k = 0;
			for (int k = -1 ; k <= 1 ; k++)
			{
				vec3 sd1 = sd0 + vec3(i, j, k);
				// vec3 sd2 = sd1 + vec3(rand(sd1.xyz), rand(sd1.yzx), rand(sd1.zxy));
				// vec3 sd2 = sd1 + vec3(sin(sd1.x) * u_CrackShapeDistortion, sin(sd1.y) * u_CrackShapeDistortion, sin(sd1.z) * u_CrackShapeDistortion);
				float rnd = rand(sd1);
				vec3 sd2 = sd1 + vec3(sin(fract(rnd * 4658)) * u_CrackShapeDistortion, sin(fract(rnd * 5487)) * u_CrackShapeDistortion, sin(fract(rnd * 3631)) * u_CrackShapeDistortion);
				vec3 sd3 = (sd - sd2);
				float dst = dot(sd3, sd3);
				if(dst < nearestDistance)
				{
					nearestDistance = dst;
					nearestPoint = sd2;
				}

				// 1st, 2nd and 3rd nearest squared distances.
				d.z = max(d.x, max(d.y, min(d.z, dst))); // 3rd.
				d.y = max(d.x, min(d.y, dst)); // 2nd.
	            d.x = min(d.x, dst); // Closest.
			}
		}
	}
	d = sqrt(d);
	nearestDistance = min(2./(1./max(d.y - d.x, .001) + 1./max(d.z - d.x, .001)), 1.);
	return vec4(nearestPoint, nearestDistance);
}

void main(void)
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
	vec2 uv = offsetv2 / float(u_Resolution);
	vec3 seed = vec3(uv * 2.0f - vec2(1.0f), 0.0f);
	if(u_UseSeedTexture)
	{
		seed = texture(u_SeedTexture, uv).rgb;
	}
	seed += u_Offset + vec3(u_Seed);
	seed *= u_Scale;
	float n = 0.0f;

	vec4 voronoiResult = voronoi(seed);
	if(u_AbsoluteValue) voronoiResult.w  = abs(voronoiResult.w);
	if(u_SquareValue) voronoiResult.w  = voronoiResult.w  * voronoiResult.w ;

	float randomHeightFactor = rand(voronoiResult.xyz) * smin(voronoiResult.w * u_Strength, u_MinMaxHeight.y, u_Smoothness);
	float heightFactor = randomHeightFactor * u_RandomHeights;
	n = smin(smax(voronoiResult.w * u_Strength, u_MinMaxHeight.x, u_Smoothness), u_MinMaxHeight.y, u_Smoothness) + heightFactor;
	n += simplex3d(seed * 2.0f * u_NoiseScale) * 0.06f * u_NoiseStrength;
	data[offset] = n;
}
