float evaluate(inout MaskContext context)
{
	float noise = tf3d_noise2_fbm(
		context.uv + u_Offset.xy, u_NoiseAlgorithm, u_Frequency, float(u_Seed), u_NoiseOctaves,
		u_Lacunarity, u_Persistence, u_NoiseWarp, u_NoiseJitter);
	return RangeMask(0.5 + 0.5 * noise);
}
