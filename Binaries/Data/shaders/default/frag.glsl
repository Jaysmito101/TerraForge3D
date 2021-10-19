#version 330 core
out vec4 FragColor;

uniform vec3 _LightPosition;
uniform vec3 _LightColor;

uniform sampler2D _Diffuse;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

void main()
{	
	vec3 objectColor = vec3(1, 1, 1);
	objectColor = texture(_Diffuse, TexCoord).xyz;
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(_LightPosition - FragPos);
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = diff * _LightColor;
	vec3 result = (vec3(0.2, 0.2, 0.2) + diffuse) * objectColor;
	FragColor = vec4(result, 1.0);
} 