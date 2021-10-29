#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;


uniform mat4 _P;
uniform mat4 _V;

void main()
{
    TexCoords = aPos*100;
    gl_Position = _P * _V * vec4(aPos*100, 1.0);
}  