#version 430 core
/*
	Fills a 3D texture with PERLIN-WORLEY and WORLEY noises to be used
	in the volumetric clouds to gather the base shape
*/

layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout (rgba8, binding = 0) uniform image3D outVolTex;

// =====================================================================================
// Code from Sebastien Hillarie 3d noise generator https://github.com/sebh/TileableVolumeNoise
uniform float frequenceMul[6u] = float[]( 2.0,8.0,14.0,20.0,26.0,32.0 );

//Code from https://github.com/NadirRoGue
//Special thanks https://github.com/NadirRoGue

float hash(int n)
{
	return fract(sin(float(n) + 1.951) * 43758.5453123);
}

float noise(vec3 x)
{
	vec3 p = floor(x);
	vec3 f = fract(x);

	f = f*f*(vec3(3.0) - vec3(2.0) * f);
	float n = p.x + p.y*57.0 + 113.0*p.z;
	return mix(
		mix(
			mix(hash(int(n + 0.0)), hash(int(n + 1.0)), f.x),
			mix(hash(int(n + 57.0)), hash(int(n + 58.0)), f.x),
			f.y),
		mix(
			mix(hash(int(n + 113.0)), hash(int(n + 114.0)), f.x),
			mix(hash(int(n + 170.0)), hash(int(n + 171.0)), f.x),
			f.y),
		f.z);
}

float cells(vec3 p, float cellCount)
{
	vec3 pCell = p * cellCount;
	float d = 1.0e10;
	for (int xo = -1; xo <= 1; xo++)
	{
		for (int yo = -1; yo <= 1; yo++)
		{
			for (int zo = -1; zo <= 1; zo++)
			{
				vec3 tp = floor(pCell) + vec3(xo, yo, zo);

				tp = pCell - tp - noise(mod(tp, cellCount / 1.0));

				d = min(d, dot(tp, tp));
			}
		}
	}
	d = min(d, 1.0);
	d = max(d, 0.0f);

	return d;
}


// From GLM (gtc/noise.hpp & detail/_noise.hpp)
vec4 mod289(vec4 x)
{
	return x - floor(x * vec4(1.0) / vec4(289.0)) * vec4(289.0);
}

vec4 permute(vec4 x)
{
	return mod289(((x * 34.0) + 1.0) * x);
}

vec4 taylorInvSqrt(vec4 r)
{
	return vec4(1.79284291400159) - vec4(0.85373472095314) * r;
}

vec4 fade(vec4 t)
{
	return (t * t * t) * (t * (t * vec4(6) - vec4(15)) + vec4(10));
}

