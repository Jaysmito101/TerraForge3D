#version 430 core

#include "common/sampling.glsl"

const float PI = 3.141592;
const float TwoPI = 2 * PI;
const float Epsilon = 0.00001;

const uint NumSamples = 64 * 1024;
const float InvNumSamples = 1.0 / float(NumSamples);

uniform samplerCube u_InputTexture;

layout(binding = 0, rgba32f) uniform imageCube outputTexture;


// Calculate normalized sampling direction vector based on current fragment coordinates (gl_GlobalInvocationID.xyz).
// This is essentially "inverse-sampling": we reconstruct what the sampling vector would be if we wanted it to "hit"
// this particular fragment in a cubemap.
// See: OpenGL core profile specs, section 8.13.
vec3 getSamplingVector()
{
    vec2 st = (vec2(gl_GlobalInvocationID.xy) + vec2(0.5))/vec2(imageSize(outputTexture));
    vec2 uv = 2.0 * vec2(st.x, 1.0-st.y) - vec2(1.0);

    vec3 ret;
    // Sadly 'switch' doesn't seem to work, at least on NVIDIA.
    if(gl_GlobalInvocationID.z == 0)      ret = vec3(1.0,  uv.y, -uv.x);
    else if(gl_GlobalInvocationID.z == 1) ret = vec3(-1.0, uv.y,  uv.x);
    else if(gl_GlobalInvocationID.z == 2) ret = vec3(uv.x, 1.0, -uv.y);
    else if(gl_GlobalInvocationID.z == 3) ret = vec3(uv.x, -1.0, uv.y);
    else if(gl_GlobalInvocationID.z == 4) ret = vec3(uv.x, uv.y, 1.0);
    else if(gl_GlobalInvocationID.z == 5) ret = vec3(-uv.x, uv.y, -1.0);
    return normalize(ret);
}

// Compute orthonormal basis for converting from tanget/shading space to world space.
void computeBasisVectors(const vec3 N, out vec3 S, out vec3 T)
{
	// Branchless select non-degenerate T.
	T = cross(N, vec3(0.0, 1.0, 0.0));
	T = mix(cross(N, vec3(1.0, 0.0, 0.0)), T, step(Epsilon, dot(T, T)));

	T = normalize(T);
	S = normalize(cross(N, T));
}

// Convert point from tangent/shading space to world space.
vec3 tangentToWorld(const vec3 v, const vec3 N, const vec3 S, const vec3 T)
{
	return S * v.x + T * v.y + N * v.z;
}

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main(void)
{
	ivec2 outputSize = imageSize(outputTexture);
	if (gl_GlobalInvocationID.x >= uint(outputSize.x) ||
		gl_GlobalInvocationID.y >= uint(outputSize.y)) {
		return;
	}

	vec3 N = getSamplingVector();
	
	vec3 S, T;
	computeBasisVectors(N, S, T);

	// Monte Carlo integration of incoming hemispherical irradiance.
	vec3 irradiance = vec3(0);
	for(uint i=0; i<NumSamples; ++i) {
		vec2 u  = tf3d_sampleHammersley(i, NumSamples);
		vec3 Li = tangentToWorld(tf3d_sampleCosineHemisphere(u.x, u.y), N, S, T);

		// With cosine-weighted sampling, cos(theta) / PDF = PI.
		irradiance += PI * texture(u_InputTexture, Li).rgb;
	}
	irradiance /= vec3(NumSamples);

	imageStore(outputTexture, ivec3(gl_GlobalInvocationID), vec4(irradiance, 1.0));
}
