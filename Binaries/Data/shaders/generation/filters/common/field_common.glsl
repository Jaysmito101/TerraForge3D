layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D InputData;
layout(TF3D_FIELD_FORMAT, binding = 2) writeonly uniform image2D OutputData;

uniform int u_Resolution;

uint indexOf(ivec2 coordinate)
{
	return uint(coordinate.y * u_Resolution + coordinate.x);
}

float sampleInput(ivec2 coordinate)
{
	coordinate = clamp(coordinate, ivec2(0), ivec2(u_Resolution - 1));
	return imageLoad(InputData, coordinate).r;
}

void writeOutput(ivec2 coordinate, float value)
{
	imageStore(OutputData, coordinate, vec4(value, 0.0, 0.0, 0.0));
}
