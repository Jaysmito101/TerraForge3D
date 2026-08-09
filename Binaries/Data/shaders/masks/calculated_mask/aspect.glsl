float evaluate(inout MaskContext context)
{
	vec2 gradient = TerrainGradient(context.coordinate);
	float aspect = mod(degrees(atan(gradient.y, gradient.x)) + 360.0, 360.0);
	return DirectionMask(aspect);
}
