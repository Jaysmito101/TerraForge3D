#version 430 core

out vec4 FragColor;


in VertexData
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
} fragmentInput;

layout(std430, binding = 0) buffer DataBuffer0
{
    vec4 position_normals[];
};


uniform int u_Resolution;
uniform int u_SubTileSize;
uniform float u_TileSize;
uniform float u_HeightmapMin;
uniform float u_HeightmapMax;

int PixelCoordToDataOffset(int x, int y)
{
	return y * u_Resolution + x;
}

float mapInRange(float value, float minV, float maxV, float newMin, float newMax)
{
	return newMin + (value - minV) * (newMax - newMin) / (maxV - minV);
}

void main()
{
    int index = PixelCoordToDataOffset(int(fragmentInput.texCoord.x * u_Resolution), int(fragmentInput.texCoord.y * u_Resolution));
	vec4 position_normal = position_normals[index];	
	FragColor = vec4(vec3(mapInRange(position_normal.x, u_HeightmapMin, u_HeightmapMax, 0.0, 1.0)), 1.0);	
}