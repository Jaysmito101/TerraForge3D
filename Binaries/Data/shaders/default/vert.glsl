#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 _Model;

out DATA
{

    vec3 FragPos;
    vec3 Normal;
	vec2 TexCoord;
} data_out; 

void main()
{
    gl_Position =  vec4(aPos.x, aPos.y, aPos.z, 1.0);
	data_out.FragPos = aPos;
	data_out.Normal = aNorm;
	data_out.TexCoord = aTexCoord;
}