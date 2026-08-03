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

    for (int i = 0; i < octaveCount; ++i)
    {
        value += tf3d_snoise2(p * frequency) * amplitude;
        amplitudeSum += amplitude;
        frequency *= safeLacunarity;
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

    for (int i = 0; i < octaveCount; ++i)
    {
        float ridge = 1.0f - abs(clamp(tf3d_snoise2(p * frequency), -1.0f, 1.0f));
        value += smoothstep(0.0f, 1.0f, ridge) * amplitude;
        amplitudeSum += amplitude;
        frequency *= safeLacunarity;
        amplitude *= safePersistence;
    }

    return value / max(amplitudeSum, TF3D_SHAPE_EPSILON);
}
