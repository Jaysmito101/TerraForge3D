float evaluate(inout MaskContext context)
{
	float radialDistance = distance(context.uv, u_Center);
	float innerRadius = min(u_Range.x, u_Range.y);
	float outerRadius = max(u_Range.x, u_Range.y);
	float radialFeather = clamp(u_EdgeFeather, 0.0, 0.5);
	float radialSoftness = (outerRadius - innerRadius) * radialFeather;
	if (outerRadius - innerRadius <= 0.000001)
		return radialDistance <= outerRadius ? 1.0 : 0.0;
	return 1.0 - smoothstep(innerRadius - radialSoftness, outerRadius + radialSoftness, radialDistance);
}
