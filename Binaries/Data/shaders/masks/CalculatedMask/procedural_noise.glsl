float evaluate(inout MaskContext context)
{
	float noise = tf3d_noise2_fbm(
		context.uv + u_NoiseOffset.xy, u_NoiseAlgorithm, u_NoiseFrequency, float(u_NoiseSeed), u_NoiseOctaves,
		u_NoiseLacunarity, u_NoisePersistence, u_NoiseWarp, u_NoiseJitter);
	return RangeMask(0.5 + 0.5 * noise);
}