float glmPerlin4D(vec4 Position, vec4 rep)
{
		vec4 Pi0 = mod(floor(Position), rep);	// Integer part for indexing
		vec4 Pi1 = mod(Pi0 + vec4(1), rep);		// Integer part + 1
		//Pi0 = mod(Pi0, vec4(289));
		//Pi1 = mod(Pi1, vec4(289));
		vec4 Pf0 = fract(Position);	// Fractional part for interpolation
		vec4 Pf1 = Pf0 - vec4(1);		// Fractional part - 1.0
		vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
		vec4 iy = vec4(Pi0.y, Pi0.y, Pi1.y, Pi1.y);
		vec4 iz0 = vec4(Pi0.z);
		vec4 iz1 = vec4(Pi1.z);
		vec4 iw0 = vec4(Pi0.w);
		vec4 iw1 = vec4(Pi1.w);

		vec4 ixy = permute(permute(ix) + iy);
		vec4 ixy0 = permute(ixy + iz0);
		vec4 ixy1 = permute(ixy + iz1);
		vec4 ixy00 = permute(ixy0 + iw0);
		vec4 ixy01 = permute(ixy0 + iw1);
		vec4 ixy10 = permute(ixy1 + iw0);
		vec4 ixy11 = permute(ixy1 + iw1);

		vec4 gx00 = ixy00 / vec4(7);
		vec4 gy00 = floor(gx00) / vec4(7);
		vec4 gz00 = floor(gy00) / vec4(6);
		gx00 = fract(gx00) - vec4(0.5);
		gy00 = fract(gy00) - vec4(0.5);
		gz00 = fract(gz00) - vec4(0.5);
		vec4 gw00 = vec4(0.75) - abs(gx00) - abs(gy00) - abs(gz00);
		vec4 sw00 = step(gw00, vec4(0.0));
		gx00 -= sw00 * (step(vec4(0), gx00) - vec4(0.5));
		gy00 -= sw00 * (step(vec4(0), gy00) - vec4(0.5));

		vec4 gx01 = ixy01 / vec4(7);
		vec4 gy01 = floor(gx01) / vec4(7);
		vec4 gz01 = floor(gy01) / vec4(6);
		gx01 = fract(gx01) - vec4(0.5);
		gy01 = fract(gy01) - vec4(0.5);
		gz01 = fract(gz01) - vec4(0.5);
		vec4 gw01 = vec4(0.75) - abs(gx01) - abs(gy01) - abs(gz01);
		vec4 sw01 = step(gw01, vec4(0.0));
		gx01 -= sw01 * (step(vec4(0), gx01) - vec4(0.5));
		gy01 -= sw01 * (step(vec4(0), gy01) - vec4(0.5));

		vec4 gx10 = ixy10 / vec4(7);
		vec4 gy10 = floor(gx10) / vec4(7);
		vec4 gz10 = floor(gy10) / vec4(6);
		gx10 = fract(gx10) - vec4(0.5);
		gy10 = fract(gy10) - vec4(0.5);
		gz10 = fract(gz10) - vec4(0.5);
		vec4 gw10 = vec4(0.75) - abs(gx10) - abs(gy10) - abs(gz10);
		vec4 sw10 = step(gw10, vec4(0));
		gx10 -= sw10 * (step(vec4(0), gx10) - vec4(0.5));
		gy10 -= sw10 * (step(vec4(0), gy10) - vec4(0.5));

		vec4 gx11 = ixy11 / vec4(7);
		vec4 gy11 = floor(gx11) / vec4(7);
		vec4 gz11 = floor(gy11) / vec4(6);
		gx11 = fract(gx11) - vec4(0.5);
		gy11 = fract(gy11) - vec4(0.5);
		gz11 = fract(gz11) - vec4(0.5);
		vec4 gw11 = vec4(0.75) - abs(gx11) - abs(gy11) - abs(gz11);
		vec4 sw11 = step(gw11, vec4(0.0));
		gx11 -= sw11 * (step(vec4(0), gx11) - vec4(0.5));
		gy11 -= sw11 * (step(vec4(0), gy11) - vec4(0.5));

		vec4 g0000 = vec4(gx00.x, gy00.x, gz00.x, gw00.x);
		vec4 g1000 = vec4(gx00.y, gy00.y, gz00.y, gw00.y);
		vec4 g0100 = vec4(gx00.z, gy00.z, gz00.z, gw00.z);
		vec4 g1100 = vec4(gx00.w, gy00.w, gz00.w, gw00.w);
		vec4 g0010 = vec4(gx10.x, gy10.x, gz10.x, gw10.x);
		vec4 g1010 = vec4(gx10.y, gy10.y, gz10.y, gw10.y);
		vec4 g0110 = vec4(gx10.z, gy10.z, gz10.z, gw10.z);
		vec4 g1110 = vec4(gx10.w, gy10.w, gz10.w, gw10.w);
		vec4 g0001 = vec4(gx01.x, gy01.x, gz01.x, gw01.x);
		vec4 g1001 = vec4(gx01.y, gy01.y, gz01.y, gw01.y);
		vec4 g0101 = vec4(gx01.z, gy01.z, gz01.z, gw01.z);
		vec4 g1101 = vec4(gx01.w, gy01.w, gz01.w, gw01.w);
		vec4 g0011 = vec4(gx11.x, gy11.x, gz11.x, gw11.x);
		vec4 g1011 = vec4(gx11.y, gy11.y, gz11.y, gw11.y);
		vec4 g0111 = vec4(gx11.z, gy11.z, gz11.z, gw11.z);
		vec4 g1111 = vec4(gx11.w, gy11.w, gz11.w, gw11.w);

		vec4 norm00 = taylorInvSqrt(vec4(dot(g0000, g0000), dot(g0100, g0100), dot(g1000, g1000), dot(g1100, g1100)));
		g0000 *= norm00.x;
		g0100 *= norm00.y;
		g1000 *= norm00.z;
		g1100 *= norm00.w;

		vec4 norm01 = taylorInvSqrt(vec4(dot(g0001, g0001), dot(g0101, g0101), dot(g1001, g1001), dot(g1101, g1101)));
		g0001 *= norm01.x;
		g0101 *= norm01.y;
		g1001 *= norm01.z;
		g1101 *= norm01.w;

		vec4 norm10 = taylorInvSqrt(vec4(dot(g0010, g0010), dot(g0110, g0110), dot(g1010, g1010), dot(g1110, g1110)));
		g0010 *= norm10.x;
		g0110 *= norm10.y;
		g1010 *= norm10.z;
		g1110 *= norm10.w;

		vec4 norm11 = taylorInvSqrt(vec4(dot(g0011, g0011), dot(g0111, g0111), dot(g1011, g1011), dot(g1111, g1111)));
		g0011 *= norm11.x;
		g0111 *= norm11.y;
		g1011 *= norm11.z;
		g1111 *= norm11.w;

		float n0000 = dot(g0000, Pf0);
		float n1000 = dot(g1000, vec4(Pf1.x, Pf0.y, Pf0.z, Pf0.w));
		float n0100 = dot(g0100, vec4(Pf0.x, Pf1.y, Pf0.z, Pf0.w));
		float n1100 = dot(g1100, vec4(Pf1.x, Pf1.y, Pf0.z, Pf0.w));
		float n0010 = dot(g0010, vec4(Pf0.x, Pf0.y, Pf1.z, Pf0.w));
		float n1010 = dot(g1010, vec4(Pf1.x, Pf0.y, Pf1.z, Pf0.w));
		float n0110 = dot(g0110, vec4(Pf0.x, Pf1.y, Pf1.z, Pf0.w));
		float n1110 = dot(g1110, vec4(Pf1.x, Pf1.y, Pf1.z, Pf0.w));
		float n0001 = dot(g0001, vec4(Pf0.x, Pf0.y, Pf0.z, Pf1.w));
		float n1001 = dot(g1001, vec4(Pf1.x, Pf0.y, Pf0.z, Pf1.w));
		float n0101 = dot(g0101, vec4(Pf0.x, Pf1.y, Pf0.z, Pf1.w));
		float n1101 = dot(g1101, vec4(Pf1.x, Pf1.y, Pf0.z, Pf1.w));
		float n0011 = dot(g0011, vec4(Pf0.x, Pf0.y, Pf1.z, Pf1.w));
		float n1011 = dot(g1011, vec4(Pf1.x, Pf0.y, Pf1.z, Pf1.w));
		float n0111 = dot(g0111, vec4(Pf0.x, Pf1.y, Pf1.z, Pf1.w));
		float n1111 = dot(g1111, Pf1);

		vec4 fade_xyzw = fade(Pf0);
		vec4 n_0w = mix(vec4(n0000, n1000, n0100, n1100), vec4(n0001, n1001, n0101, n1101), fade_xyzw.w);
		vec4 n_1w = mix(vec4(n0010, n1010, n0110, n1110), vec4(n0011, n1011, n0111, n1111), fade_xyzw.w);
		vec4 n_zw = mix(n_0w, n_1w, fade_xyzw.z);
		vec2 n_yzw = mix(vec2(n_zw.x, n_zw.y), vec2(n_zw.z, n_zw.w), fade_xyzw.y);
		float n_xyzw = mix(n_yzw.x, n_yzw.y, fade_xyzw.x);
		return float(2.2) * n_xyzw;
}

