#version 430 core

layout (location = 0) in vec4 aPosition;
layout (location = 1) in vec4 aNormal;
layout (location = 2) in vec4 aTexCoord;
layout (location = 3) in vec4 aExtras1;

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
uniform float u_TileSize;
uniform vec2 u_TileOffset;
uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_ProjectionView;

void main()
{
    vec2 texCoord = aTexCoord.xy;
    ivec2 pointCoord = ivec2(texCoord * 0.975f * u_Resolution);
    vec3 position = aPosition.xyz + aNormal.xyz * position_normals[pointCoord.y * u_Resolution + pointCoord.x].x;
    vertexOutput.position = position;
    vertexOutput.normal = aNormal.xyz;
    vertexOutput.texCoord = texCoord;
    gl_Position = u_ProjectionView * vec4(position, 1.0);
}