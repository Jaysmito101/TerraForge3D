#version 430 core
out vec4 FragColor;


uniform vec3 _LightPosition;
uniform vec3 _LightColor;
uniform float _TextureBake = 0.0f;
uniform float _Mode = 0.0f;
uniform vec3 _HMapMinMax;

in float height;
in float Distance;
in vec4 FragPos;
in vec3 Normal;
in vec2 TexCoord;

void main()
{	

	vec3 objectColor = vec3(1, 1, 1);	

	if(_TextureBake == 1.0f)
	{
		if(_Mode == 0.0f)
			objectColor = vec3(1.0);
		else if(_Mode == 1.0f)
			objectColor = vec3( (height - _HMapMinMax.x) / (_HMapMinMax.y - _HMapMinMax.x) );
		else if(_Mode == 2.0f)
			objectColor = Normal;
		FragColor = vec4(objectColor, 1.0f);
		return;
	}

	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(_LightPosition - FragPos.xyz );
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = diff * _LightColor;
	vec3 result = (vec3(0.2, 0.2, 0.2) + diffuse) * objectColor;

	FragColor = vec4(result, 1.0f);
} 