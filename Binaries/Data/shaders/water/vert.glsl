#version 330 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aNorm;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aExtras1;

uniform mat4 _PV;
uniform mat4 _Model;

out DATA
{
    vec3  FragPos;
    vec3 Normal;
	vec2 TexCoords;
	mat4 PV;
	float height;
} data_out; 

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
	data_out.FragPos = vec3(aPos.x, aPos.y, aPos.z);
	data_out.Normal = vec3(aNorm.x, aNorm.y, aNorm.z);
	data_out.PV = _PV;
	data_out.height = aPos.y;
	data_out.TexCoords = aTexCoord;
}