float remap(float originalValue, float originalMin, float originalMax, float newMin, float newMax)
{
	return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

// ======================================================================

float worleyNoise3D(vec3 p, float cellCount)
{
	return cells(p, cellCount);
}

float perlinNoise3D(vec3 pIn, float frequency, int octaveCount)
{
	float octaveFrenquencyFactor = 2.0;			// noise frequency factor between octave, forced to 2

	// Compute the sum for each octave
	float sum = 0.0f;
	float weightSum = 0.0f;
	float weight = 0.5f;
	for (int oct = 0; oct < octaveCount; oct++)
	{
		// Perlin vec3 is bugged in GLM on the Z axis :(, black stripes are visible
		// So instead we use 4d Perlin and only use xyz...
		//glm::vec3 p(x * freq, y * freq, z * freq);
		//float val = glm::perlin(p, glm::vec3(freq)) *0.5 + 0.5;

		vec4 p = vec4(pIn.x, pIn.y, pIn.z, 0.0) * vec4(frequency);
		float val = glmPerlin4D(p, vec4(frequency));

		sum += val * weight;
		weightSum += weight;

		weight *= weight;
		frequency *= octaveFrenquencyFactor;
	}

	float noise = (sum / weightSum);// *0.5 + 0.5;;
	noise = min(noise, 1.0f);
	noise = max(noise, 0.0f);
	return noise;
}

vec4 stackable3DNoise(ivec3 pixel)
{
	vec3 coord = vec3(float(pixel.x) / 128.0, float(pixel.y) / 128.0, float(pixel.z) / 128.0);

	// Perlin FBM noise
	int octaveCount = 3;
	float frequency = 8.0;
	float perlinNoise = perlinNoise3D(coord, frequency, octaveCount);

	float PerlinWorleyNoise = 0.0f;
	{
		float cellCount = 4.0;
		float worleyNoise0 = (1.0 - worleyNoise3D(coord, cellCount * frequenceMul[0]));
		float worleyNoise1 = (1.0 - worleyNoise3D(coord, cellCount * frequenceMul[1]));
		float worleyNoise2 = (1.0 - worleyNoise3D(coord, cellCount * frequenceMul[2]));
		float worleyNoise3 = (1.0 - worleyNoise3D(coord, cellCount * frequenceMul[3]));
		float worleyNoise4 = (1.0 - worleyNoise3D(coord, cellCount * frequenceMul[4]));
		float worleyNoise5 = (1.0 - worleyNoise3D(coord, cellCount * frequenceMul[5]));	// half the frequency of texel, we should not go further (with cellCount = 32 and texture size = 64)

		// PerlinWorley noise as described p.101 of GPU Pro 7
		float worleyFBM = worleyNoise0*0.625f + worleyNoise1*0.25f + worleyNoise2*0.125f;

		PerlinWorleyNoise = remap(perlinNoise, 0.0, 1.0, worleyFBM, 1.0);
	}

	float cellCount = 4.0;
	float worleyNoise0 = (1.0 - worleyNoise3D(coord, cellCount * 1.0));
	float worleyNoise1 = (1.0 - worleyNoise3D(coord, cellCount * 2.0));
	float worleyNoise2 = (1.0 - worleyNoise3D(coord, cellCount * 4.0));
	float worleyNoise3 = (1.0 - worleyNoise3D(coord, cellCount * 8.0));
	float worleyNoise4 = (1.0 - worleyNoise3D(coord, cellCount * 16.0));
	//float worleyNoise5 = (1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * 32));	
	//cellCount=2 -> half the frequency of texel, we should not go further (with cellCount = 32 and texture size = 64)

	// Three frequency of Worley FBM noise
	float worleyFBM0 = worleyNoise1*0.625f + worleyNoise2*0.25f + worleyNoise3*0.125f;
	float worleyFBM1 = worleyNoise2*0.625f + worleyNoise3*0.25f + worleyNoise4*0.125f;
	//float worleyFBM2 = worleyNoise3*0.625f + worleyNoise4*0.25f + worleyNoise5*0.125f;
	float worleyFBM2 = worleyNoise3*0.75f + worleyNoise4*0.25f; 
	// cellCount=4 -> worleyNoise5 is just noise due to sampling frequency=texel frequency. So only take into account 2 frequencies for FBM

	return vec4(PerlinWorleyNoise * PerlinWorleyNoise, worleyFBM0, worleyFBM1, worleyFBM2);
}

void main()
{
    ivec3 pixel = ivec3(gl_GlobalInvocationID.xyz);

	imageStore (outVolTex, pixel, stackable3DNoise(pixel));
}