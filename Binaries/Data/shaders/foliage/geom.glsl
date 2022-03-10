#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4 _PV;
uniform mat4 _Model;


out float height;
out float Distance;
out vec4 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out mat3 oTBN;

in DATA
{
	float height;
    vec3 Normal;
    float distance;
	vec2 TexCoord;
} data_in[]; 

const vec4 clipPlane = vec4(0, -1, 0, 5);

void main()
{	
	vec3 n;

	vec3 a  = (_PV * gl_in[0].gl_Position).xyz;
	vec3 b  = (_PV * gl_in[1].gl_Position).xyz;
	vec3 c  = (_PV * gl_in[2].gl_Position).xyz;
	
	vec3 edge0 = b - a;
	vec3 edge1 = c - a;

	vec2 deltaUV0 = data_in[1].TexCoord - data_in[0].TexCoord;
	vec2 deltaUV1 = data_in[2].TexCoord - data_in[0].TexCoord;

	// one over the determinant
	float invDet = 1.0f / (deltaUV0.x * deltaUV1.y - deltaUV1.x * deltaUV0.y);

	vec3 tangent = vec3(invDet * (deltaUV1.y * edge0 - deltaUV0.y * edge1));
	vec3 bitangent = vec3(invDet * (-deltaUV1.x * edge0 + deltaUV0.x * edge1));
	mat4 model = _Model;
	vec3 T = normalize(vec3(model * vec4(tangent, 0.0f)));
	vec3 B = normalize(vec3(model * vec4(bitangent, 0.0f)));
	vec3 N = normalize(vec3(model * vec4(cross(edge1, edge0), 0.0f)));

	mat3 TBN = mat3(T, B, N);
	oTBN = TBN;	


	gl_Position = _PV * gl_in[0].gl_Position;
	//gl_ClipDistance[0] = dot(gl_Position, clipPlane);
	Normal = (_PV * vec4(data_in[0].Normal, 1)).xyz * -1;
	height = data_in[0].height;
	Distance = data_in[0].distance;
	TexCoord = data_in[0].TexCoord;
	FragPos = _PV * gl_in[0].gl_Position;
	EmitVertex();

	gl_Position = _PV * gl_in[1].gl_Position;
	//gl_ClipDistance[0] = dot(gl_Position, clipPlane);
	height = data_in[1].height;
	Distance = data_in[1].distance;
	Normal = vec4(data_in[1].Normal, 1).xyz * -1;
	TexCoord = data_in[1].TexCoord;
	FragPos = _PV * gl_in[1].gl_Position;
	EmitVertex();

	gl_Position =  _PV * gl_in[2].gl_Position;
	//gl_ClipDistance[0] = dot(gl_Position, clipPlane);
	height = data_in[2].height;
	Distance = data_in[2].distance;
	Normal =(_PV * vec4(data_in[2].Normal, 1)).xyz * -1;
	TexCoord = data_in[2].TexCoord;
	FragPos = _PV * gl_in[2].gl_Position;
	EmitVertex();

	EndPrimitive();
} 