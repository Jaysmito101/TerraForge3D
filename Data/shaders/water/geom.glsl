#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

out vec3 FragPos;
out vec3 Normal;

in DATA
{
    vec3  FragPos;
    vec3 Normal;
	mat4 PV;
} data_in[]; 

void main()
{	
	gl_Position = data_in[0].PV * gl_in[0].gl_Position;
	Normal = data_in[0].Normal;
	FragPos = data_in[0].FragPos;
	EmitVertex();

	gl_Position = data_in[0].PV * gl_in[1].gl_Position;
	Normal = data_in[1].Normal;
	FragPos = data_in[1].FragPos;
	EmitVertex();

	gl_Position =  data_in[0].PV * gl_in[2].gl_Position;
	Normal = data_in[2].Normal;
	FragPos = data_in[2].FragPos;
	EmitVertex();

	EndPrimitive();
} 