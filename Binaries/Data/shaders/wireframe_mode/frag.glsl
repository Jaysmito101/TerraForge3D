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

#define MAX_LIGHTS 16
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT	   1

struct RendererLightData
{
	vec3 position;
	vec3 color;
	float intensity;
	int type;
};

uniform RendererLightData u_Lights[MAX_LIGHTS];
uniform int u_LightCount;
uniform int u_Resolution;
uniform int u_SubTileSize;
uniform float u_TileSize;
uniform bool u_InvertNormals;
uniform vec3 u_CameraPosition;

int PixelCoordToDataOffset(int x, int y)
{
	int tileSize = u_SubTileSize;
	int tileCount = u_Resolution / tileSize;
	int tileX = x / tileSize, tileY = y / tileSize;
	int tileXOffset = x % tileSize, tileYOffset = y % tileSize;
	int tileOffset = (tileY * tileCount + tileX) * (tileSize * tileSize);
	return (tileOffset + (tileYOffset * tileSize + tileXOffset));
}

void main()
{
	FragColor = vec4(1.0f);
}