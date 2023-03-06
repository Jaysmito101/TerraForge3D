#version 430 core

out vec4 FragColor;


in VertexData
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
} fragmentInput;

layout(std430, binding = 0) buffer DataBuffer0
{
    vec4 position_normals[];
};

#define MAX_LIGHTS 16
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT	   1

struct RendererLightData
{
	vec3 position;
	vec3 color;
	float intensity;
	int type;
};

uniform RendererLightData u_Lights[MAX_LIGHTS];
uniform int u_LightCount;
uniform int u_Resolution;
uniform float u_TileSize;
uniform bool u_InvertNormals;
uniform vec3 u_CameraPosition;
uniform bool u_EnableSkyLight;
uniform samplerCube u_IrradianceMap;

int PixelCoordToDataOffset(int x, int y)
{
	return y * u_Resolution + x;
}

// From : https://stackoverflow.com/a/5261402/14911094
mat3 calculateTBN()
{
	vec3 p_dx = dFdx(fragmentInput.position);
	vec3 p_dy = dFdy(fragmentInput.position);
	vec2 tc_dx = dFdx(fragmentInput.texCoord);
	vec2 tc_dy = dFdy(fragmentInput.texCoord);
	vec3 t = normalize(p_dx * tc_dy.y - p_dy * tc_dx.y);
	vec3 b = normalize(-p_dx * tc_dy.x + p_dy * tc_dx.x);
	vec3 n = cross(t, b);
	return mat3(t, b, n);
}

vec3 calculateNormal()
{
	ivec2 pointCoord = ivec2(fragmentInput.texCoord * u_Resolution);
	float T = position_normals[(pointCoord.y - 1) * u_Resolution + pointCoord.x].x;
	float B = position_normals[(pointCoord.y + 1) * u_Resolution + pointCoord.x].x;
	float R = position_normals[pointCoord.y * u_Resolution + (pointCoord.x + 1)].x;
	float L = position_normals[pointCoord.y * u_Resolution + (pointCoord.x - 1)].x;
	return normalize(vec3(2*(R-L), 2*(B-T), -4)) * (u_InvertNormals ? 1.0f : -1.0f);
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
	mat3 TBN = calculateTBN();
	vec3 normal  = calculateNormal(); normal = normalize(TBN * normal);
	vec3 outputColor = vec3(0.0f);
	float diff = 0.0f, spec = 0.0f, atten = 0.0f;
	for(int i = 0 ; i < u_LightCount ; i ++)
	{
		if(u_Lights[i].type == LIGHT_TYPE_DIRECTIONAL)
		{
			diff = max(dot(normalize(-u_Lights[i].position), normal), 0.0f);
			//spec = pow(max(dot(u_CameraPosition - fragmentInput.position, reflect(u_Lights[i].position, normal)), 0.0f), 4.0f);
			outputColor += u_Lights[i].intensity * u_Lights[i].color * ( 0.18f + spec + diff);
		}
		else
		{
			atten = 1.0f / pow(length(u_Lights[i].position - fragmentInput.position), 2.0f);
			diff = max(dot(normalize(u_Lights[i].position - fragmentInput.position), normal), 0.0f);
			//spec = pow(max(dot(u_CameraPosition - fragmentInput.position, reflect(-(u_Lights[i].position - fragmentInput.position), normal)), 0.0f), 4.0f);
			outputColor += atten * u_Lights[i].intensity * u_Lights[i].color * ( 0.18f + spec + diff);
		}
	}
	vec3 irradiance = texture(u_IrradianceMap, normal).rgb;
	if(u_EnableSkyLight) outputColor = outputColor + irradiance * 0.4f;
	outputColor = aces(outputColor);
	outputColor = pow(outputColor, vec3(1.0f/2.2f));
	FragColor = vec4(outputColor, 1.0f);
}