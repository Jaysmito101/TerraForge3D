#version 430 core

#include "Core/Extensions.glsl"
#include "Core/Core.glsl"

DEFINE_FRAGMENT_OUTPUT(0, vec4, FragColor)
DEFINE_FRAGMENT_OUTPUT(1, int, MousePickID)

DEFINE_FRAGMENT_INPUT(0, vec3, Position)
DEFINE_FRAGMENT_INPUT(1, vec3, Normal)

BEGIN_UNIFORMS()
	DEFINE_UNIFORM(ivec4, _Engine)
	DEFINE_UNIFORM(mat4, _Model)
	DEFINE_UNIFORM(mat4, _PV)
	DEFINE_UNIFORM(vec4, _Resolution)
END_UNIFORMS()

#ifdef TF3D_OPENGL
uniform int _MousePickID = 0;
#endif // TF3D_OPENGL

void main()
{
	vec3 lightPos = vec3(0.0, 1.5f, 0.0f);
	vec3 lightColor = vec3(0.8f);
	vec3 objectColor = vec3(0.8f);

	// ambient
	float ambientStrength = 0.1f;
	vec3 ambient = ambientStrength * lightColor;
  	
	// diffuse 
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - Position);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor; 

	FragColor = vec4(vec3(diff), 1.0f);
	MousePickID = GET_MOUSE_PICK_ID();
}