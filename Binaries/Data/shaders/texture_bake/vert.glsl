#version 430 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aNorm;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aExtras1;

uniform mat4 _Model;
uniform mat4 _PV;

out DATA
{
	float height;
    vec3 FragPos;
    vec3 Normal;
    float distance;
	vec2 TexCoord;
} data_out; 



void main()
{
	gl_Position =  vec4(0, aPos.x, aPos.z, 1.0);
	data_out.height = aExtras1.x;
	data_out.FragPos = vec3(aPos.x, aPos.y, aPos.z);
	data_out.Normal = vec3(aNorm.x, aNorm.y, aNorm.z);
	data_out.TexCoord = aTexCoord;
//	data_out.distance = sqrt(_CameraPos, gl_Position.xyz);
	data_out.distance = 0;
}