#version 430 core

out vec4 FragColor;


in VertexData
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
} fragmentInput;

uniform int u_Resolution;
uniform int u_SubTileSize;
uniform float u_TileSize;
uniform bool u_InvertNormals;
uniform vec3 u_CameraPosition;

int PixelCoordToDataOffset(int x, int y)
{
	return y * u_Resolution + x;
}

void main()
{
	FragColor = vec4(1.0f);
}
