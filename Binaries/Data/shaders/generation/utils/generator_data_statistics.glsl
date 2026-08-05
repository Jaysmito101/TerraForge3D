#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D FieldData;

layout(std430, binding = 1) buffer StatisticsBuffer
{
	uint statistics[];
};

uniform int u_Resolution;
uniform int u_SampleStride;
uniform int u_Mode;
uniform int u_HasRequestedPercentile;
uniform float u_RequestedPercentile;

const uint HistogramStartIndex = 2u;
const uint PercentileStartIndex = 258u;
const uint RequestedPercentileIndex = 265u;

uint floatToOrderedUint(float value)
{
	uint bits = floatBitsToUint(value);
	return (bits & 0x80000000u) != 0u ? ~bits : bits ^ 0x80000000u;
}

float orderedUintToFloat(uint value)
{
	uint bits = (value & 0x80000000u) != 0u ? value ^ 0x80000000u : ~value;
	return uintBitsToFloat(bits);
}

float percentileValue(float percentile, float minimum, float maximum)
{
	uint total = 0u;
	for (uint bin = 0u; bin < 256u; ++bin) total += statistics[HistogramStartIndex + bin];
	if (total == 0u) return minimum;

	uint target = uint(clamp(percentile, 0.0f, 1.0f) * float(total - 1u));
	uint cumulative = 0u;
	for (uint bin = 0u; bin < 256u; ++bin)
	{
		uint count = statistics[HistogramStartIndex + bin];
		if (cumulative + count > target)
		{
			float normalizedBin = float(bin) / 255.0f;
			return mix(minimum, maximum, normalizedBin);
		}
		cumulative += count;
	}
	return maximum;
}

void main()
{
	if (u_Mode == 2)
	{
		if (gl_GlobalInvocationID.x != 0u || gl_GlobalInvocationID.y != 0u) return;
		float minimum = orderedUintToFloat(statistics[0]);
		float maximum = orderedUintToFloat(statistics[1]);
		const float standardPercentiles[7] = float[7](0.01f, 0.02f, 0.05f, 0.50f, 0.95f, 0.98f, 0.99f);
		for (uint index = 0u; index < 7u; ++index)
			statistics[PercentileStartIndex + index] = floatBitsToUint(percentileValue(standardPercentiles[index], minimum, maximum));
		if (u_HasRequestedPercentile != 0)
			statistics[RequestedPercentileIndex] = floatBitsToUint(percentileValue(u_RequestedPercentile, minimum, maximum));
		return;
	}

	uvec2 sampleCoord = gl_GlobalInvocationID.xy * uint(max(u_SampleStride, 1));
	if (sampleCoord.x >= uint(u_Resolution) || sampleCoord.y >= uint(u_Resolution)) return;

	float value = imageLoad(FieldData, ivec2(sampleCoord)).r;
	if (value != value) return;

	if (u_Mode == 0)
	{
		uint ordered = floatToOrderedUint(value);
		atomicMin(statistics[0], ordered);
		atomicMax(statistics[1], ordered);
		return;
	}

	float minimum = orderedUintToFloat(statistics[0]);
	float maximum = orderedUintToFloat(statistics[1]);
	float range = maximum - minimum;
	float normalized = range > 0.0000001f ? clamp((value - minimum) / range, 0.0f, 1.0f) : 0.5f;
	uint bin = min(uint(normalized * 256.0f), 255u);
	atomicAdd(statistics[HistogramStartIndex + bin], 1u);
}
