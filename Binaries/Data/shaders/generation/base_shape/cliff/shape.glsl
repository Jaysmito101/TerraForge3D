#include "common/noise_2d.glsl"
#include "common/base_shape_helpers.glsl"

vec2 rotate(vec2 v, float a) 
{
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

float noise(vec2 uv)
{
	mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
	float f  = 0.5000f * tf3d_snoise2( uv ); uv = m*uv;
	f += 0.2500f * tf3d_snoise2( uv ); uv = m*uv;
	f += 0.1250f * tf3d_snoise2( uv ); uv = m*uv;
	f += 0.0625f * tf3d_snoise2( uv ); uv = m*uv;
	return f;
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float rotation = 3.14159265f * clamp(u_Rotation, -36000.0f, 36000.0f) / 180.0f;
	uv = 2.0f * uv - vec2(1.0f);
	uv = rotate(uv, rotation);

	float thickness = tf3d_shape_positive(u_Thickness, 0.001f);
	float distortionScale = tf3d_shape_positive(u_DistortionScale, 0.001f);
	float position = clamp(u_Position, -4.0f, 4.0f);
	float distortion = clamp(u_Distortion, 0.0f, 1.0f) * noise(uv * distortionScale);
	float x = (1.0f - uv.x - position - distortion) / thickness;
	float clampedX = clamp(x, 0.0f, 1.0f);
	float ns = pow(0.5f * clampedX * clampedX * clampedX - 1.5f * clampedX, 2.0f);
	ns += noise(uv + vec2(1.0f, 2.0f)) * clamp(u_Noise, 0.0f, 1.0f);
	return ns * clamp(u_Strength, -4.0f, 4.0f);
}
