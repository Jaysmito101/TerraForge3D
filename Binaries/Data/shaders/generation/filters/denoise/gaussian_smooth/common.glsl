layout(std430, binding = 0) readonly buffer InputData
{
	float inputData[];
};

layout(std430, binding = 2) writeonly buffer OutputData
{
	float outputData[];
};

uniform int u_Resolution;

uint indexOf(ivec2 coordinate)
{
	return uint(coordinate.y * u_Resolution + coordinate.x);
}

float sampleInput(ivec2 coordinate)
{
	coordinate = clamp(coordinate, ivec2(0), ivec2(u_Resolution - 1));
	return inputData[indexOf(coordinate)];
}
