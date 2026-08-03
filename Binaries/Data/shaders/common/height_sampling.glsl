float SampleHeightBilinear(vec2 texCoord)
{
	vec2 samplePosition = clamp(texCoord, vec2(0.0), vec2(1.0)) * float(u_Resolution - 1);
	ivec2 lower = ivec2(floor(samplePosition));
	ivec2 upper = min(lower + ivec2(1), ivec2(u_Resolution - 1));
	vec2 blend = fract(samplePosition);

	float lowerLeft = data0[PixelCoordToDataOffset(lower.x, lower.y)];
	float lowerRight = data0[PixelCoordToDataOffset(upper.x, lower.y)];
	float upperLeft = data0[PixelCoordToDataOffset(lower.x, upper.y)];
	float upperRight = data0[PixelCoordToDataOffset(upper.x, upper.y)];
	return mix(mix(lowerLeft, lowerRight, blend.x), mix(upperLeft, upperRight, blend.x), blend.y);
}
