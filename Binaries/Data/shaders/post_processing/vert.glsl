#version 330 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aNorm; 
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aExtras1;

out vec2 TexCoords;

uniform mat4 _PV;

vec2 mAdd = vec2(0.5, 0.5);

void main()
{
    TexCoords = aPos.xy * mAdd + mAdd;
    gl_Position = _PV * vec4(aPos.xyz, 1.0);
}  