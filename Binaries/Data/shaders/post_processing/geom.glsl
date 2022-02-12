#version 430 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

out vec2 TexCoords;

uniform mat4 _PV;

void main() 
{
    gl_Position = _PV * vec4( 1.0, 1.0, 0.0, 1.0 );
    TexCoords = vec2( 1.0, 1.0 );
    EmitVertex();

    gl_Position =_PV * vec4(-1.0, 1.0, 0.0, 1.0 );
    TexCoords = vec2( 0.0, 1.0 ); 
    EmitVertex();

    gl_Position = _PV * vec4( 1.0,-1.0, 0.0, 1.0 );
    TexCoords = vec2( 1.0, 0.0 ); 
    EmitVertex();

    gl_Position = _PV * vec4(-1.0,-1.0, 0.0, 1.0 );
    TexCoords = vec2( 0.0, 0.0 ); 
    EmitVertex();

    EndPrimitive(); 
}