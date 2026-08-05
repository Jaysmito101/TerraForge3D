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
uniform int u_SubTileSize;
uniform float u_TileSize;
uniform vec2 u_TileOffset;
uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_ProjectionView;


int PixelCoordToDataOffset(int x, int y)
{
    return y * u_Resolution + x;
}

void main()
{
    vec2 texCoord = aTexCoord.xy;
    ivec2 pointCoord = ivec2(texCoord * 0.975f * u_Resolution);
    vec3 position = aPosition.xyz;
    if (aTexCoord.z > 0.5f)
    {
        if (aTexCoord.w > 0.5f) position.y += imageLoad(u_Heightmap, pointCoord).r;
    }
    else
    {
        position += aNormal.xyz * imageLoad(u_Heightmap, pointCoord).r;
    }
    //vec3 position = aPosition.xyz + aNormal.xyz * position_normals[pointCoord.y * u_Resolution + pointCoord.x].x;
    //vec3 position = aPosition.xyz + aNormal.xyz * sin(pointCoord.y * 0.2);
    vertexOutput.position = position;
    vertexOutput.normal = aNormal.xyz;
    vertexOutput.texCoord = texCoord;
    gl_Position = u_ProjectionView * vec4(position, 1.0);
}
