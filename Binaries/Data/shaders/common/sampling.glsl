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

vec2 tf3d_sampleHammersley(uint i, uint sampleCount)
{
	return vec2(float(i) / float(sampleCount), tf3d_radicalInverseVdc(i));
}

vec3 tf3d_sampleHemisphere(float u1, float u2)
{
	float r = sqrt(max(0.0, 1.0 - u1 * u1));
	return vec3(cos(TF3D_TWO_PI * u2) * r, sin(TF3D_TWO_PI * u2) * r, u1);
}
