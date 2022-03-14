#version 430 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aNorm;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aExtras1;

uniform mat4 _Model;
uniform mat4 _PV;
uniform float _TextureBake = 0.0f;
uniform float _Scale = 1.0f;

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
	if(_TextureBake == 0.0f)
	{
		gl_Position =  vec4(aPos.x, aPos.y, aPos.z, 1.0);
		data_out.height = aExtras1.x;
		data_out.FragPos = vec3(aPos.x, aPos.y, aPos.z);
		data_out.Normal = vec3(aNorm.x, aNorm.y, aNorm.z);
		data_out.TexCoord = aTexCoord;
		data_out.distance = 0;
	}
	else
	{
		gl_Position =  vec4(aPos.x/_Scale, aPos.z/_Scale, 0.0f, 1.0);
		data_out.height = aExtras1.x;
		data_out.FragPos = vec3(aPos.x, aPos.y, aPos.z);
		data_out.Normal = vec3(aNorm.x, aNorm.y, aNorm.z);
		data_out.TexCoord = aTexCoord;
		data_out.distance = 0;
	}
}