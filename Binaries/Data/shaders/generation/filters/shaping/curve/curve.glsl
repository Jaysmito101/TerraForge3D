#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

const int TF3D_CURVE_MAX_POINTS = 16;

uniform vec2 u_Curve[TF3D_CURVE_MAX_POINTS];
uniform int u_CurvePointCount;
uniform float u_Softness;

float evaluateCurve(float inputValue)
{
	int pointCount = clamp(u_CurvePointCount, 2, TF3D_CURVE_MAX_POINTS);
	if (inputValue <= u_Curve[0].x) return u_Curve[0].y;

	for (int pointIndex = 1; pointIndex < TF3D_CURVE_MAX_POINTS; ++pointIndex)
	{
		if (pointIndex >= pointCount) break;

		vec2 leftPoint = u_Curve[pointIndex - 1];
		vec2 rightPoint = u_Curve[pointIndex];
		if (inputValue <= rightPoint.x)
		{
			float segmentWidth = rightPoint.x - leftPoint.x;
			if (segmentWidth <= 0.000001f) return rightPoint.y;
			float segmentPosition = clamp((inputValue - leftPoint.x) / segmentWidth, 0.0f, 1.0f);
			float smootherPosition = segmentPosition * segmentPosition * segmentPosition
				* (segmentPosition * (segmentPosition * 6.0f - 15.0f) + 10.0f);
			float softness = clamp(u_Softness, 0.0f, 1.0f);
			float softnessInfluence = clamp(softness * 1.5f, 0.0f, 1.0f);
			segmentPosition = mix(segmentPosition, smootherPosition, softnessInfluence);
			return mix(leftPoint.y, rightPoint.y, segmentPosition);
		}
	}

	return u_Curve[pointCount - 1].y;
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	float normalizedInput = tf3dFieldNormalize(sampleInput(coordinate));
	float normalizedOutput = clamp(evaluateCurve(normalizedInput), 0.0f, 1.0f);
	writeOutput(coordinate, tf3dFieldDenormalize(normalizedOutput));
}
