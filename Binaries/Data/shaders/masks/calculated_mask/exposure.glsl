float evaluate(inout MaskContext context)
{
	vec2 gradient = TerrainGradient(context.coordinate);
	vec2 direction = vec2(cos(radians(u_Direction)), sin(radians(u_Direction)));
	return RangeMask(0.5 + 0.5 * dot(SafeNormalize(gradient), direction));
}
