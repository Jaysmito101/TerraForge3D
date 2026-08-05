#version 430 core

layout (location = 0) in vec4 aPosition;
layout (location = 1) in vec4 aNormal;
layout (location = 2) in vec4 aTexCoord;

out VertexData
{
  vec3 position;
  vec3 basePosition;
  vec3 normal;
  vec4 texCoord;
} vertexOutput;

layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D u_Heightmap;

uniform int u_Resolution;
uniform float u_TileSize;
uniform vec2 u_TileOffset;
uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_ProjectionView;
uniform bool u_PlaneMode;
uniform float u_FieldMinimum;
uniform float u_HeightOffset;
uniform float u_SolidDepth;


int PixelCoordToDataOffset(int x, int y)
{
	return y * u_Resolution + x;
}

#include "common/height_sampling.glsl"

void main()
{
    vec2 texCoord = aTexCoord.xy;
    float height = SampleHeightBilinear(texCoord);
    vec3 basePosition = aPosition.xyz;
    vec3 position = basePosition;
	if (aTexCoord.z > 0.5f)
    {
        if (aTexCoord.w > 0.5f) position.y += height + u_HeightOffset;
		else if (u_PlaneMode) position.y = u_FieldMinimum - u_SolidDepth + u_HeightOffset;
    }
    else
    {
        position += aNormal.xyz * height;
		if (u_PlaneMode) position.y += u_HeightOffset;
    }
    //vec3 position = aPosition.xyz + aNormal.xyz * data0[pointCoord.y * u_Resolution + pointCoord.x].x;
    //vec3 position = aPosition.xyz + aNormal.xyz * sin(pointCoord.y * 0.2);
    vertexOutput.position = position;
	vertexOutput.basePosition = basePosition;
	vertexOutput.normal = aNormal.xyz;
    vertexOutput.texCoord = aTexCoord;
    gl_Position = u_ProjectionView * vec4(position, 1.0);
}
