#version 330 core
out vec4 FragColor;

uniform vec3 _LightPosition;
uniform vec3 _LightColor;
uniform vec3 _SeaColor;

in vec3 FragPos;
in vec3 Normal;

void main()
{	
	//vec3 objectColor = vec3(0.34f, 0.49f, 0.27f);
	//vec3 objectColor = vec3(0.3f, 0.3f, 1.0f);
	vec3 objectColor = _SeaColor;
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(_LightPosition - FragPos);
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = diff * _LightColor;
	vec3 result = (vec3(0.2, 0.2, 0.2) + diffuse) * objectColor;
	FragColor = vec4(result, 0.6f);
} 