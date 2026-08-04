const float TF3D_NOISE_PI2 = 6.28318530718;

vec2 tf3d_hash2(vec2 p)
{
	p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
	return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float tf3d_hash12(vec2 p, float seed)
{
	vec3 value = fract(vec3(p.xyx + seed) * 0.1031);
	value += dot(value, value.yzx + 33.33);
	return fract((value.x + value.y) * value.z);
}

vec2 tf3d_hash22(vec2 p, float seed)
{
	return vec2(
		tf3d_hash12(p + vec2(17.0, 3.0), seed),
		tf3d_hash12(p + vec2(5.0, 29.0), seed + 19.19));
}

float tf3d_noise_seed_offset(float seed)
{
	return seed * 0.071421;
}

float tf3d_simplex2_raw(vec2 p, float seed)
{
	const float K1 = 0.366025404;
	const float K2 = 0.211324865;
	vec2 seedOffset = vec2(tf3d_noise_seed_offset(seed), seed * 0.11317);
	p += seedOffset;
	vec2 i = floor(p + (p.x + p.y) * K1);
	vec2 a = p - i + (i.x + i.y) * K2;
	float m = step(a.y, a.x);
	vec2 o = vec2(m, 1.0 - m);
	vec2 b = a - o + K2;
	vec2 c = a - 1.0 + 2.0 * K2;
	vec3 h = max(0.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
	vec3 n = h * h * h * h * vec3(
		dot(a, tf3d_hash2(i + vec2(seed))),
		dot(b, tf3d_hash2(i + o + vec2(seed))),
		dot(c, tf3d_hash2(i + 1.0 + vec2(seed))));
	return clamp(dot(n, vec3(70.0)), -1.0, 1.0);
}

float tf3d_value2(vec2 p, float seed)
{
	vec2 cell = floor(p);
	vec2 local = fract(p);
	vec2 smoothLocal = local * local * (3.0 - 2.0 * local);
	float a = tf3d_hash12(cell, seed);
	float b = tf3d_hash12(cell + vec2(1.0, 0.0), seed);
	float c = tf3d_hash12(cell + vec2(0.0, 1.0), seed);
	float d = tf3d_hash12(cell + vec2(1.0, 1.0), seed);
	return 2.0 * mix(mix(a, b, smoothLocal.x), mix(c, d, smoothLocal.x), smoothLocal.y) - 1.0;
}

float tf3d_perlin2(vec2 p, float seed)
{
	vec2 cell = floor(p);
	vec2 local = fract(p);
	vec2 fade = local * local * local * (local * (local * 6.0 - 15.0) + 10.0);
	vec2 seedOffset = vec2(seed * 0.071421, seed * 0.11317);
	vec2 g00 = tf3d_hash2(cell + seedOffset);
	vec2 g10 = tf3d_hash2(cell + vec2(1.0, 0.0) + seedOffset);
	vec2 g01 = tf3d_hash2(cell + vec2(0.0, 1.0) + seedOffset);
	vec2 g11 = tf3d_hash2(cell + vec2(1.0) + seedOffset);
	g00 /= max(length(g00), 0.001);
	g10 /= max(length(g10), 0.001);
	g01 /= max(length(g01), 0.001);
	g11 /= max(length(g11), 0.001);
	float n00 = dot(g00, local);
	float n10 = dot(g10, local - vec2(1.0, 0.0));
	float n01 = dot(g01, local - vec2(0.0, 1.0));
	float n11 = dot(g11, local - vec2(1.0));
	return clamp(2.0 * mix(mix(n00, n10, fade.x), mix(n01, n11, fade.x), fade.y), -1.0, 1.0);
}

vec2 tf3d_feature_point(vec2 cell, float jitter, float seed)
{
	vec2 randomPoint = tf3d_hash22(cell, seed) - vec2(0.5);
	return cell + vec2(0.5) + randomPoint * clamp(jitter, 0.0, 1.0);
}

float tf3d_worley2(vec2 p, float jitter, float seed)
{
	vec2 cell = floor(p);
	float nearestDistance = 1.0e6;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			vec2 feature = tf3d_feature_point(cell + vec2(x, y), jitter, seed);
			nearestDistance = min(nearestDistance, distance(p, feature));
		}
	}
	return 1.0 - 2.0 * clamp(nearestDistance / 0.70710678, 0.0, 1.0);
}

float tf3d_voronoi2(vec2 p, float jitter, float seed)
{
	vec2 cell = floor(p);
	float firstDistance = 1.0e6;
	float secondDistance = 1.0e6;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			vec2 feature = tf3d_feature_point(cell + vec2(x, y), jitter, seed);
			float currentDistance = distance(p, feature);
			if (currentDistance < firstDistance)
			{
				secondDistance = firstDistance;
				firstDistance = currentDistance;
			}
			else if (currentDistance < secondDistance)
			{
				secondDistance = currentDistance;
			}
		}
	}
	float edgeDistance = secondDistance - firstDistance;
	return clamp(1.0 - edgeDistance * 4.0, -1.0, 1.0);
}

