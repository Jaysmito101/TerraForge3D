#version 330 core
out vec4 FragColor;

uniform vec3 _LightPosition;
uniform vec3 _LightColor;
uniform vec3 _SeaColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 ClipSpaceCoord;
in float Height;

uniform sampler2D _ReflectionTexture;
uniform sampler2D _RefractionTexture;
uniform sampler2D _DuDvMap;
uniform sampler2D _NormalMap;

uniform float _SeaAlpha;
uniform float _SeaReflectivity;
uniform float _SeaDistScale;
uniform float _SeaDistStrength;
uniform float _SeaLevel;
uniform float _Time;
uniform float _SeaWaveSpeed;


void main()
{	

	vec4 seaCol = vec4(_SeaColor, 0.6f);

	vec2 ndc = (ClipSpaceCoord.xy/ClipSpaceCoord.w) / 2.0 + 0.5;
	vec2 reflectionTexCoord = vec2(ndc.x, -ndc.y);

	vec2 distortion = (texture(_DuDvMap, vec2(TexCoords.x + _Time*_SeaWaveSpeed, TexCoords.y + _Time*_SeaWaveSpeed)*_SeaDistScale).rg * 2 -1.0) * _SeaDistStrength;
	reflectionTexCoord += distortion;

	vec4 texCol = texture(_ReflectionTexture, reflectionTexCoord);

	vec4 normalMapcColor = texture(_NormalMap, reflectionTexCoord);
	vec3 normal = vec3(normalMapcColor.r * 2.0 - 1.0, normalMapcColor.b, normalMapcColor.g * 2.0 - 1.0);

	//vec3 norm = normalize(Normal);
	vec3 norm = mix(normalize(normal), normalize(Normal), 0.5);

	vec3 objectColor = mix(seaCol, texCol, _SeaReflectivity).xyz;
	vec3 lightDir = normalize(_LightPosition - FragPos);
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = diff * _LightColor;
	vec4 result = vec4((vec3(0.2, 0.2, 0.2) + diffuse) * objectColor, _SeaAlpha);


	FragColor = result;
} 