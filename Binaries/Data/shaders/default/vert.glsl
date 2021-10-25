#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 _Model;

out DATA
{
	float height;
    vec3 FragPos;
    vec3 Normal;
	vec2 TexCoord;
} data_out; 

void main()
{
    gl_Position =  vec4(aPos.x, aPos.y, aPos.z, 1.0);
	data_out.height = aPos.y;
	data_out.FragPos = vec3(aPos.x, aPos.y, aPos.z);
	data_out.Normal = vec3(aNorm.x, aNorm.y, aNorm.z);
	data_out.TexCoord = aTexCoord;
}