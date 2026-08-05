#version 430 core

#include "common/sampling.glsl"

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

const float PI = 3.141592653589793;
const float EPSILON = 0.00001;
const uint NumSamples = 512;

layout(binding = 0, rg16f) restrict writeonly uniform image2D outputLut;

float GeometrySchlickGGX(float nDotV, float roughness)
{
	float k = ((roughness + 1.0) * (roughness + 1.0)) * 0.125;
	return nDotV / max(nDotV * (1.0 - k) + k, EPSILON);
}

float GeometrySmith(float nDotV, float nDotL, float roughness)
{
	return GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float alpha = roughness * roughness;
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));
	float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
	vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

vec2 IntegrateBRDF(float nDotV, float roughness)
{
	float safeRoughness = max(roughness, 0.04);
	vec3 V = vec3(sqrt(max(0.0, 1.0 - nDotV * nDotV)), 0.0, nDotV);
	float A = 0.0;
	float B = 0.0;

	for (uint i = 0; i < NumSamples; ++i)
	{
		vec2 Xi = tf3d_sampleHammersley(i, NumSamples);
		vec3 H = ImportanceSampleGGX(Xi, vec3(0.0, 0.0, 1.0), safeRoughness);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);

		float nDotL = max(L.z, 0.0);
		float nDotH = max(H.z, 0.0);
		float vDotH = max(dot(V, H), 0.0);
		if (nDotL > 0.0)
		{
			float G = GeometrySmith(nDotV, nDotL, safeRoughness);
			float GVis = (G * vDotH) / max(nDotH * nDotV, EPSILON);
			float Fc = pow(1.0 - vDotH, 5.0);
			A += (1.0 - Fc) * GVis;
			B += Fc * GVis;
		}
	}

	return vec2(A, B) / float(NumSamples);
}

void main()
{
	ivec2 size = imageSize(outputLut);
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	if (pixel.x >= size.x || pixel.y >= size.y) return;

	vec2 uv = (vec2(pixel) + vec2(0.5)) / vec2(size);
	vec2 brdf = IntegrateBRDF(uv.x, uv.y);
	imageStore(outputLut, pixel, vec4(brdf, 0.0, 1.0));
}
