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

void main()
{
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
	atomicAdd(statistics[2u + bin], 1u);
}
