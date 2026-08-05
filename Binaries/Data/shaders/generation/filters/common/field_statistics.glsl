const uint TF3D_FIELD_STATISTICS_HISTOGRAM_START = 2u;
const uint TF3D_FIELD_STATISTICS_PERCENTILE_START = 258u;
const uint TF3D_FIELD_STATISTICS_REQUESTED_PERCENTILE = 265u;

layout(std430, binding = 4) readonly buffer FieldStatisticsBuffer
{
	uint tf3dFieldStatistics[];
};

uniform bool u_HasFieldHistogram;

float tf3dStatisticsOrderedUintToFloat(uint value)
{
	uint bits = (value & 0x80000000u) != 0u ? value ^ 0x80000000u : ~value;
	return uintBitsToFloat(bits);
}

float tf3dFieldMinimum()
{
	return tf3dStatisticsOrderedUintToFloat(tf3dFieldStatistics[0]);
}

float tf3dFieldMaximum()
{
	return tf3dStatisticsOrderedUintToFloat(tf3dFieldStatistics[1]);
}

float tf3dFieldFullRange()
{
	return max(tf3dFieldMaximum() - tf3dFieldMinimum(), 0.000001f);
}

float tf3dFieldP02()
{
	return uintBitsToFloat(tf3dFieldStatistics[TF3D_FIELD_STATISTICS_PERCENTILE_START + 1u]);
}

float tf3dFieldP98()
{
	return uintBitsToFloat(tf3dFieldStatistics[TF3D_FIELD_STATISTICS_PERCENTILE_START + 5u]);
}

float tf3dFieldScaleRange()
{
	float fullRange = tf3dFieldFullRange();
	if (!u_HasFieldHistogram) return fullRange;
	float robustRange = max(tf3dFieldP98() - tf3dFieldP02(), 0.0f);
	return robustRange > 0.000001f ? robustRange : fullRange;
}

float tf3dFieldNormalize(float value)
{
	return clamp((value - tf3dFieldMinimum()) / tf3dFieldFullRange(), 0.0f, 1.0f);
}

float tf3dFieldDenormalize(float value)
{
	return mix(tf3dFieldMinimum(), tf3dFieldMaximum(), clamp(value, 0.0f, 1.0f));
}

float tf3dFieldRequestedPercentile()
{
	return uintBitsToFloat(tf3dFieldStatistics[TF3D_FIELD_STATISTICS_REQUESTED_PERCENTILE]);
}

uint tf3dFieldHistogramCount(uint bin)
{
	return tf3dFieldStatistics[TF3D_FIELD_STATISTICS_HISTOGRAM_START + min(bin, 255u)];
}
