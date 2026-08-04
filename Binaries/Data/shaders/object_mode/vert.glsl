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

layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D u_Heightmap;

uniform int u_Resolution;
uniform float u_TileSize;
uniform vec2 u_TileOffset;
uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_ProjectionView;


int PixelCoordToDataOffset(int x, int y)
{
	return y * u_Resolution + x;
}

#include "common/height_sampling.glsl"

void main()
{
    vec2 texCoord = aTexCoord.xy;
    vec3 position = aPosition.xyz + aNormal.xyz * SampleHeightBilinear(texCoord);
    //vec3 position = aPosition.xyz + aNormal.xyz * data0[pointCoord.y * u_Resolution + pointCoord.x].x;
    //vec3 position = aPosition.xyz + aNormal.xyz * sin(pointCoord.y * 0.2);
    vertexOutput.position = position;
    vertexOutput.normal = aNormal.xyz;
    vertexOutput.texCoord = texCoord;
    gl_Position = u_ProjectionView * vec4(position, 1.0);
}
