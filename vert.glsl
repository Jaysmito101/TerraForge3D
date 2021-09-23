
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;

uniform mat4 _PV;

out vec3 FragPos;
out vec3 Normal;

void main()
{
    gl_Position = _PV * vec4(aPos.x, aPos.y, aPos.z, 1.0);
	FragPos = aPos;
	Normal = aNorm;
}

