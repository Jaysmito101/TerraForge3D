#version 330 core
out vec4 FragColor;

uniform vec3 _LightPosition;
uniform vec3 _LightColor;
uniform vec3 _SeaColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 ClipSpaceCoord;

uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform float _SeaAlpha;
uniform float _SeaDistScale;
uniform float _SeaDistStrength;

float hash(float n) { return fract(sin(n) * 1e4); }
float hash(vec2 p) { return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

float noise(float x) {
	float i = floor(x);
	float f = fract(x);
	float u = f * f * (3.0 - 2.0 * f);
	return mix(hash(i), hash(i + 1.0), u);
}

float noise(vec2 x) {
	vec2 i = floor(x);
	vec2 f = fract(x);

	// Four corners in 2D of a tile
	float a = hash(i);
	float b = hash(i + vec2(1.0, 0.0));
	float c = hash(i + vec2(0.0, 1.0));
	float d = hash(i + vec2(1.0, 1.0));

	// Simple 2D lerp using smoothstep envelope between the values.
	// return vec3(mix(mix(a, b, smoothstep(0.0, 1.0, f.x)),
	//			mix(c, d, smoothstep(0.0, 1.0, f.x)),
	//			smoothstep(0.0, 1.0, f.y)));

	// Same code, with the clamps in smoothstep and common subexpressions
	// optimized away.
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void main()
{	
	vec4 seaCol = vec4(_SeaColor, 0.6f);

	vec2 ndc = (ClipSpaceCoord.xy/ClipSpaceCoord.w) / 2.0 + 0.5;
	vec2 reflectionTexCoord = vec2(ndc.x, -ndc.y);

	vec4 texCol = texture(reflectionTexture, reflectionTexCoord + vec2(noise(reflectionTexCoord * _SeaDistScale) * _SeaDistStrength));


	vec3 objectColor = mix(texCol, seaCol, 0.3).xyz;
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(_LightPosition - FragPos);
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = diff * _LightColor;
	vec4 result = vec4((vec3(0.2, 0.2, 0.2) + diffuse) * objectColor, _SeaAlpha);


	FragColor = result;
} 