#version 430 core

uniform mat4 u_MPV;

layout (location=0) in vec4 position;

out vec3 localPosition;

void main()
{
	localPosition = position.xyz;
	gl_Position = u_MPV * vec4(localPosition, 1.0);
}