float tf3d_gabor2(vec2 p, float jitter, float seed)
{
	vec2 cell = floor(p);
	float sum = 0.0;
	float weightSum = 0.0;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			vec2 neighbour = cell + vec2(x, y);
			vec2 randomPoint = tf3d_hash22(neighbour, seed);
			vec2 feature = neighbour + randomPoint;
			vec2 direction = tf3d_hash2(neighbour + vec2(seed));
			direction /= max(length(direction), 0.001);
			vec2 delta = p - feature;
			float kernel = exp(-dot(delta, delta) / 0.72);
			float frequency = mix(0.55, 1.55, randomPoint.y);
			float phase = randomPoint.x * TF3D_NOISE_PI2;
			sum += kernel * cos(TF3D_NOISE_PI2 * frequency * dot(delta, direction) + phase);
			weightSum += kernel;
		}
	}
	return clamp(sum / max(weightSum, 0.0001) * 1.35, -1.0, 1.0);
}

float tf3d_phasor2(vec2 p, float jitter, float seed)
{
	vec2 phaseCell = floor(p * 0.5);
	vec2 direction = tf3d_hash2(phaseCell + vec2(seed));
	direction /= max(length(direction), 0.001);
	float phase = 0.5 + 0.5 * tf3d_value2(p * (0.45 + jitter * 0.35) + vec2(7.0, 13.0), seed + 31.0);
	float carrier = dot(p, direction) * (0.75 + jitter * 0.75) + phase * 1.7;
	return sin(TF3D_NOISE_PI2 * carrier);
}

float tf3d_noise2(vec2 p, int algorithm, float jitter, float seed)
{
	int selected = clamp(algorithm, TF3D_NOISE_BILLOW, TF3D_NOISE_COUNT - 1);
	float base = 0.0;
	switch (selected)
	{
	case TF3D_NOISE_BILLOW:
		base = abs(tf3d_simplex2_raw(p, seed)) * 2.0 - 1.0;
		break;
	case TF3D_NOISE_SIMPLEX:
		base = tf3d_simplex2_raw(p, seed);
		break;
	case TF3D_NOISE_PERLIN:
		base = tf3d_perlin2(p, seed);
		break;
	case TF3D_NOISE_GABOR:
		base = tf3d_gabor2(p, jitter, seed);
		break;
	case TF3D_NOISE_VALUE:
		base = tf3d_value2(p, seed);
		break;
	case TF3D_NOISE_PHASOR:
		base = tf3d_phasor2(p, jitter, seed);
		break;
	case TF3D_NOISE_RIDGED:
		base = 1.0 - 2.0 * abs(tf3d_simplex2_raw(p, seed));
		break;
	case TF3D_NOISE_VORONOI:
		base = tf3d_voronoi2(p, jitter, seed);
		break;
	case TF3D_NOISE_WHITE:
		base = tf3d_hash12(floor(p), seed) * 2.0 - 1.0;
		break;
	case TF3D_NOISE_WORLEY:
		base = tf3d_worley2(p, jitter, seed);
		break;
	}
	return clamp(base, -1.0, 1.0);
}

float tf3d_snoise2(vec2 p)
{
	vec2 domain = p * max(abs(u_NoiseScale), 0.0001);
	float warp = clamp(abs(u_NoiseWarp), 0.0, 4.0);
	if (warp > 0.0001)
	{
		vec2 warpField = vec2(
			tf3d_noise2(domain * 0.5 + vec2(17.0, 5.0), u_NoiseAlgorithm, u_NoiseJitter, float(u_NoiseSeed) + 13.0),
			tf3d_noise2(domain * 0.5 + vec2(-7.0, 23.0), u_NoiseAlgorithm, u_NoiseJitter, float(u_NoiseSeed) + 37.0));
		domain += warpField * warp;
	}
	return tf3d_noise2(
		domain,
		u_NoiseAlgorithm,
		clamp(u_NoiseJitter, 0.0, 1.0),
		float(u_NoiseSeed));
}

float tf3d_noise2_fbm(
	vec2 p,
	int algorithm,
	float scale,
	float seed,
	int octaves,
	float lacunarity,
	float persistence,
	float warp,
	float jitter)
{
	vec2 domain = p * max(abs(scale), 0.0001);
	float safeWarp = clamp(abs(warp), 0.0, 4.0);
	if (safeWarp > 0.0001)
	{
		vec2 warpField = vec2(
			tf3d_noise2(domain * 0.5 + vec2(17.0, 5.0), algorithm, jitter, seed + 13.0),
			tf3d_noise2(domain * 0.5 + vec2(-7.0, 23.0), algorithm, jitter, seed + 37.0));
		domain += warpField * safeWarp;
	}

	int octaveCount = clamp(octaves, 1, 16);
	float safeLacunarity = clamp(lacunarity, 1.0, 4.0);
	float safePersistence = clamp(persistence, 0.0, 0.99);
	float value = 0.0;
	float amplitude = 1.0;
	float amplitudeSum = 0.0;
	for (int octave = 0; octave < octaveCount; ++octave)
	{
		value += tf3d_noise2(domain, algorithm, jitter, seed + float(octave) * 11.73) * amplitude;
		amplitudeSum += amplitude;
		domain = domain * safeLacunarity + vec2(17.13, 9.71);
		amplitude *= safePersistence;
	}
	return value / max(amplitudeSum, 0.0001);
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
