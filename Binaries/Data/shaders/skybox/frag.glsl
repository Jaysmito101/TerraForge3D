#version 430 core

in vec3 localPosition;

out vec4 FragColor;

uniform samplerCube u_Skybox;

void main()
{
	vec3 envVector = normalize(localPosition);
	FragColor = texture(u_Skybox, envVector);
}
