// Shared 2D simplex-style noise helpers.

vec2 tf3d_hash2(vec2 p)
{
	p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
	return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float tf3d_snoise2(vec2 p)
{
	const float K1 = 0.366025404;
	const float K2 = 0.211324865;
	vec2 i = floor(p + (p.x + p.y) * K1);
	vec2 a = p - i + (i.x + i.y) * K2;
	float m = step(a.y, a.x);
	vec2 o = vec2(m, 1.0 - m);
	vec2 b = a - o + K2;
	vec2 c = a - 1.0 + 2.0 * K2;
	vec3 h = max(0.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
	vec3 n = h * h * h * h * vec3(dot(a, tf3d_hash2(i)), dot(b, tf3d_hash2(i + o)), dot(c, tf3d_hash2(i + 1.0)));
	return dot(n, vec3(70.0));
}

float tf3d_fbm2(vec2 uv)
{
	mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
	float value = 0.5 * tf3d_snoise2(uv); uv = m * uv;
	value += 0.25 * tf3d_snoise2(uv); uv = m * uv;
	value += 0.125 * tf3d_snoise2(uv); uv = m * uv;
	value += 0.0625 * tf3d_snoise2(uv);
	return value;
}
