const float TF3D_PI = 3.141592653589793;
const float TF3D_TWO_PI = 2.0 * TF3D_PI;
const float TF3D_EPSILON = 0.00001;

float tf3d_radicalInverseVdc(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

// Sample i-th point from Hammersley point set of NumSamples points total.
vec2 tf3d_sampleHammersley(uint i, uint sampleCount)
{
	return vec2(float(i) / float(sampleCount), tf3d_radicalInverseVdc(i));
}

// Cosine-weighted hemisphere sample for Lambertian irradiance integration.
// Its PDF is cos(theta) / PI, so the irradiance estimator is PI * Li.
// See: "Physically Based Rendering" 2nd ed., section 13.6.1.
vec3 tf3d_sampleCosineHemisphere(float u1, float u2)
{
	float radius = sqrt(u1);
	float phi = TF3D_TWO_PI * u2;
	return vec3(cos(phi) * radius, sin(phi) * radius, sqrt(max(0.0, 1.0 - u1)));
}

// Retained for callers that need a uniform hemisphere distribution.
vec3 tf3d_sampleHemisphere(float u1, float u2)
{
	float r = sqrt(max(0.0, 1.0 - u1 * u1));
	return vec3(cos(TF3D_TWO_PI * u2) * r, sin(TF3D_TWO_PI * u2) * r, u1);
}

vec3 tf3d_clampRadiance(vec3 radiance, float maxLuminance)
{
	radiance = max(radiance, vec3(0.0));
	float luminance = dot(radiance, vec3(0.2126, 0.7152, 0.0722));
	return radiance * min(1.0, maxLuminance / max(luminance, TF3D_EPSILON));
}
