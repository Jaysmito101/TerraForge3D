#version 330 core

layout (location = 0) in vec3 aPos;

layout (location = 1) in vec3 aNorm;

uniform mat4 _PV;
uniform float _Time;

out vec3 FragPos;

out vec3 Normal;

void main()
{
	vec3 pos = vec3(aPos.x, aPos.y +0.5f, aPos.z);
	pos = normalize(pos);
	gl_Position = _PV * vec4(pos, 1.0f);
	FragPos = aPos;
	Normal = aNorm;
}