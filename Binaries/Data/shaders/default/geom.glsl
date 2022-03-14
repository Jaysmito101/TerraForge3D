#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4 _PV;

uniform float _FlatShade;

out float height;
out float Distance;
out vec4 FragPos;
out vec3 Normal;
out vec2 TexCoord;

in DATA
{
	float height;
    vec3 FragPos;
    vec3 Normal;
    float distance;
	vec2 TexCoord;
} data_in[]; 

const vec4 clipPlane = vec4(0, -1, 0, 5);

void main()
{	
	vec3 n;

	if(_FlatShade > 0.5f)
	{	
	vec3 a  = (_PV * gl_in[0].gl_Position).xyz;
	vec3 b  = (_PV * gl_in[1].gl_Position).xyz;
	vec3 c  = (_PV * gl_in[2].gl_Position).xyz;
	
	vec3 ab = b - a;
	vec3 ac = c - a;
	
	n = normalize(cross(ab,  ac));
	}
	
	gl_Position = _PV * gl_in[0].gl_Position;
	//gl_ClipDistance[0] = dot(gl_Position, clipPlane);
	if(_FlatShade > 0.5f)
		Normal = n;
	else
		Normal = data_in[0].Normal;
	height = data_in[0].height;
	Distance = data_in[0].distance;
	TexCoord = data_in[0].TexCoord;
	FragPos = vec4(data_in[0].FragPos, 1.0f);
	EmitVertex();

	gl_Position = _PV * gl_in[1].gl_Position;
	//gl_ClipDistance[0] = dot(gl_Position, clipPlane);
	height = data_in[1].height;
	Distance = data_in[1].distance;
	if(_FlatShade > 0.5f)
		Normal = n;
	else
		Normal = data_in[1].Normal;
	TexCoord = data_in[1].TexCoord;
	FragPos = vec4(data_in[1].FragPos, 1.0f);
	EmitVertex();

	gl_Position =  _PV * gl_in[2].gl_Position;
	//gl_ClipDistance[0] = dot(gl_Position, clipPlane);
	height = data_in[2].height;
	Distance = data_in[2].distance;
	if(_FlatShade > 0.5f)
		Normal = n;
	else
		Normal = data_in[1].Normal;
	TexCoord = data_in[2].TexCoord;
	FragPos = vec4(data_in[2].FragPos, 1.0f);
	EmitVertex();

	EndPrimitive();
} 
