const float TF3D_SHAPE_EPSILON = 0.00001;

float tf3d_shape_positive(float value, float minimumValue)
{
	return max(abs(value), minimumValue);
}

float tf3d_shape_smoothstep(float edge0, float edge1, float value)
{
	float edgeDistance = abs(edge1 - edge0);
	if (edgeDistance < TF3D_SHAPE_EPSILON)
		return step(edge0, value);

	float low = min(edge0, edge1);
	float high = max(edge0, edge1);
	float transition = smoothstep(low, high, value);
	return edge0 < edge1 ? transition : 1.0 - transition;
}

float tf3d_shape_smin(float a, float b, float blend)
{
	float k = max(abs(blend), TF3D_SHAPE_EPSILON);
	float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
	return mix(b, a, h) - k * h * (1.0 - h);
}

float tf3d_shape_smax(float a, float b, float blend)
{
	float k = max(abs(blend), TF3D_SHAPE_EPSILON);
	float h = clamp(0.5 + 0.5 * (a - b) / k, 0.0, 1.0);
	return mix(b, a, h) + k * h * (1.0 - h);
}
