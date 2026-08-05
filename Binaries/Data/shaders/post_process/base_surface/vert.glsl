#version 430 core

void main()
{
	const vec2 vertices[3] = vec2[3](
		vec2(-1.0, -1.0),
		vec2( 3.0, -1.0),
		vec2(-1.0,  3.0));
	gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
}
