#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 ClipSpaceCoord;
out float Height;

in DATA
{
    vec3  FragPos;
    vec3 Normal;
	vec2 TexCoords;
	mat4 PV;
	float height;
} data_in[]; 

void main()
{	
	gl_Position = data_in[0].PV * gl_in[0].gl_Position;
	ClipSpaceCoord = data_in[0].PV * gl_in[0].gl_Position;
	Normal = data_in[0].Normal;
	FragPos = data_in[0].FragPos;
	TexCoords = data_in[0].TexCoords;
	Height = data_in[0].height;
	EmitVertex();

	gl_Position = data_in[1].PV * gl_in[1].gl_Position;
	ClipSpaceCoord = data_in[1].PV * gl_in[1].gl_Position;
	Normal = data_in[1].Normal;
	FragPos = data_in[1].FragPos;
	TexCoords = data_in[1].TexCoords;
	Height = data_in[1].height;
	EmitVertex();

	gl_Position = data_in[2].PV * gl_in[2].gl_Position;
	ClipSpaceCoord = data_in[2].PV * gl_in[2].gl_Position;
	Normal = data_in[2].Normal;
	FragPos = data_in[2].FragPos;
	TexCoords = data_in[2].TexCoords;
	Height = data_in[2].height;
	EmitVertex();

	EndPrimitive();
} 