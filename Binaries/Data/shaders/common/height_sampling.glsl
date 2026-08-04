float SampleHeightBilinear(vec2 texCoord)
{
	vec2 samplePosition = clamp(texCoord, vec2(0.0), vec2(1.0)) * float(u_Resolution - 1);
	ivec2 lower = ivec2(floor(samplePosition));
	ivec2 upper = min(lower + ivec2(1), ivec2(u_Resolution - 1));
	vec2 blend = fract(samplePosition);

	float lowerLeft = imageLoad(u_Heightmap, lower).r;
	float lowerRight = imageLoad(u_Heightmap, ivec2(upper.x, lower.y)).r;
	float upperLeft = imageLoad(u_Heightmap, ivec2(lower.x, upper.y)).r;
	float upperRight = imageLoad(u_Heightmap, upper).r;
	return mix(mix(lowerLeft, lowerRight, blend.x), mix(upperLeft, upperRight, blend.x), blend.y);
}

vec2 SampleHeightGradient(vec2 texCoord)
{
	if (u_Resolution <= 1) return vec2(0.0);

	vec2 texel = vec2(1.0 / float(u_Resolution - 1));
	vec2 offsetX = vec2(texel.x, 0.0);
	vec2 offsetY = vec2(0.0, texel.y);

	float h00 = SampleHeightBilinear(texCoord - offsetX - offsetY);
	float h10 = SampleHeightBilinear(texCoord          - offsetY);
	float h20 = SampleHeightBilinear(texCoord + offsetX - offsetY);
	float h01 = SampleHeightBilinear(texCoord - offsetX);
	float h21 = SampleHeightBilinear(texCoord + offsetX);
	float h02 = SampleHeightBilinear(texCoord - offsetX + offsetY);
	float h12 = SampleHeightBilinear(texCoord          + offsetY);
	float h22 = SampleHeightBilinear(texCoord + offsetX + offsetY);

	float gradientX = (h20 + 2.0 * h21 + h22) - (h00 + 2.0 * h01 + h02);
	float gradientY = (h02 + 2.0 * h12 + h22) - (h00 + 2.0 * h10 + h20);
	return vec2(gradientX / (8.0 * texel.x), gradientY / (8.0 * texel.y));
}
