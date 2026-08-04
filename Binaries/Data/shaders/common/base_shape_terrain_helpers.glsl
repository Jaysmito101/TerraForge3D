vec2 tf3d_terrain_hash22(vec2 p)
{
    p = vec2(
        dot(p, vec2(127.1f, 311.7f)),
        dot(p, vec2(269.5f, 183.3f)));
    return fract(sin(p) * 43758.5453123f);
}

float tf3d_terrain_fbm2(
    vec2 p,
    float scale,
    int octaves,
    float lacunarity,
    float persistence)
{
    int octaveCount = clamp(octaves, 1, 16);
    float frequency = tf3d_shape_positive(scale, 0.001f);
    float safeLacunarity = clamp(lacunarity, 1.0f, 4.0f);
    float safePersistence = clamp(persistence, 0.0f, 0.95f);
    float value = 0.0f;
    float amplitude = 1.0f;
    float amplitudeSum = 0.0f;
    vec2 domain = p * frequency;
    const mat2 octaveRotation = mat2(0.8f, -0.6f, 0.6f, 0.8f);

    for (int i = 0; i < octaveCount; ++i)
    {
        value += tf3d_snoise2(domain) * amplitude;
        amplitudeSum += amplitude;
        domain = octaveRotation * domain * safeLacunarity + vec2(17.13f, 9.71f);
        amplitude *= safePersistence;
    }

    return value / max(amplitudeSum, TF3D_SHAPE_EPSILON);
}

float tf3d_terrain_ridge2(
    vec2 p,
    float scale,
    int octaves,
    float lacunarity,
    float persistence)
{
    int octaveCount = clamp(octaves, 1, 16);
    float frequency = tf3d_shape_positive(scale, 0.001f);
    float safeLacunarity = clamp(lacunarity, 1.0f, 4.0f);
    float safePersistence = clamp(persistence, 0.0f, 0.95f);
    float value = 0.0f;
    float amplitude = 1.0f;
    float amplitudeSum = 0.0f;
    vec2 domain = p * frequency;
    const mat2 octaveRotation = mat2(0.8f, -0.6f, 0.6f, 0.8f);

    for (int i = 0; i < octaveCount; ++i)
    {
        float ridge = 1.0f - abs(clamp(tf3d_snoise2(domain), -1.0f, 1.0f));
        value += smoothstep(0.0f, 1.0f, ridge) * amplitude;
        amplitudeSum += amplitude;
        domain = octaveRotation * domain * safeLacunarity + vec2(17.13f, 9.71f);
        amplitude *= safePersistence;
    }

    return value / max(amplitudeSum, TF3D_SHAPE_EPSILON);
}
