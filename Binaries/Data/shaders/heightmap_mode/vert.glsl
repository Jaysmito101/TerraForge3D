#version 430 core

layout (location = 0) in vec4 aPosition;
layout (location = 1) in vec4 aNormal;
layout (location = 2) in vec4 aTexCoord;

out VertexData
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
} vertexOutput;

layout(std430, binding = 0) buffer DataBuffer0
{
    vec4 position_normals[];
};

uniform int u_Resolution;
uniform int u_SubTileSize;
uniform float u_TileSize;
uniform vec2 u_TileOffset;
uniform vec2 u_Offset;
uniform float u_AspectRatio;
uniform float u_Scale;

int PixelCoordToDataOffset(int x, int y)
{
	return y * u_Resolution + x;
}

void main()
{
    vec2 texCoord = aTexCoord.xy;
    ivec2 pointCoord = ivec2(texCoord * 0.975f * u_Resolution);
    vertexOutput.texCoord = texCoord;
	vec3 position = vec3(1.0f);
	if(u_AspectRatio > 1.0f) position = aPosition.xyz * vec3(1/u_AspectRatio, 1, 1);
	else position = aPosition.xyz * vec3(1, u_AspectRatio, 1);
	gl_Position = vec4((position - vec3(u_Offset.x, u_Offset.y, 0)) * u_Scale, 1);
}